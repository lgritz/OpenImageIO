#!/bin/bash
# The current "next run" (2026-10-14d): victim-slot memory limit (after mc3)
# and the sweep that skips tiles in use, incl. untiled single-tile images.
# About 35 minutes. Writes results/<host>-mc4/.
# One build (lg-spinlock = phase 1 + 2 + victim microcache + controller).
# The victim cache size is set at run time with
#   OPENIMAGEIO_IMAGECACHE_OPTIONS=microcache_size=N   (N=0 is phase 2 behavior)
# Knobs (env), defaults in brackets:
#   RUN [<host>-mc2]  MC_SIZES ["0 64 256"]  CONFIG_NAMES ["free allcores"]
#   (from free, s0cores, split, allcores)  SYNTH_WL ["7 8"]  PROD_WL ["7 8"]
#   PROD_CACHES ["8192 256"]  STATS_CACHES ["1024 256"]
#   UNTILED_CACHES ["32 128 512"] (16 untiled 16 MB images, --runstats, NCPU threads)
# --runstats at the STATS_CACHES sizes shows redundant reads and held memory.
# Also ctest (texture/imagecache).
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
RUN=${RUN:-$HOST-mc4}
OUT=$TOP/oiio/spinlock-work/results/$RUN
SW=$TOP/spinwork/mc
mkdir -p "$OUT" "$SW"
LOG=$OUT/progress.log
log() { echo "$(date '+%F %T') $*" | tee -a "$LOG"; sync; }
MC_SIZES=${MC_SIZES:-"0 64 256"}
CONFIG_NAMES=${CONFIG_NAMES:-"free allcores"}
SYNTH_WL=${SYNTH_WL:-"7 8"}
PROD_WL=${PROD_WL:-"7 8"}
PROD_CACHES=${PROD_CACHES:-"8192 256"}
STATS_CACHES=${STATS_CACHES:-"1024 256"}
UNTILED_CACHES=${UNTILED_CACHES:-"32 128 512"}

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
want_cfg() { case " $CONFIG_NAMES " in *" $1 "*) return 0;; esac; return 1; }
ALL=( "${CONFIGS[@]}" ); CONFIGS=()
for cfg in "${ALL[@]}"; do want_cfg "${cfg%% *}" && CONFIGS+=("$cfg"); done
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
        for m in $SYNTH_WL; do
            tt "$OUT/tt-w$m-$tag-mc$mc.txt" $mc "$cpus" $GEN \
                --threadtimes $m --threads "$nt" --trials 3
        done
        if [ ${#PRODFILES[@]} -gt 0 ]; then
            for c in $PROD_CACHES; do
                for m in $PROD_WL; do
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
        for c in $STATS_CACHES; do
            tt "$OUT/stats-prod-c$c-w7-free-$NCPU-mc$mc.txt" $mc - \
                --threadtimes 7 --threads "$NCPU" --runstats \
                --cachesize $c --maxfiles 1000 "${PRODFILES[@]}"
        done
    done
    log "END stats"
fi

# ---- untiled single-tile images: held tiles can exceed the cache --------
if [ -n "$UNTILED_CACHES" ]; then
    log "START untiled"
    UD=$SW/untiled
    mkdir -p "$UD"
    for i in $(seq -w 0 15); do
        [ -s "$UD/u$i.exr" ] || "$TOP/build/bin/oiiotool" \
            --pattern fill:top=0.$i,0,0,1:bottom=1,1,1,1 1024x1024 4 \
            -d float --compression none -o "$UD/u$i.exr"
    done
    for mc in $MC_SIZES; do
        for c in $UNTILED_CACHES; do
            tt "$OUT/stats-untiled-c$c-w7-free-$NCPU-mc$mc.txt" $mc - \
                --threadtimes 7 --threads "$NCPU" --trials 1 --iters 200000 \
                --runstats --cachesize $c "$UD"/u*.exr
        done
    done
    log "END untiled"
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
    wls=""
    for m in $SYNTH_WL; do wls="$wls w$m"; done
    for c in $PROD_CACHES; do for m in $PROD_WL; do wls="$wls prod-c$c-w$m"; done; done
    for w in $wls; do
        for cfg in "${CONFIGS[@]}"; do
            read -r name nt cpus <<< "$cfg"
            [ -e "$OUT/tt-$w-$name-$nt-mc${MC_SIZES%% *}.txt" ] || continue
            row="| $w | $name-$nt |"
            for mc in $MC_SIZES; do row="$row $(t "$OUT/tt-$w-$name-$nt-mc$mc.txt" "$nt") |"; done
            echo "$row"
        done
    done
    echo
    echo "## w7 --runstats at $NCPU threads"
    for f in "$OUT"/stats-prod-*.txt "$OUT"/stats-untiled-*.txt; do
        [ -e "$f" ] || continue
        echo; echo "### $(basename "$f" .txt)"; echo '```'
        grep -E 'micro-cache|redundant|Peak cache|victim slots|Sweeps|Tiles:|File I/O|^ *[0-9]+ +[0-9.]+ +[0-9.]+x' "$f"
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
