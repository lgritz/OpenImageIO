#!/bin/bash
# The current "next run" (2026-10-13b): scaling-ceiling check, to size phase 3.
# About 15-25 minutes. Writes results/<host>-ceiling/:
#   1. scalectl: compute-only control (testtex's ALU loop, no shared data),
#      free and pinned. Its slowdown is the machine's own limit
#      (turbo, SMT, scheduler).
#   2. testtex w1 and w7 (synthetic), plus prod w7 at 8 GB if a list is given,
#      on the lg-spinlock build (= phase 1 + 2), with the same CPU sets.
# CPU sets (from /sys topology): free (no pinning), s0cores (socket 0, one
# thread per core), split (half the cores of each socket, one thread per core),
# allcores (every core, one thread each), s0all (socket 0 incl. SMT siblings).
# Then commits ALL results and pushes with `(cd oiio ; git50 push lgritz lg-spinlock)`.
#
# Usage, from TOP (~/code/openimageio-feedstock), after
# `git -C oiio checkout lg-spinlock && git -C oiio pull`:
#     oiio/spinlock-work/remote/next_run.sh [/path/to/prodtextures.txt]
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
RUN=$HOST-ceiling
OUT=$TOP/oiio/spinlock-work/results/$RUN
SW=$TOP/spinwork/ceiling
mkdir -p "$OUT" "$SW"
LOG=$OUT/progress.log
log() { echo "$(date '+%F %T') $*" | tee -a "$LOG"; sync; }

if ! command -v taskset >/dev/null 2>&1; then
    echo "taskset not found (util-linux); pinned configs will be skipped." | tee -a "$LOG"
fi

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
S0CORES=$(awk '$2==0 && $1==$3 {print $1}' "$topo" | paste -sd, -)
S0ALL=$(awk '$2==0 {print $1}' "$topo" | paste -sd, -)
ALLCORES=$(awk '$1==$3 {print $1}' "$topo" | paste -sd, -)
# Half the cores of each socket.
SPLIT=$(awk '$1==$3 {c[$2]++; if (c[$2] <= h) print $1}' \
        h=$(( NCORES / 2 / $(awk '{print $2}' "$topo" | sort -u | wc -l) )) "$topo" | paste -sd, -)
count() { echo "$1" | tr , '\n' | wc -l; }
{
    echo "NCPU=$NCPU NCORES=$NCORES"
    echo "s0cores  ($(count "$S0CORES")): $S0CORES"
    echo "split    ($(count "$SPLIT")): $SPLIT"
    echo "allcores ($(count "$ALLCORES")): $ALLCORES"
    echo "s0all    ($(count "$S0ALL")): $S0ALL"
} | tee "$OUT/cpusets.txt"

# config name, thread count, cpu list ("" = free)
CONFIGS=(
    "free 1 -"
    "free 16 -"
    "s0cores $(count "$S0CORES") $S0CORES"
    "split $(count "$SPLIT") $SPLIT"
    "free 24 -"
    "free 32 -"
    "allcores $(count "$ALLCORES") $ALLCORES"
    "s0all $(count "$S0ALL") $S0ALL"
    "free $NCPU -"
)
pin() {   # cpulist, then command
    local cpus=$1; shift
    if [ "$cpus" = - ]; then "$@"
    elif command -v taskset >/dev/null 2>&1; then taskset -c "$cpus" "$@"
    else echo "(skipped: no taskset)"; fi
}

# ---- build ------------------------------------------------------------
log "START build lg-spinlock $(git -C oiio rev-parse --short HEAD)"
( EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS:-} -DOIIO_BUILD_TESTS=ON" ./build.sh ) \
    > "$SW/build.log" 2>&1 || { log "build FAILED; see $SW/build.log"; tail -30 "$SW/build.log"; exit 1; }
tail -5 "$SW/build.log" > "$OUT/build.tail"
log "END build"
TESTTEX=$TOP/build/bin/testtex
${CXX:-c++} -O2 -std=c++17 -pthread oiio/spinlock-work/bench/scalectl.cpp \
    -o "$SW/scalectl" 2> "$OUT/scalectl-compile.txt" || { log "scalectl compile FAILED"; exit 1; }

PRODFILES=()
if [ -n "$TEXLIST" ]; then
    while read -r f; do [ -n "$f" ] && PRODFILES+=("$f"); done < "$TEXLIST"
fi

# ---- runs -------------------------------------------------------------
cd "$SW"
for cfg in "${CONFIGS[@]}"; do
    read -r name nt cpus <<< "$cfg"
    tag=$name-$nt
    log "START $tag"
    pin "$cpus" ./scalectl 2000000 "$nt" > "$OUT/scalectl-$tag.txt" 2>&1
    for m in 1 7; do
        pin "$cpus" "$TESTTEX" --maketests 10 'test{:04}.exr' --maketest-res 1024 \
            --threadtimes $m --threads "$nt" --trials 3 \
            > "$OUT/testtex-w$m-$tag.txt" 2>&1
    done
    if [ ${#PRODFILES[@]} -gt 0 ]; then
        pin "$cpus" "$TESTTEX" --threadtimes 7 --threads "$nt" --trials 3 \
            --cachesize 8192 --maxfiles 1000 "${PRODFILES[@]}" \
            > "$OUT/testtex-prod-w7-$tag.txt" 2>&1
    fi
    log "END $tag"
done
cd "$TOP"
if [ ${#PRODFILES[@]} -gt 0 ] && [ "${SCRUB_NAMES:-1}" = 1 ]; then
    printf '%s\n' "${PRODFILES[@]}" > "$SW/prodfiles.lst"
    python3 -I oiio/spinlock-work/remote/scrub.py "$SW/prodfiles.lst" "$OUT"/testtex-prod-*.txt
fi

# ---- summary: seconds (best of 3) per config ---------------------------
{
    echo "| config | threads | scalectl | w1 | w7 | prod w7 |"
    echo "|---|---|---|---|---|---|"
    for cfg in "${CONFIGS[@]}"; do
        read -r name nt cpus <<< "$cfg"
        tag=$name-$nt
        s=$(awk 'NR==2 {print $2}' "$OUT/scalectl-$tag.txt" 2>/dev/null)
        t() { awk -v n="$nt" '$1==n && $2 ~ /^[0-9.]+$/ {print $2; exit}' "$1" 2>/dev/null; }
        echo "| $name | $nt | ${s:--} | $(t "$OUT/testtex-w1-$tag.txt") | $(t "$OUT/testtex-w7-$tag.txt") | $(t "$OUT/testtex-prod-w7-$tag.txt") |"
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
