#!/bin/bash
# The current "next run": production textures at three cache sizes,
# base vs sweep only, --maxfiles 1000 (no reopens). Then commits ALL
# results and pushes them with `(cd oiio ; git50 push lgritz lg-spinlock)`.
#
# Usage, from TOP (~/code/openimageio-feedstock), after
# `git -C oiio checkout lg-spinlock && git -C oiio pull`:
#     oiio/spinlock-work/remote/next_run.sh /path/to/prodtextures.txt
#
# Safe to rerun: a config whose RUN dir already finished is skipped by
# run_all.sh (via its .done markers), so only the missing pieces run.

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
RUNS=()
for c in 8192 1024 256; do
    run=$HOST-prod-c$c
    RUNS+=("$run")
    echo "=== $run"
    RUN=$run VARIANTS="base sweep" STEPS="survey build prodtex" \
        PROD_ARGS="--cachesize $c --maxfiles 1000" PROD_TAG=-c$c \
        TEXLIST="$TEXLIST" oiio/spinlock-work/remote/run_all.sh \
        || echo "*** $run exited with an error; continuing"
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
