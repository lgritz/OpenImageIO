#!/bin/bash
# The current "next run" (2026-10-19): spin_mutex backoff cap. On the M4,
# atomic_backoff(4096, 2) instead of (128, 64) fixes the 16T regression and
# helps 8-12T. Check x86 / 2 sockets. About 25 minutes. Writes
# results/<host>-backoff/.
# Needs TOP/build (from ./build.sh) only for libOpenImageIO_Util and the
# generated headers; the benchmarks compile against thread.h copies in
# spinlock-work/bench/backoff/ (before = pre-PR, main = merged PR).
# Knobs (env), defaults in brackets:
#   RUN [<host>-backoff]  VARIANTS ["128_64 4096_2 2048_4 4096_8"] (P_S for
#   spin_mutex's atomic_backoff)  BENCH_VARIANTS [see below]
#   THREADS ["1 2 4 8 16 32 64"]  PINNED ["s0cores split"]  ITERS [50000000]
#   QUICK [0]  NOPUSH [0]
#
# Usage, from TOP (~/code/openimageio-feedstock), after
# `git -C oiio checkout lg-spinlock && git -C oiio pull`:
#     oiio/spinlock-work/remote/next_run.sh
# Safe to rerun: finished pieces (non-empty result files) are skipped.
# TOP/build is shared with other machines: do not run on two at once.

set -u
if [ ! -x ./build.sh ] || [ ! -e oiio/.git ]; then
    echo "Run this from TOP (the dir containing build.sh and oiio/)."
    exit 1
fi
TOP=$(pwd)
HOST=$(hostname -s)
RUN=${RUN:-$HOST-backoff}
OUT=$TOP/oiio/spinlock-work/results/$RUN
SW=$TOP/spinwork/backoff
BSRC=$TOP/oiio/spinlock-work/bench/backoff
mkdir -p "$OUT" "$SW"
LOG=$OUT/progress.log
log() { echo "$(date '+%F %T') $*" | tee -a "$LOG"; sync; }
QUICK=${QUICK:-0}
NOPUSH=${NOPUSH:-0}
VARIANTS=${VARIANTS:-"128_64 4096_2 2048_4 4096_8"}
BENCH_VARIANTS=${BENCH_VARIANTS:-"before:oiio after:oiio 128:64 128:0 1024:8 2048:4 4096:2 4096:8 16384:2"}
THREADS=${THREADS:-"1 2 4 8 16 32 64"}
PINNED=${PINNED:-"s0cores split"}
ITERS=${ITERS:-50000000}
LIGHT_OPS=8000000
if [ "$QUICK" = 1 ]; then
    ITERS=200000; LIGHT_OPS=20000; THREADS="1 4"; VARIANTS="128_64 4096_2"
    BENCH_VARIANTS="after:oiio 4096:2"
fi

HAVE_TASKSET=0
command -v taskset >/dev/null 2>&1 && HAVE_TASKSET=1

# ---- topology -> CPU sets ----------------------------------------------
NCPU=$(nproc 2>/dev/null || sysctl -n hw.ncpu)
S0CORES=- SPLIT=- NPKG=1
if [ -e /sys/devices/system/cpu/cpu0/topology/physical_package_id ]; then
    topo=$SW/topo.txt
    for c in /sys/devices/system/cpu/cpu[0-9]*; do
        n=${c##*cpu}
        pkg=$(cat "$c/topology/physical_package_id")
        first=$(sed 's/[,-].*//' "$c/topology/thread_siblings_list")
        echo "$n $pkg $first"
    done | sort -n > "$topo"
    cp "$topo" "$OUT/topology.txt"
    NCORES=$(awk '$1==$3' "$topo" | wc -l)
    NPKG=$(awk '{print $2}' "$topo" | sort -u | wc -l)
    S0CORES=$(awk '$2==0 && $1==$3 {print $1}' "$topo" | paste -sd, -)
    SPLIT=$(awk '$1==$3 {c[$2]++; if (c[$2] <= h) print $1}' \
            h=$(( NCORES / 2 / NPKG )) "$topo" | paste -sd, -)
fi
{ uname -a; lscpu 2>/dev/null; } > "$OUT/machine.txt"
count() { echo "$1" | tr , '\n' | wc -l; }
CONFIGS=()
for t in $THREADS; do [ "$t" -le "$NCPU" ] && CONFIGS+=("free-$t $t -"); done
if [ $HAVE_TASKSET = 1 ] && [ "$S0CORES" != - ]; then
    case " $PINNED " in *" s0cores "*)
        CONFIGS+=("s0cores-$(count "$S0CORES") $(count "$S0CORES") $S0CORES");; esac
    if [ "$NPKG" -gt 1 ]; then
        case " $PINNED " in *" split "*)
            CONFIGS+=("split-$(count "$SPLIT") $(count "$SPLIT") $SPLIT");; esac
    fi
fi
printf '%s\n' "${CONFIGS[@]}" | tee "$OUT/configs.txt"
pin() { local cpus=$1; shift
        if [ "$cpus" = - ]; then "$@"; else taskset -c "$cpus" "$@"; fi; }

# ---- build ----------------------------------------------------------------
log "START build lg-spinlock $(git -C oiio rev-parse --short HEAD)"
( EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS:-} -DOIIO_BUILD_TESTS=ON" ./build.sh ) \
    > "$SW/build.log" 2>&1 || { log "build FAILED; see $SW/build.log"; tail -30 "$SW/build.log"; exit 1; }
tail -5 "$SW/build.log" > "$OUT/build.tail"
git -C oiio log -1 --format='%H %s' > "$OUT/git.txt"
UTILLIB=$(find "$TOP/build/" \( -name 'libOpenImageIO_Util.so*' -o -name 'libOpenImageIO_Util*.dylib' \) | head -1)
[ -n "$UTILLIB" ] || { log "libOpenImageIO_Util not found under $TOP/build"; exit 1; }
LIBDIR=$(dirname "$UTILLIB")
# Use the include flags the build itself used for spinlock_test (deps like
# Imath live in different places on different machines).
INCS=$(python3 -I - "$TOP/build" <<'EOF'
import json, os, re, shlex, sys
b = sys.argv[1]
def flags(words):
    out, it = [], iter(words)
    for w in it:
        if w in ('-I', '-isystem'):
            out += [w, next(it, '')]
        elif w.startswith(('-I', '-isystem')):
            out.append(w)
    return out
cc = os.path.join(b, 'compile_commands.json')
if os.path.exists(cc):
    for e in json.load(open(cc)):
        if e['file'].endswith('spinlock_test.cpp'):
            print(' '.join(flags(e.get('arguments') or shlex.split(e['command']))))
            sys.exit(0)
fm = os.path.join(b, 'src/libutil/CMakeFiles/spinlock_test.dir/flags.make')
if os.path.exists(fm):
    for l in open(fm):
        if l.startswith('CXX_INCLUDES'):
            print(' '.join(flags(shlex.split(l.split('=', 1)[1]))))
            sys.exit(0)
nj = os.path.join(b, 'build.ninja')
if os.path.exists(nj):
    txt = open(nj).read()
    m = re.search(r'spinlock_test\.cpp\.o:.*?\n((?:  .*\n)+)', txt)
    if m:
        for l in m.group(1).splitlines():
            if l.strip().startswith('INCLUDES'):
                print(' '.join(flags(shlex.split(l.split('=', 1)[1]))))
                sys.exit(0)
sys.exit(1)
EOF
) || { log "could not find include flags in $TOP/build"; exit 1; }
echo "$INCS" > "$SW/includes.txt"
CXX=${CXX:-c++}
cc1() {   # out, thread.h, source
    local out=$1 hdr=$2 src=$3 d=$SW/inc-$(basename "$1")
    mkdir -p "$d/OpenImageIO"; cp "$hdr" "$d/OpenImageIO/thread.h"
    # shellcheck disable=SC2086
    $CXX -O3 -DNDEBUG -DOIIO_INTERNAL=1 -std=c++17 -pthread -I"$d" $INCS "$src" \
        -L"$LIBDIR" -lOpenImageIO_Util -Wl,-rpath,"$LIBDIR" -o "$out" \
        >> "$SW/compile.log" 2>&1 || { log "compile FAILED $out; see $SW/compile.log"; tail -20 "$SW/compile.log"; exit 1; }
}
log "START compile"
: > "$SW/compile.log"
cc1 "$SW/spin_before" "$BSRC/thread_before.h" "$BSRC/spinlock_test.cpp"
for v in $VARIANTS; do
    p=${v%_*}; s=${v#*_}
    sed "s/atomic_backoff backoff(128, 64);/atomic_backoff backoff($p, $s);/" \
        "$BSRC/thread_main.h" > "$SW/thread_$v.h"
    grep -q "backoff($p, $s);" "$SW/thread_$v.h" || { log "sed FAILED for $v"; exit 1; }
    cc1 "$SW/spin_$v" "$SW/thread_$v.h" "$BSRC/spinlock_test.cpp"
done
cc1 "$SW/bench_before" "$BSRC/thread_before.h" "$BSRC/backoff_bench.cpp"
cc1 "$SW/bench_after" "$BSRC/thread_main.h" "$BSRC/backoff_bench.cpp"
cc1 "$SW/pausecost" "$BSRC/thread_main.h" "$BSRC/pausecost.cpp"
log "END compile"
[ -s "$OUT/pausecost.txt" ] || "$SW/pausecost" > "$OUT/pausecost.txt" 2>&1

# ---- spinlock_test with each spin_mutex backoff --------------------------
for cfg in "${CONFIGS[@]}"; do
    read -r name nt cpus <<< "$cfg"
    for v in before $VARIANTS; do
        f=$OUT/spinlock-$v-$name.txt
        [ -s "$f" ] && continue
        log "START spinlock $v $name"
        timeout 600 pin "$cpus" "$SW/spin_$v" --threads "$nt" --trials 3 \
            --iters "$ITERS" > "$f" 2>&1 || echo "*** exit $?" >> "$f"
    done
done
log "END spinlock"

# ---- backoff bench: hammer and light workloads ---------------------------
for wl in hammer light; do
    ops=$ITERS; [ $wl = light ] && ops=$LIGHT_OPS
    for cfg in "${CONFIGS[@]}"; do
        read -r name nt cpus <<< "$cfg"
        for v in $BENCH_VARIANTS; do
            f=$OUT/bench-$wl-${v//:/_}-$name.txt
            [ -s "$f" ] && continue
            bin=$SW/bench_after var=$v
            case $v in before:oiio) bin=$SW/bench_before; var=oiio;; after:oiio) var=oiio;; esac
            log "START bench $wl $v $name"
            timeout 300 pin "$cpus" "$bin" "$var" "$nt" "$wl" "$ops" 3 > "$f" 2>&1 \
                || echo "*** exit $?" >> "$f"
        done
    done
done
log "END bench"

# ---- summary ------------------------------------------------------------
{
    echo "## pause cost"; echo '```'; cat "$OUT/pausecost.txt"; echo '```'; echo
    echo "## spinlock_test seconds (best of 3, $ITERS ops; range in parens)"; echo
    hdr="| config |"; sep="|---|"
    for v in before $VARIANTS; do hdr="$hdr $v |"; sep="$sep---|"; done
    echo "$hdr"; echo "$sep"
    for cfg in "${CONFIGS[@]}"; do
        read -r name nt cpus <<< "$cfg"
        row="| $name |"
        for v in before $VARIANTS; do
            r=$(awk -v n="$nt" '$1==n && $2 ~ /^[0-9.]+$/ {print $2" ("$6")"; exit}' \
                "$OUT/spinlock-$v-$name.txt" 2>/dev/null)
            row="$row $r |"
        done
        echo "$row"
    done
    for wl in hammer light; do
        echo; echo "## bench $wl: ns/op (best of 3) / mean contended wait us"; echo
        hdr="| config |"; sep="|---|"
        for v in $BENCH_VARIANTS; do hdr="$hdr $v |"; sep="$sep---|"; done
        echo "$hdr"; echo "$sep"
        for cfg in "${CONFIGS[@]}"; do
            read -r name nt cpus <<< "$cfg"
            row="| $name |"
            for v in $BENCH_VARIANTS; do
                r=$(awk '{for (i=1;i<=NF;i++) { if ($i=="ns/op") ns=$(i-1); if ($i=="wait") w=$(i+1) }}
                         END {print ns" / "w}' "$OUT/bench-$wl-${v//:/_}-$name.txt" 2>/dev/null)
                row="$row $r |"
            done
            echo "$row"
        done
    done
} > "$OUT/SUMMARY.md"
cat "$OUT/SUMMARY.md"

# ---- commit + push ----------------------------------------------------
git -C oiio add spinlock-work/results/"$RUN"
if git -C oiio diff --cached --quiet; then
    echo "Nothing new to commit."
else
    git -C oiio commit -q -m "spinlock-work: results from $RUN"
    git -C oiio log --oneline -1
fi
[ "$NOPUSH" = 1 ] && { echo "NOPUSH=1: not pushing."; exit 0; }
echo
# git50 may be an alias or shell function, which a non-interactive script
# can't see, so fall back to an interactive shell.
PUSH='cd oiio ; git50 push lgritz lg-spinlock'
if command -v git50 >/dev/null 2>&1; then
    (eval "$PUSH")
else
    bash -ic "$PUSH"
fi || echo "*** push failed; run by hand:  ($PUSH)"
