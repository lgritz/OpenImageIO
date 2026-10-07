#!/bin/bash
# Build each code variant and run the spin_mutex / TextureSystem contention
# benchmarks. Results go to spinlock-work/results/<RUN>/ for committing.
#
# Usage (from anywhere in the repo checkout of branch lg-spinlock):
#   spinlock-work/remote/run_all.sh
#
# Environment knobs (all optional):
#   OIIO_CMAKE_ARGS  extra cmake args for every variant build, e.g.
#                    "-DCMAKE_PREFIX_PATH=/opt/deps -DUSE_PYTHON=0"
#   WORK             scratch dir for worktrees/builds (default: <repo>/../oiio-spinwork)
#   JOBS             build parallelism (default: nproc)
#   QUICK=1          short smoke run (few threads, few iterations)
#   STEPS            subset of: survey build ctest spinbench spinlock spinrw
#                    parallel testtex prodtex perf   (default: all but perf)
#   VARIANTS         subset of variant names from variants.txt (default: all)
#   TEXFILES         space-separated production texture paths, and/or
#   TEXLIST          file with one production texture path per line
#   SCRUB_NAMES=0    keep production file names in results (default: scrub them)
#   PERF=1           also run the perf step (needs perf; perf_event_paranoid <= 1)
#   RUN              results subdir name; reuse an old one to RESUME a run
#
# Crash protocol: every command is logged to results/<RUN>/progress.log
# (START/END, then sync) before it runs. Finished steps leave a .done-*
# marker, so rerunning with the same RUN=... skips them.

set -u
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(git -C "$HERE" rev-parse --show-toplevel)
BENCH=$REPO/spinlock-work/bench
WORK=${WORK:-$(dirname "$REPO")/oiio-spinwork}
NCPU=$(nproc 2>/dev/null || sysctl -n hw.ncpu)
JOBS=${JOBS:-$NCPU}
QUICK=${QUICK:-0}
PERF=${PERF:-0}
DEFAULT_STEPS="survey build ctest spinbench spinlock spinrw parallel testtex prodtex"
[ "$PERF" = 1 ] && DEFAULT_STEPS="$DEFAULT_STEPS perf"
STEPS=${STEPS:-$DEFAULT_STEPS}
RUN=${RUN:-$(hostname -s)-$(date +%Y%m%d-%H%M)$([ "$QUICK" = 1 ] && echo -quick)}
OUT=$REPO/spinlock-work/results/$RUN
CXX=${CXX:-c++}
mkdir -p "$OUT" "$WORK"
PROG=$OUT/progress.log
read -ra CMARGS <<< "${OIIO_CMAKE_ARGS:-}"

log()      { echo "$(date '+%F %T') $*" | tee -a "$PROG"; sync; }
want()     { [[ " $STEPS " == *" $1 "* ]]; }
is_done()  { [ -e "$OUT/.done-$1" ]; }
mark()     { touch "$OUT/.done-$1"; sync; }
timed()    { python3 -I "$HERE/timed.py" "$@"; }   # timed <logfile> cmd...

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
bin() { echo "$WORK/$1/build/bin/$2"; }
built() { [ -x "$(bin $1 $2)" ]; }

# ---- thread counts -----------------------------------------------------
if [ "$QUICK" = 1 ]; then
    TLIST="1 4 16 $NCPU"
else
    TLIST="1 2 4 8 16 32 48 64 96 128 192 256 384 512"
fi
TLIST=$(for t in $TLIST $NCPU; do [ $t -le $((2*NCPU)) ] && echo $t; done | sort -n | uniq | tr '\n' ' ')

log "=== run_all.sh RUN=$RUN NCPU=$NCPU QUICK=$QUICK STEPS='$STEPS'"
log "variants: ${VNAMES[*]}   threads: $TLIST   WORK=$WORK"

# ---- survey ------------------------------------------------------------
if want survey && ! is_done survey; then
    log "START survey"
    {
        echo "## date";      date
        echo "## git";       git -C "$REPO" log -1 --format='%H %s'
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
        echo "## OIIO_CMAKE_ARGS"; echo "${OIIO_CMAKE_ARGS:-}"
    } > "$OUT/machine.txt" 2>&1
    log "END survey"; mark survey
fi

# ---- build -------------------------------------------------------------
if want build; then
    for i in "${!VNAMES[@]}"; do
        v=${VNAMES[$i]}; sha=${VSHAS[$i]}
        is_done build-$v && continue
        src=$WORK/$v/src; bld=$WORK/$v/build
        log "START build $v ($sha)"
        if [ ! -d "$src" ]; then
            git -C "$REPO" worktree add --detach "$src" "$sha" >> "$OUT/build-$v.log" 2>&1 \
                || { log "FAIL worktree $v"; continue; }
        fi
        # The testsuite looks for oiio-images, openexr-images, etc. next to
        # the source tree. Link any that sit next to the main checkout.
        for d in "$(dirname "$REPO")"/*-images "$(dirname "$REPO")"/libtiffpic "$(dirname "$REPO")"/j2kp4files_v1_5; do
            [ -d "$d" ] && [ ! -e "$WORK/$v/$(basename "$d")" ] && ln -s "$d" "$WORK/$v/$(basename "$d")"
        done
        if [ ! -f "$bld/CMakeCache.txt" ]; then
            cmake -S "$src" -B "$bld" -DCMAKE_BUILD_TYPE=Release \
                -DPython3_EXECUTABLE="$(command -v python3)" "${CMARGS[@]}" \
                >> "$OUT/build-$v.log" 2>&1 || { log "FAIL configure $v (see build-$v.log)"; continue; }
        fi
        targets="testtex spinlock_test spin_rw_test parallel_test"
        [ "$v" = "$LASTV" ] && want ctest && targets=all
        if [ "$targets" = all ]; then
            cmake --build "$bld" -j "$JOBS" >> "$OUT/build-$v.log" 2>&1
        else
            cmake --build "$bld" -j "$JOBS" --target $targets >> "$OUT/build-$v.log" 2>&1
        fi
        if [ $? -eq 0 ]; then
            log "END build $v OK"; mark build-$v
            tail -3 "$OUT/build-$v.log" > "$OUT/build-$v.tail"; rm -f "$OUT/build-$v.log"
        else
            log "FAIL build $v (see build-$v.log)"
        fi
    done
fi

# ---- ctest (newest variant) --------------------------------------------
if want ctest && ! is_done ctest && built $LASTV testtex; then
    if ! built $LASTV oiiotool; then
        log "START full build of $LASTV for ctest"
        cmake --build "$WORK/$LASTV/build" -j "$JOBS" > "$OUT/build-$LASTV-all.log" 2>&1 \
            && rm -f "$OUT/build-$LASTV-all.log"
        log "END full build of $LASTV"
    fi
    log "START ctest on $LASTV"
    (
        cd "$WORK/$LASTV/build" &&
        ctest -R '^openvdb$' --timeout 900   # sets up a fixture texture3d needs
        # texture-udim* are excluded: from a worktree, oiiotool -text picks a
        # different font, so its generated inputs differ (not a code issue).
        ctest -R 'unit_spinlock|unit_spin_rw|unit_thread|unit_parallel|unit_ustring|unit_atomic|texture|imagecache|maketx' \
              -E 'texture-udim' \
              -j 8 --timeout 1800
    ) > "$OUT/ctest.txt" 2>&1
    log "END ctest rc=$? : $(grep -E 'tests passed|tests failed' "$OUT/ctest.txt" | tail -1)"
    mark ctest
fi

# ---- spinbench (standalone lock microbenchmark) ------------------------
if want spinbench && ! is_done spinbench && [ -d "$WORK/$SPINV/build/include" ]; then
    SB=$WORK/spinbench; mkdir -p "$SB"
    log "START spinbench compile"
    if grep -nE '\.wait\(|notify_' "$BENCH/spinbench.cpp"; then
        log "SAFETY: wait/notify in spinbench.cpp, skipping";
    else
        inc="-I$WORK/$SPINV/src/src/include -I$WORK/$SPINV/build/include"
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
        log "START spinbench run (see bench.log for per-process progress)"
        BINDIR=$SB BINS="spinbench17o3 spinbench20o3" SHAPES="tiny medium coloc" LOG="$OUT/bench.log" \
            bash "$BENCH/run.bash" $ops $trials "$OUT/spinbench.txt" "$re" $TLIST
        log "END spinbench run"
        # Single NUMA node comparison, if there is more than one node.
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

# ---- spinlock_test / spin_rw_test / parallel_test, old vs new ----------
run_unit() {   # step prog extra-args...
    local step=$1 prog=$2; shift 2
    is_done $step && return
    for v in base $SPINV; do
        has_variant $v && built $v $prog || continue
        for n in $TLIST; do
            log "START $step $v threads=$n"
            r=$(timed "$OUT/$step-$v.log" "$(bin $v $prog)" --threads $n "$@")
            echo "$v $n $r" >> "$OUT/$step.txt"
            log "END $step $v threads=$n $r"
        done
    done
    mark $step
}
if [ "$QUICK" = 1 ]; then SL_ITERS=16000000; RW_ITERS=6400000; PAR_ITERS=100000
else SL_ITERS=160000000; RW_ITERS=64000000; PAR_ITERS=1000000; fi
# spinlock_test and parallel_test: total work split across --threads N.
want spinlock && run_unit spinlock spinlock_test --iters $SL_ITERS --trials 3
want parallel && run_unit parallel parallel_test --iters $PAR_ITERS --trials 3
# spin_rw_test only honors --threads with --wedge (fixed list of counts up to N).
if want spinrw && ! is_done spinrw; then
    for v in base $SPINV; do
        has_variant $v && built $v spin_rw_test || continue
        log "START spinrw $v wedge to $((2*NCPU))"
        r=$(timed "$OUT/spinrw-$v.log" "$(bin $v spin_rw_test)" --wedge --threads $((2*NCPU)) --iters $RW_ITERS --trials 3)
        log "END spinrw $v $r"
    done
    mark spinrw
fi

# ---- testtex: TextureSystem scaling on generated textures ---------------
testtex_one() {   # variant tag cwd args...
    local v=$1 tag=$2 dir=$3; shift 3
    mkdir -p "$dir"
    log "START testtex $v $tag"
    r=$(cd "$dir" && timed "$OUT/testtex-$v-$tag.txt" "$(bin $v testtex)" "$@")
    log "END testtex $v $tag $r"
}
if want testtex && ! is_done testtex; then
    if [ "$QUICK" = 1 ]; then WL="1 7"; IT="--iters 200000"; else WL="1 2 4 7 8"; IT=""; fi
    gen10='--maketests 10 test{:04}.exr --maketest-res 1024'
    gen64='--maketests 64 many{:04}.exr --maketest-res 512'
    # Interleave variants within each workload, to spread thermal drift.
    for m in $WL; do
        for v in "${VNAMES[@]}"; do
            built $v testtex || continue
            testtex_one $v w$m "$WORK/texdata-$v" -t 0 $gen10 --threadtimes $m --wedge --threads $NCPU $IT
        done
    done
    for v in "${VNAMES[@]}"; do
        built $v testtex || continue
        # Many files with a low open-file limit: exercises the file sweep lock.
        testtex_one $v maxfiles16 "$WORK/texdata-$v" -t 0 $gen64 --maxfiles 16 --threadtimes 7 --wedge --threads $NCPU $IT
        # Small cache: tile churn.
        testtex_one $v cache64 "$WORK/texdata-$v" -t 0 $gen64 --cachesize 64 --threadtimes 7 --wedge --threads $NCPU $IT
        # Stats at full thread count (microcache misses, mutex wait times).
        testtex_one $v stats-w1 "$WORK/texdata-$v" -t 0 $gen10 --threadtimes 1 --threads $NCPU --runstats $IT
        testtex_one $v stats-w7 "$WORK/texdata-$v" -t 0 $gen10 --threadtimes 7 --threads $NCPU --runstats $IT
    done
    mark testtex
fi

# ---- prodtex: TextureSystem scaling on production textures --------------
PRODFILES=()
[ -n "${TEXFILES:-}" ] && read -ra PRODFILES <<< "$TEXFILES"
if [ -n "${TEXLIST:-}" ] && [ -f "$TEXLIST" ]; then
    while read -r f; do [ -n "$f" ] && PRODFILES+=("$f"); done < "$TEXLIST"
fi
if want prodtex && ! is_done prodtex && [ ${#PRODFILES[@]} -gt 0 ]; then
    if [ "$QUICK" = 1 ]; then WL="7"; IT="--iters 200000"; else WL="1 4 7 8"; IT=""; fi
    for m in $WL; do
        for v in "${VNAMES[@]}"; do
            built $v testtex || continue
            testtex_one $v prod-w$m "$WORK/proddata-$v" --threadtimes $m --wedge --threads $NCPU $IT "${PRODFILES[@]}"
        done
    done
    for v in "${VNAMES[@]}"; do
        built $v testtex || continue
        testtex_one $v prod-stats-w7 "$WORK/proddata-$v" --threadtimes 7 --threads $NCPU --runstats $IT "${PRODFILES[@]}"
    done
    if [ "${SCRUB_NAMES:-1}" = 1 ]; then
        printf '%s\n' "${PRODFILES[@]}" > "$WORK/prodfiles.lst"
        python3 -I "$HERE/scrub.py" "$WORK/prodfiles.lst" "$OUT"/testtex-*-prod-*.txt
        log "scrubbed production file names from results"
    fi
    mark prodtex
fi

# ---- perf (optional) ----------------------------------------------------
if want perf && ! is_done perf && command -v perf > /dev/null; then
    for v in base $SPINV $LASTV; do
        built $v testtex || continue
        for m in 1 7; do
            d="$WORK/texdata-$v"; mkdir -p "$d"
            log "START perf c2c $v w$m"
            (cd "$d" && perf c2c record -o "$WORK/c2c-$v-w$m.data" -- \
                "$(bin $v testtex)" -t 0 --maketests 10 'test{:04}.exr' --maketest-res 1024 \
                --threadtimes $m --threads $NCPU --iters 500000) > "$OUT/perf-record-$v-w$m.log" 2>&1
            perf c2c report -i "$WORK/c2c-$v-w$m.data" --stdio 2>/dev/null | head -400 > "$OUT/c2c-$v-w$m.txt"
            log "END perf c2c $v w$m"
            log "START perf record $v w$m"
            (cd "$d" && perf record -g -o "$WORK/perf-$v-w$m.data" -- \
                "$(bin $v testtex)" -t 0 --maketests 10 'test{:04}.exr' --maketest-res 1024 \
                --threadtimes $m --threads $NCPU --iters 500000) >> "$OUT/perf-record-$v-w$m.log" 2>&1
            perf report -i "$WORK/perf-$v-w$m.data" --stdio --no-children --percent-limit 0.5 2>/dev/null \
                | head -300 > "$OUT/perf-$v-w$m.txt"
            log "END perf record $v w$m"
        done
    done
    mark perf
fi

# ---- summary ------------------------------------------------------------
python3 -I "$HERE/summarize.py" "$OUT" > "$OUT/SUMMARY.md" 2> "$OUT/summarize.err"
log "=== done. Summary: $OUT/SUMMARY.md"
cat <<EOF

Next steps:
  git -C "$REPO" add spinlock-work/results/$RUN
  git -C "$REPO" commit -m "spinlock-work: results from $RUN"
  git -C "$REPO" push
EOF
