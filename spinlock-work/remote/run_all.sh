#!/bin/bash
# Build each code variant with TOP/build.sh and run the spin_mutex /
# TextureSystem contention benchmarks against it.
#
# Layout assumed (the "feedstock" machine):
#   TOP/            run this script from here:  oiio/spinlock-work/remote/run_all.sh
#   TOP/oiio/       OIIO git checkout (submodule), on branch lg-spinlock
#   TOP/build.sh    builds TOP/oiio into TOP/build, installs into TOP/dist
#                   (--clean = fresh config; EXTRA_CMAKE_ARGS = extra cmake args)
#
# For each variant in variants.txt, the script checks out that commit in
# TOP/oiio, runs ./build.sh, then runs that variant's benchmarks from
# TOP/build/bin. When it exits (normally, on error, or ^C), it restores
# TOP/oiio to the checkout it started from. Results go to
# TOP/oiio/spinlock-work/results/<RUN>/, to be committed and pushed.
#
# Environment knobs (all optional):
#   QUICK=1          short smoke run (few threads, few iterations)
#   STEPS            subset of: survey build ctest spinbench spinlock spinrw
#                    parallel testtex prodtex perf   (default: all but perf)
#   VARIANTS         subset of variant names from variants.txt (default: all)
#   TEXFILES         space-separated production texture paths, and/or
#   TEXLIST          file with one production texture path per line
#   SCRUB_NAMES=0    keep production file names in results (default: scrub)
#   PROD_ARGS        extra testtex args for the prodtex step only, e.g.
#                    "--cachesize 8192 --maxfiles 1000"
#   PROD_TAG         suffix for prodtex result names, e.g. "-c8g" gives
#                    testtex-<v>-prod-c8g-w7.txt (so runs don't collide)
#   PERF=1           also run the perf step (needs perf, perf_event_paranoid <= 1)
#   CLEAN_FIRST=1    pass --clean to build.sh for the first variant built
#   RUN              results subdir name; reuse an old one to RESUME a run
#   EXTRA_CMAKE_ARGS passed through to build.sh (we append -DOIIO_BUILD_TESTS=ON)
#
# Crash protocol: every command is logged to results/<RUN>/progress.log
# (START/END, then sync) before it runs. Finished steps leave .done-*
# markers, so rerunning with the same RUN=... skips them.

set -u

TOP=$(pwd)
if [ ! -x "$TOP/build.sh" ] || [ ! -e "$TOP/oiio/.git" ]; then
    echo "Run this from TOP (the dir containing build.sh and the oiio checkout)."
    exit 1
fi
SRC=$TOP/oiio
BUILD=$TOP/build
SW=$TOP/spinwork                 # scratch: script copy, spinbench, texture data
mkdir -p "$SW"

# Older variant commits don't contain these scripts, so run from a copy.
HERE=$(cd "$(dirname "$0")" && pwd)
if [ "$HERE" != "$SW/scripts/remote" ]; then
    rm -rf "$SW/scripts"; mkdir -p "$SW/scripts"
    cp -R "$SRC/spinlock-work/remote" "$SRC/spinlock-work/bench" "$SW/scripts/"
    exec bash "$SW/scripts/remote/run_all.sh" "$@"
fi
BENCH=$SW/scripts/bench

NCPU=$(nproc 2>/dev/null || sysctl -n hw.ncpu)
QUICK=${QUICK:-0}
PERF=${PERF:-0}
DEFAULT_STEPS="survey build ctest spinbench spinlock spinrw parallel testtex prodtex"
[ "$PERF" = 1 ] && DEFAULT_STEPS="$DEFAULT_STEPS perf"
STEPS=${STEPS:-$DEFAULT_STEPS}
RUN=${RUN:-$(hostname -s)-$(date +%Y%m%d-%H%M)$([ "$QUICK" = 1 ] && echo -quick)}
OUT=$SRC/spinlock-work/results/$RUN
CXX=${CXX:-c++}
mkdir -p "$OUT"
PROG=$OUT/progress.log

log()      { echo "$(date '+%F %T') $*" | tee -a "$PROG"; sync; }
want()     { [[ " $STEPS " == *" $1 "* ]]; }
is_done()  { [ -e "$OUT/.done-$1" ]; }
mark()     { touch "$OUT/.done-$1"; sync; }
timed()    { python3 -I "$HERE/timed.py" "$@"; }   # timed <logfile> cmd...
bin()      { echo "$BUILD/bin/$1"; }
have()     { [ -x "$BUILD/bin/$1" ]; }

# ---- refuse to clobber local edits; restore checkout on exit ------------
if [ -n "$(git -C "$SRC" status --porcelain --untracked-files=no)" ]; then
    echo "TOP/oiio has uncommitted changes to tracked files; commit or stash them first."
    git -C "$SRC" status --short --untracked-files=no
    exit 1
fi
ORIG_REF=$(git -C "$SRC" symbolic-ref -q --short HEAD || git -C "$SRC" rev-parse HEAD)
restore() {
    local cur
    cur=$(git -C "$SRC" rev-parse HEAD)
    if [ "$cur" != "$(git -C "$SRC" rev-parse "$ORIG_REF")" ] || \
       [ "$(git -C "$SRC" symbolic-ref -q --short HEAD)" != "$ORIG_REF" ]; then
        git -C "$SRC" checkout -q "$ORIG_REF" && log "restored TOP/oiio to $ORIG_REF"
        log "NOTE: TOP/build and TOP/dist now hold the last variant built; run ./build.sh to rebuild $ORIG_REF if needed."
    fi
}
trap restore EXIT
trap 'log "interrupted"; exit 130' INT TERM

# ---- variants ----------------------------------------------------------
declare -a VNAMES VSHAS
while read -r name sha _; do
    [[ -z "$name" || "$name" == \#* ]] && continue
    if [ -n "${VARIANTS:-}" ] && [[ " $VARIANTS " != *" $name "* ]]; then
        continue
    fi
    VNAMES+=("$name"); VSHAS+=("$sha")
done < "$HERE/variants.txt"
NV=${#VNAMES[@]}
[ $NV -gt 0 ] || { echo "no variants selected"; exit 1; }
LASTV=${VNAMES[$((NV-1))]}
has_variant() { [[ " ${VNAMES[*]} " == *" $1 "* ]]; }
SPINV=spin; has_variant spin || SPINV=$LASTV

# ---- thread counts -----------------------------------------------------
if [ "$QUICK" = 1 ]; then
    TLIST="1 4 16 $NCPU"
else
    TLIST="1 2 4 8 16 32 48 64 96 128 192 256 384 512"
fi
TLIST=$(for t in $TLIST $NCPU; do [ $t -le $((2*NCPU)) ] && echo $t; done | sort -n | uniq | tr '\n' ' ')

if [ "$QUICK" = 1 ]; then
    SL_ITERS=16000000; RW_ITERS=6400000; PAR_ITERS=100000
    TT_EXTRA="--iters 200000"; TT_WL="1 7"; PROD_WL="7"
else
    SL_ITERS=160000000; RW_ITERS=64000000; PAR_ITERS=1000000
    TT_EXTRA="--trials 3"; TT_WL="1 2 4 7 8"; PROD_WL="1 4 7 8"
fi

PRODFILES=()
[ -n "${TEXFILES:-}" ] && read -ra PRODFILES <<< "$TEXFILES"
if [ -n "${TEXLIST:-}" ] && [ -f "$TEXLIST" ]; then
    while read -r f; do [ -n "$f" ] && PRODFILES+=("$f"); done < "$TEXLIST"
fi

log "=== run_all.sh RUN=$RUN TOP=$TOP NCPU=$NCPU QUICK=$QUICK STEPS='$STEPS'"
log "variants: ${VNAMES[*]}   threads: $TLIST   production files: ${#PRODFILES[@]}"
log "TOP/oiio started at $ORIG_REF"

# ---- survey ------------------------------------------------------------
if want survey && ! is_done survey; then
    log "START survey"
    {
        echo "## date";      date
        echo "## git";       git -C "$SRC" log -1 --format='%H %s'
        echo "## uname";     uname -a
        echo "## nproc";     echo $NCPU
        echo "## lscpu";     lscpu 2>/dev/null || sysctl -a 2>/dev/null | grep -E 'machdep.cpu|hw.(n|physical|logical)cpu'
        echo "## numa";      numactl -H 2>/dev/null || echo "numactl not available"
        echo "## smt";       cat /sys/devices/system/cpu/smt/active 2>/dev/null || echo n/a
        echo "## governor";  cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo n/a
        echo "## memory";    free -g 2>/dev/null || sysctl -n hw.memsize
        echo "## compiler";  $CXX --version 2>&1 | head -2
        echo "## cmake";     cmake --version | head -1
        echo "## python";    python3 --version
        echo "## perf";      command -v perf || echo "no perf"
        echo "## EXTRA_CMAKE_ARGS"; echo "${EXTRA_CMAKE_ARGS:-}"
        echo "## build CMakeCache (selected)"
        grep -E '^(CMAKE_BUILD_TYPE|CMAKE_CXX_COMPILER|CMAKE_CXX_FLAGS|CMAKE_CXX_STANDARD|USE_|OIIO_|TBB|OpenImageIO_BUILD)' \
            "$BUILD/CMakeCache.txt" 2>/dev/null | head -60
    } > "$OUT/machine.txt" 2>&1
    log "END survey"; mark survey
fi

# ---- per-variant build + benchmarks -------------------------------------
build_variant() {   # name sha
    local v=$1 sha=$2 clean=""
    log "START checkout $v ($sha)"
    git -C "$SRC" checkout -q --detach "$sha" || { log "FAIL checkout $v"; return 1; }
    log "START build.sh $v"
    if [ "${CLEAN_FIRST:-0}" = 1 ] && [ ! -e "$OUT/.cleaned" ]; then
        clean=--clean; touch "$OUT/.cleaned"
    fi
    ( cd "$TOP" && EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS:-} -DOIIO_BUILD_TESTS=ON" ./build.sh $clean ) \
        > "$OUT/build-$v.log" 2>&1
    local rc=$?
    local missing=""
    for p in testtex spinlock_test spin_rw_test parallel_test; do have $p || missing="$missing $p"; done
    if [ $rc -ne 0 ] || [ -n "$missing" ]; then
        log "FAIL build.sh $v rc=$rc missing:$missing (see build-$v.log)"
        return 1
    fi
    tail -5 "$OUT/build-$v.log" > "$OUT/build-$v.tail"; rm -f "$OUT/build-$v.log"
    log "END build.sh $v OK"
}

run_unit() {   # step variant prog args...   (one run per thread count)
    local step=$1 v=$2 prog=$3; shift 3
    is_done $step-$v && return
    for n in $TLIST; do
        log "START $step $v threads=$n"
        r=$(timed "$OUT/$step-$v.log" "$(bin $prog)" --threads $n "$@")
        echo "$v $n $r" >> "$OUT/$step.txt"
        log "END $step $v threads=$n $r"
    done
    mark $step-$v
}

testtex_one() {   # variant tag cwd args...
    local v=$1 tag=$2 dir=$3; shift 3
    is_done testtex-$v-$tag && return
    mkdir -p "$dir"
    log "START testtex $v $tag"
    r=$(cd "$dir" && timed "$OUT/testtex-$v-$tag.txt" "$(bin testtex)" "$@")
    log "END testtex $v $tag $r"
    mark testtex-$v-$tag
}

variant_done() { is_done variant-$1; }

for i in "${!VNAMES[@]}"; do
    v=${VNAMES[$i]}; sha=${VSHAS[$i]}
    variant_done $v && { log "skip $v (already complete)"; continue; }

    if want build || ! have testtex; then
        build_variant $v $sha || continue
    else
        git -C "$SRC" checkout -q --detach "$sha"
        log "NOTE: STEPS has no 'build'; using whatever is in TOP/build for $v"
    fi

    # ---- ctest (newest variant only) ----
    if [ "$v" = "$LASTV" ] && want ctest && ! is_done ctest; then
        log "START ctest on $v"
        (
            cd "$BUILD" &&
            ctest -R '^openvdb$' --timeout 900   # sets up a fixture texture3d needs
            # texture-udim* excluded: their generated inputs depend on font lookup.
            ctest -R 'unit_spinlock|unit_spin_rw|unit_thread|unit_parallel|unit_ustring|unit_atomic|texture|imagecache|maketx' \
                  -E 'texture-udim' -j 8 --timeout 1800
        ) > "$OUT/ctest.txt" 2>&1
        log "END ctest rc=$? : $(grep -E 'tests passed|tests failed' "$OUT/ctest.txt" | tail -1)"
        mark ctest
    fi

    # ---- spinbench (needs the spin variant's headers) ----
    if [ "$v" = "$SPINV" ] && want spinbench && ! is_done spinbench; then
        SB=$SW/spinbench; mkdir -p "$SB"
        if grep -nE '\.wait\(|notify_' "$BENCH/spinbench.cpp"; then
            log "SAFETY: wait/notify in spinbench.cpp, skipping"
        else
            log "START spinbench compile"
            inc="-I$SRC/src/include -I$BUILD/include"
            $CXX -std=c++17 -O3 -fno-math-errno -DNDEBUG -pthread -DWITH_OIIO $inc \
                 "$BENCH/spinbench.cpp" -o "$SB/spinbench17o3" > "$OUT/spinbench-compile.txt" 2>&1
            $CXX -std=c++20 -O3 -fno-math-errno -DNDEBUG -pthread -DWITH_OIIO $inc \
                 "$BENCH/spinbench.cpp" -o "$SB/spinbench20o3" >> "$OUT/spinbench-compile.txt" 2>&1
            log "END spinbench compile"
            "$SB/spinbench17o3" pausecost > "$OUT/pausecost.txt" 2>&1
            if [ "$QUICK" = 1 ]; then
                ops=400000; trials=1; re='^(A_cur|X_bool_isb_128y64|O_oiio_spin_mutex|H_stdmutex)$'
            else
                ops=4000000; trials=5
                re='^(A_cur|A2_cur_nodclp|B_flag_isb_16y1|C_bool_isb_16y1|D_article|E_bool_isb_64y64|X_.*|O_oiio_spin_mutex|H_stdmutex|F_flagtest_isb_64y16)$'
            fi
            log "START spinbench run (per-process progress in bench.log)"
            BINDIR=$SB BINS="spinbench17o3 spinbench20o3" SHAPES="tiny medium coloc" LOG="$OUT/bench.log" \
                bash "$BENCH/run.bash" $ops $trials "$OUT/spinbench.txt" "$re" $TLIST
            log "END spinbench run"
            nodes=$(numactl -H 2>/dev/null | awk '/^available:/{print $2}')
            if [ -n "$nodes" ] && [ "$nodes" -gt 1 ]; then
                ncpu0=$(numactl -H | awk '/^node 0 cpus:/{print NF-3}')
                tl0=$(for t in $TLIST; do [ $t -le $ncpu0 ] && echo $t; done | tr '\n' ' ')
                log "START spinbench numa node0 ($ncpu0 cpus)"
                numactl --cpunodebind=0 --membind=0 env BINDIR=$SB BINS=spinbench17o3 SHAPES="tiny medium coloc" \
                    LOG="$OUT/bench.log" bash "$BENCH/run.bash" $ops $trials "$OUT/spinbench-node0.txt" \
                    '^(A_cur|X_bool_isb_128y64|O_oiio_spin_mutex|H_stdmutex)$' $tl0
                log "END spinbench numa"
            fi
        fi
        mark spinbench
    fi

    # ---- lock unit benchmarks (base and spin only; later variants share spin's locks) ----
    if [ "$v" = base ] || [ "$v" = "$SPINV" ]; then
        want spinlock && run_unit spinlock $v spinlock_test --iters $SL_ITERS --trials 3
        want parallel && run_unit parallel $v parallel_test --iters $PAR_ITERS --trials 3
        if want spinrw && ! is_done spinrw-$v; then
            # spin_rw_test only honors --threads with --wedge (fixed list of counts up to N)
            log "START spinrw $v wedge to $((2*NCPU))"
            r=$(timed "$OUT/spinrw-$v.log" "$(bin spin_rw_test)" --wedge --threads $((2*NCPU)) --iters $RW_ITERS --trials 3)
            log "END spinrw $v $r"
            mark spinrw-$v
        fi
    fi

    # ---- testtex on generated textures ----
    if want testtex; then
        d=$SW/texdata
        gen10='--maketests 10 test{:04}.exr --maketest-res 1024'
        gen64='--maketests 64 many{:04}.exr --maketest-res 512'
        for m in $TT_WL; do
            testtex_one $v w$m "$d" $gen10 --threadtimes $m --wedge --threads $NCPU $TT_EXTRA
        done
        # Many files with a low open-file limit (exercises the file sweep lock).
        testtex_one $v maxfiles16 "$d" $gen64 --maxfiles 16 --threadtimes 7 --wedge --threads $NCPU $TT_EXTRA
        # Small cache: tile churn.
        testtex_one $v cache64 "$d" $gen64 --cachesize 64 --threadtimes 7 --wedge --threads $NCPU $TT_EXTRA
        # Stats at full thread count (micro-cache misses, mutex wait times).
        testtex_one $v stats-w1 "$d" $gen10 --threadtimes 1 --threads $NCPU --runstats
        testtex_one $v stats-w7 "$d" $gen10 --threadtimes 7 --threads $NCPU --runstats
    fi

    # ---- testtex on production textures ----
    if want prodtex && [ ${#PRODFILES[@]} -gt 0 ]; then
        d=$SW/proddata
        pt=prod${PROD_TAG:-}
        for m in $PROD_WL; do
            testtex_one $v $pt-w$m "$d" --threadtimes $m --wedge --threads $NCPU $TT_EXTRA ${PROD_ARGS:-} "${PRODFILES[@]}"
        done
        testtex_one $v $pt-stats-w7 "$d" --threadtimes 7 --threads $NCPU --runstats ${PROD_ARGS:-} "${PRODFILES[@]}"
        if [ "${SCRUB_NAMES:-1}" = 1 ]; then
            printf '%s\n' "${PRODFILES[@]}" > "$SW/prodfiles.lst"
            python3 -I "$HERE/scrub.py" "$SW/prodfiles.lst" "$OUT"/testtex-*-prod*.txt "$PROG"
        fi
    fi

    # ---- perf (optional; base, spin, newest) ----
    if want perf && command -v perf > /dev/null && \
       { [ "$v" = base ] || [ "$v" = "$SPINV" ] || [ "$v" = "$LASTV" ]; }; then
        d=$SW/texdata; mkdir -p "$d"
        for m in 1 7; do
            is_done perf-$v-w$m && continue
            log "START perf c2c $v w$m"
            (cd "$d" && perf c2c record -o "$SW/c2c-$v-w$m.data" -- \
                "$(bin testtex)" --maketests 10 'test{:04}.exr' --maketest-res 1024 \
                --threadtimes $m --threads $NCPU --iters 500000) > "$OUT/perf-record-$v-w$m.log" 2>&1
            perf c2c report -i "$SW/c2c-$v-w$m.data" --stdio 2>/dev/null | head -400 > "$OUT/c2c-$v-w$m.txt"
            log "END perf c2c $v w$m"
            log "START perf record $v w$m"
            (cd "$d" && perf record -g -o "$SW/perf-$v-w$m.data" -- \
                "$(bin testtex)" --maketests 10 'test{:04}.exr' --maketest-res 1024 \
                --threadtimes $m --threads $NCPU --iters 500000) >> "$OUT/perf-record-$v-w$m.log" 2>&1
            perf report -i "$SW/perf-$v-w$m.data" --stdio --no-children --percent-limit 0.5 2>/dev/null \
                | head -300 > "$OUT/perf-$v-w$m.txt"
            log "END perf record $v w$m"
            mark perf-$v-w$m
        done
    fi

    mark variant-$v
    log "=== variant $v complete"
done

# ---- summary ------------------------------------------------------------
python3 -I "$HERE/summarize.py" "$OUT" > "$OUT/SUMMARY.md" 2> "$OUT/summarize.err"
log "=== done. Summary: $OUT/SUMMARY.md"
cat <<EOF

Next steps (from TOP):
  git -C oiio add spinlock-work/results/$RUN
  git -C oiio commit -m "spinlock-work: results from $RUN"
  git -C oiio push
EOF
