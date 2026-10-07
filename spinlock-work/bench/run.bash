#!/bin/bash
# Spin lock benchmark driver: one (binary, variant, threads, shape) per process.
# Logs START/END to $LOG with sync, so a crash leaves the culprit on disk.
#
# Usage: run.bash <ops> <trials> <outfile> [variant-regex] [threads...]
# Env:   BINDIR  dir holding the spinbench binaries (default: this script's dir)
#        BINS    binaries to run (default: "spinbench17 spinbench20")
#        SHAPES  critical-section shapes (default: "tiny medium"; also "coloc")
#        LOG     progress log (default: ./bench.log)
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
BINDIR=${BINDIR:-$HERE}
OPS=$1; TRIALS=$2; OUT=$3; RE=${4:-.}
shift 4 2>/dev/null || shift $#
THREADS=${*:-1 2 4 8 12 16 32}
LOG=${LOG:-bench.log}
NCPU=$(nproc 2>/dev/null || sysctl -n hw.ncpu)

if grep -nE '\.wait\(|notify_' "$HERE/spinbench.cpp"; then
    echo "SAFETY: wait/notify found in source, aborting"; exit 1
fi

log() { echo "$(date +%H:%M:%S) $*" >> "$LOG"; sync; }

for bin in ${BINS:-spinbench17 spinbench20}; do
    [ -x "$BINDIR/$bin" ] || { echo "missing $BINDIR/$bin"; continue; }
    for v in $("$BINDIR/$bin" list | grep -E "$RE"); do
        log "START $bin unc $v"
        r=$("$BINDIR/$bin" unc $v)
        echo "$bin unc $v $r" >> "$OUT"
        log "END"
        [ "$v" = H_stdmutex ] && [ "$(uname)" = Darwin ] && continue
        for shape in ${SHAPES:-tiny medium}; do
            for nt in $THREADS; do
                case $v in D_article|D2_article_yldinsn) [ $nt -gt $NCPU ] && continue;; esac
                log "START $bin con $v $nt $shape"
                r=$("$BINDIR/$bin" con $v $nt $shape $OPS $TRIALS)
                rc=$?
                echo "$bin con $v $nt $shape $r" >> "$OUT"
                log "END rc=$rc"
            done
        done
    done
done
log "ALL DONE $OUT"
