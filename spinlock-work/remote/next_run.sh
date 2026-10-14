#!/bin/bash
# The current "next run" (2026-10-14): phase 3 victim microcache on a
# multi-socket box. About 45-60 minutes. Writes results/<host>-mc/.
# One build (lg-spinlock = phase 1 + 2 + victim microcache). The victim
# cache size is set at run time with
#   OPENIMAGEIO_IMAGECACHE_OPTIONS=microcache_size=N   (N=0 is phase 2 behavior)
# for N in MC_SIZES (default "0 16 64 256"), on pinned CPU sets:
#   free-1, s0cores (socket 0, 1 thread/core), split (half the cores of
#   each socket), allcores (1 thread/core everywhere), free-<ncpu>.
# Workloads: synthetic testtex w1 w4 w7 w8; prod w7 w8 at 8 GB and 1 GB
# cache (if a list is given), plus --runstats at 1 GB and 256 MB to see
# whether held tiles raise redundant reads. Also ctest (texture/imagecache).
# Then commits ALL results and pushes with `(cd oiio ; git50 push lgritz lg-spinlock)`.
#
# Usage, from TOP (~/code/openimageio-feedstock), after
# `git -C oiio checkout lg-spinlock && git -C oiio pull`:
#     oiio/spinlock-work/remote/next_run.sh [/path/to/prodtextures.txt]
# Safe to rerun: finished pieces (non-empty result files) are skipped.
#
# TOP/build is shared with other machines: do not run on two at once.

set -u
TEXLIST=${1:-${TEXLIST:-}}
if [ -n "$TEXLIST" ]; then
    [ -f "$TEXLIST" ] || { echo "No such list: $TEXLIST"; exit 1; }
    TEXLIST=$(cd "$(dirname "$TEXLIST")" && pwd)/$(basename "$TEXLIST")
fi
if [ ! -x ./build.sh ] || [ ! -e oiio/.git ]; then
    echo "Run this from TOP (the dir containing build.sh and oiio/)."
    exit 1
fi
TOP=$(pwd)
HOST=$(hostname -s)
RUN=$HOST-mc
OUT=$TOP/oiio/spinlock-work/results/$RUN
SW=$TOP/spinwork/mc
mkdir -p "$OUT" "$SW"
LOG=$OUT/progress.log
log() { echo "$(date '+%F %T') $*" | tee -a "$LOG"; sync; }
MC_SIZES=${MC_SIZES:-"0 16 64 256"}

HAVE_TASKSET=0
command -v taskset >/dev/null 2>&1 && HAVE_TASKSET=1
[ $HAVE_TASKSET = 1 ] || log "taskset not found; pinned configs skipped"

# ---- topology -> CPU sets ---------------------------------------------
topo=$SW/topo.txt
for c in /sys/devices/system/cpu/cpu[0-9]*; do
    n=${c##*cpu}
    pkg=$(cat "$c/topology/physical_package_id")
    first=$(sed 's/[,-].*//' "$c/topology/thread_siblings_list")
    echo "$n $pkg $first"
done | sort -n > "$topo"
cp "$topo" "$OUT/topology.txt"
NCPU=$(wc -l < "$topo")
NCORES=$(awk '$1==$3' "$topo" | wc -l)
NPKG=$(awk '{print $2}' "$topo" | sort -u | wc -l)
S0CORES=$(awk '$2==0 && $1==$3 {print $1}' "$topo" | paste -sd, -)
ALLCORES=$(awk '$1==$3 {print $1}' "$topo" | paste -sd, -)
SPLIT=$(awk '$1==$3 {c[$2]++; if (c[$2] <= h) print $1}' \
        h=$(( NCORES / 2 / NPKG )) "$topo" | paste -sd, -)
count() { echo "$1" | tr , '\n' | wc -l; }
CONFIGS=( "free 1 -" )
if [ $HAVE_TASKSET = 1 ]; then
    CONFIGS+=( "s0cores $(count "$S0CORES") $S0CORES" )
    [ "$NPKG" -gt 1 ] && CONFIGS+=( "split $(count "$SPLIT") $SPLIT" )
    CONFIGS+=( "allcores $(count "$ALLCORES") $ALLCORES" )
fi
CONFIGS+=( "free $NCPU -" )
printf '%s\n' "${CONFIGS[@]}" | tee "$OUT/configs.txt"
pin() {   # cpulist, then command
    local cpus=$1; shift
    if [ "$cpus" = - ]; then "$@"; else taskset -c "$cpus" "$@"; fi
}

# ---- build + ctest ----------------------------------------------------
log "START build lg-spinlock $(git -C oiio rev-parse --short HEAD)"
( EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS:-} -DOIIO_BUILD_TESTS=ON" ./build.sh ) \
    > "$SW/build.log" 2>&1 || { log "build FAILED; see $SW/build.log"; tail -30 "$SW/build.log"; exit 1; }
tail -5 "$SW/build.log" > "$OUT/build.tail"
git -C oiio log -1 --format='%H %s' > "$OUT/git.txt"
log "END build"
TESTTEX=$TOP/build/bin/testtex
if [ ! -s "$OUT/ctest.txt" ]; then
    log "START ctest"
    ( cd "$TOP/build" &&
      ctest -R '^openvdb$' --timeout 900
      ctest -R 'texture|imagecache|maketx' -E 'texture-udim' -j 8 --timeout 1800
    ) > "$OUT/ctest.txt" 2>&1
    log "END ctest: $(grep -E 'tests passed|tests failed' "$OUT/ctest.txt" | tail -1)"
fi

PRODFILES=()
if [ -n "$TEXLIST" ]; then
    while read -r f; do [ -n "$f" ] && PRODFILES+=("$f"); done < "$TEXLIST"
fi
GEN='--maketests 10 test{:04}.exr --maketest-res 1024'

tt() {   # outfile, mc, cpus, testtex args...
    local f=$1 mc=$2 cpus=$3; shift 3
    [ -s "$f" ] && return
    OPENIMAGEIO_IMAGECACHE_OPTIONS="microcache_size=$mc" pin "$cpus" "$TESTTEX" "$@" > "$f" 2>&1
}

# ---- timing runs ------------------------------------------------------
cd "$SW"
for cfg in "${CONFIGS[@]}"; do
    read -r name nt cpus <<< "$cfg"
    tag=$name-$nt
    log "START $tag"
    for mc in $MC_SIZES; do
        for m in 1 4 7 8; do
            tt "$OUT/tt-w$m-$tag-mc$mc.txt" $mc "$cpus" $GEN \
                --threadtimes $m --threads "$nt" --trials 3
        done
        if [ ${#PRODFILES[@]} -gt 0 ]; then
            for c in 8192 1024; do
                for m in 7 8; do
                    tt "$OUT/tt-prod-c$c-w$m-$tag-mc$mc.txt" $mc "$cpus" \
                        --threadtimes $m --threads "$nt" --trials 3 \
                        --cachesize $c --maxfiles 1000 "${PRODFILES[@]}"
                done
            done
        fi
    done
    log "END $tag"
done

# ---- stats runs: does holding tiles raise redundant reads? -------------
if [ ${#PRODFILES[@]} -gt 0 ]; then
    log "START stats"
    for mc in $MC_SIZES; do
        for c in 1024 256; do
            tt "$OUT/stats-prod-c$c-w7-free-$NCPU-mc$mc.txt" $mc - \
                --threadtimes 7 --threads "$NCPU" --runstats \
                --cachesize $c --maxfiles 1000 "${PRODFILES[@]}"
        done
    done
    log "END stats"
fi
cd "$TOP"
if [ ${#PRODFILES[@]} -gt 0 ] && [ "${SCRUB_NAMES:-1}" = 1 ]; then
    printf '%s\n' "${PRODFILES[@]}" > "$SW/prodfiles.lst"
    python3 -I oiio/spinlock-work/remote/scrub.py "$SW/prodfiles.lst" \
        "$OUT"/tt-prod-*.txt "$OUT"/stats-prod-*.txt
fi

# ---- summary ------------------------------------------------------------
{
    t() { awk -v n="$2" '$1==n && $2 ~ /^[0-9.]+$/ {print $2; exit}' "$1" 2>/dev/null; }
    hdr="| workload | config |"; sep="|---|---|"
    for mc in $MC_SIZES; do hdr="$hdr mc$mc |"; sep="$sep---|"; done
    echo "## testtex seconds (best of 3)"; echo; echo "$hdr"; echo "$sep"
    for w in w1 w4 w7 w8 prod-c8192-w7 prod-c8192-w8 prod-c1024-w7 prod-c1024-w8; do
        for cfg in "${CONFIGS[@]}"; do
            read -r name nt cpus <<< "$cfg"
            [ -e "$OUT/tt-$w-$name-$nt-mc${MC_SIZES%% *}.txt" ] || continue
            row="| $w | $name-$nt |"
            for mc in $MC_SIZES; do row="$row $(t "$OUT/tt-$w-$name-$nt-mc$mc.txt" "$nt") |"; done
            echo "$row"
        done
    done
    echo
    echo "## prod w7 --runstats at $NCPU threads"
    for f in "$OUT"/stats-prod-*.txt; do
        [ -e "$f" ] || continue
        echo; echo "### $(basename "$f" .txt)"; echo '```'
        grep -E 'micro-cache|redundant|Peak cache|Tiles:|File I/O|^ *[0-9]+ +[0-9.]+ ' "$f"
        echo '```'
    done
} > "$OUT/SUMMARY.md"
cat "$OUT/SUMMARY.md"

# ---- commit + push ----------------------------------------------------
git -C oiio add spinlock-work/results
if git -C oiio diff --cached --quiet; then
    echo "Nothing new to commit."
else
    git -C oiio commit -q -m "spinlock-work: results from $RUN"
    git -C oiio log --oneline -1
fi
echo
# git50 may be an alias or shell function, which a non-interactive script
# can't see, so fall back to an interactive shell.
PUSH='cd oiio ; git50 push lgritz lg-spinlock'
if command -v git50 >/dev/null 2>&1; then
    (eval "$PUSH")
else
    bash -ic "$PUSH"
fi || echo "*** push failed; run by hand:  ($PUSH)"
