#!/bin/bash
# The current "next run" (2026-10-13): phase 1 & 2 on a many-core machine.
#   1. <host>-full: synthetic benchmarks (spinbench, spinlock_test,
#      spin_rw_test, parallel_test, testtex workloads, ctest), up to 2x
#      hw threads, plus a single-NUMA-node spinbench if numactl exists.
#   2. <host>-prod-c8192 and <host>-prod-c1024: production textures with
#      an 8 GB / 1 GB cache and --maxfiles 1000.
# Variants: base, spin (phase 1), sweep (phase 1 + 2).
# Then commits ALL results and pushes them with
# `(cd oiio ; git50 push lgritz lg-spinlock)`.
#
# Usage, from TOP (~/code/openimageio-feedstock), after
# `git -C oiio checkout lg-spinlock && git -C oiio pull`:
#     oiio/spinlock-work/remote/next_run.sh /path/to/prodtextures.txt
# Optional: PERF=1 adds perf c2c / perf record to the full run (if perf works).
#
# Safe to rerun: finished pieces are skipped by run_all.sh (via .done
# markers in each RUN dir), so only the missing ones run.
#
# TOP/build and TOP/spinwork are shared with other machines on the same
# filesystem: do not run this on two machines at once.

set -u
TEXLIST=${1:-${TEXLIST:-}}
if [ -z "$TEXLIST" ] || [ ! -f "$TEXLIST" ]; then
    echo "Usage: $0 /path/to/prodtextures.txt   (run from TOP)"
    exit 1
fi
TEXLIST=$(cd "$(dirname "$TEXLIST")" && pwd)/$(basename "$TEXLIST")
if [ ! -x ./build.sh ] || [ ! -e oiio/.git ]; then
    echo "Run this from TOP (the dir containing build.sh and oiio/)."
    exit 1
fi

HOST=$(hostname -s)
VARIANTS="base spin sweep"
RUNS=()
runone() {   # run-name, then env assignments for run_all.sh
    local run=$1; shift
    RUNS+=("$run")
    echo "=== $run  ($(date))"
    env RUN=$run VARIANTS="$VARIANTS" TEXLIST="$TEXLIST" "$@" \
        oiio/spinlock-work/remote/run_all.sh \
        || echo "*** $run exited with an error; continuing"
}

full_steps="survey build ctest spinbench spinlock spinrw parallel testtex"
[ "${PERF:-0}" = 1 ] && full_steps="$full_steps perf"
runone $HOST-full STEPS="$full_steps" PERF=${PERF:-0}
for c in 8192 1024; do
    runone $HOST-prod-c$c STEPS="survey build prodtex" \
        PROD_ARGS="--cachesize $c --maxfiles 1000" PROD_TAG=-c$c
done

echo
echo "=== results:"
for run in "${RUNS[@]}"; do
    d=oiio/spinlock-work/results/$run
    if [ -s "$d/SUMMARY.md" ]; then echo "  OK       $d"; else echo "  MISSING  $d"; fi
done

git -C oiio add spinlock-work/results
if git -C oiio diff --cached --quiet; then
    echo "Nothing new to commit."
else
    git -C oiio commit -q -m "spinlock-work: results from ${RUNS[*]}"
    git -C oiio log --oneline -1
fi
echo
# Push the results. git50 may be an alias or shell function, which a
# non-interactive script can't see, so fall back to an interactive shell.
PUSH='cd oiio ; git50 push lgritz lg-spinlock'
if command -v git50 >/dev/null 2>&1; then
    (eval "$PUSH")
else
    bash -ic "$PUSH"
fi || echo "*** push failed; run by hand:  ($PUSH)"
