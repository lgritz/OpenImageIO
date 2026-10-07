> Note (2026-10-07): written for a coding agent. The remote machine cannot
> run one, so this plan is automated by `remote/run_all.sh` (see README.md).
> The H1 fix (tile use) is now commit 114a92be0, H2 is fdf355203, and the H5
> file-sweep std::mutex is 7624b8e90. This file remains the reference for
> the hypotheses and how to interpret the results.

# Task: validate OIIO spin_mutex changes on x86_64 at high core counts, and attack TextureSystem contention

You are working on OpenImageIO (OIIO) on a Linux x86_64 machine with 64 or
more cores. The work so far was done on an Apple M4 Max (arm64, 16 cores:
12 performance + 4 efficiency). Your job:

1. Verify that the committed spin_mutex change is correct and fast on x86_64.
2. Measure how it behaves with many more threads and cores (64-256+, SMT,
   multiple sockets/NUMA).
3. Reported problem: **performance, especially TextureSystem, gets very bad
   beyond 16-32 threads.** Find out why, and try fixes that reduce thread
   contention. Measure every fix.
4. Evaluate where OIIO uses `spin_mutex` but `std::mutex` would be better
   (initial list below).

Everything you need is in this folder (`spinlock-work/`), plus the
`lg-spinlock` branch of the OIIO repo.

---

## 0. Ground rules (read first)

### Crash safety and the STATUS.md protocol
An earlier attempt on macOS **kernel-panicked the machine twice**. The cause
was a lock built on C++20 `std::atomic_flag::wait()/notify_one()` under
16-thread contention: macOS's `__ulock_wait/__ulock_wake` hit an XNU
spinlock-timeout panic. Linux uses futexes, which are far more mature, but
be careful anyway. On a 64+ core box, a bad experiment can also starve the
whole machine.

- Before you run **anything**, create `STATUS.md` at the repo root and keep
  it current. Record: this task (link to this README), the machine survey,
  a checklist, and a run log. **Before every benchmark or test run,** append
  a line saying exactly what you are about to run, then `sync` (the page
  cache is lost on a crash). After the run, record the result. If the
  machine dies, the last "running X" line with no result names the culprit.
- `run.bash` also writes `bench.log` with START/END lines plus `sync`, one
  process per (variant, threads, shape).
- Every bench process calls `alarm(300)`, so it kills itself if it hangs.
- No-yield spin variants are refused above `hardware_concurrency()`
  threads. Yielding variants are capped at 4x hardware threads.
- Never run benchmarks while a build or a ctest run is going.
- If the machine is shared with other users or jobs, ask the user before
  running long all-core benchmarks.
- Commit messages: terse; a body only when the change needs a "why". End
  with exactly one attribution line, `Assisted-by: <tool> / <model>`. Never
  add `Co-Authored-By` or session IDs. Do not touch CHANGES.md. Do not push
  unless the user says so.

### Survey the machine first and record it in STATUS.md
```
lscpu; nproc; numactl -H; cat /sys/devices/system/cpu/smt/active
uname -a; c++ --version; cmake --version
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null
```
Note: physical cores vs hardware threads (SMT), sockets/NUMA nodes, CPU
model (Intel Skylake-SP and later have a ~140-cycle `pause`; older Intel
has ~10 cycles; AMD differs again), and whether turbo or the governor will
add noise.

---

## 1. What has been done (commit `bad4c87e4` on branch `lg-spinlock`)

Parent (baseline) commit: `2a58fd4f3`. The only file changed is
`src/include/OpenImageIO/thread.h`. Read the commit message
(`git show bad4c87e4`). Summary:

1. `pause()`: added an `__aarch64__` branch (`isb`). Before, arm64 got an
   empty loop. x86 is unchanged: it was already `pause`.
2. `OIIO_PAUSE_NOUNROLL`: clang -O3 fully unrolled `pause(128)`, and the
   bloated `lock()` stopped being inlined.
3. `spin_mutex::lock()`'s branch hint was inverted
   (`!OIIO_UNLIKELY(try_lock())`). That was worth up to 2x at 16-32 threads.
4. `spin_mutex` now uses `std::atomic<bool>` instead of `atomic_flag`. The
   wait loop is now a legal `load(relaxed)` (test-and-test-and-set) instead
   of a volatile-cast read. That cast was UB and is why TSan needed
   `OIIO_THREAD_ALLOW_DCLP=0`. Note that on MSVC, `sizeof(spin_mutex)`
   changes from 4 to 1.
5. `atomic_backoff(pausemax, spins = 0)`: pauses double up to `pausemax`,
   then `spins` more rounds at `pausemax`, then `yield()` every call. The
   default keeps the old behavior. `spin_mutex` now uses
   `atomic_backoff(128, 64)`. Old: `(16)`, i.e. 1, 2, 4, 8, 16 pauses and
   then yield.

Key finding on M4: **yielding early was the dominant cost at 8-12
threads.** The C++20 options were evaluated. `atomic_flag::test()` gives
nothing over `atomic<bool>::load()`. `wait()/notify()` is about 4x slower
uncontended and is what panicked macOS.

M4 Max results, real `spinlock_test` at -O3, wall ns/op (CPU ns/op):

| threads | 4 | 8 | 12 | 16 | 32 (2x oversubscribed) |
|---|---|---|---|---|---|
| old | 3.8 (21) | 26.2 (216) | 18.8 (222) | 5.0 (62) | 2.5 (41) |
| new | 1.2 (9) | 2.5 (23) | 3.8 (40) | 5.0 (78) | 6.2 (95) |

The old code wins only when oversubscribed with an empty critical section.
It gets there by starvation: early yields let one thread run alone.
`testtex --threadtimes` was unchanged within noise by this commit, so
ImageCache is not limited by spin_mutex (see section 5). Full M4 data:
`results-m4max/` (`full.txt`, `round2-4.txt`, `STATUS-m4max.md`).

**Open question for x86:** the constants 128/64 were tuned where one pause
(`isb`) costs about 8 ns. On x86, `pause` costs roughly 3-40 ns depending
on the microarchitecture, so the spin budget before yielding varies 10x
across chips. Measure it. Arch-specific constants (`#if` on
`__x86_64__`/`__aarch64__`) are acceptable if the data calls for them.

---

## 2. Build

Use two separate trees so A/B comparisons are clean:
```
git fetch && git checkout lg-spinlock          # contains bad4c87e4
git worktree add ../oiio-base 2a58fd4f3        # baseline
# Configure each the way this machine normally builds OIIO, Release, e.g.:
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake -S ../oiio-base -B ../oiio-base/build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target spinlock_test spin_rw_test thread_test ustring_test parallel_test testtex -j <N>
(same for ../oiio-base/build)
```
Run cmake from the repo root with `-S/-B`, or from inside the build dir.
Running `cmake .` in the source dir triggers an in-source build error.

Correctness, in the new tree:
```
cd build && ctest -R 'unit_spinlock|unit_spin_rw|unit_ustring|unit_thread|unit_parallel|texture-' --timeout 900
```
(The `texture-texture3d` tests need the `openvdb` test's fixture dir. If
they fail with "sphere.vdb does not exist", run the full suite or include
`openvdb` in the regex.) Optionally build a TSan configuration and run
`unit_spinlock` and `unit_thread`. With the new code it should be clean
without `OIIO_THREAD_ALLOW_DCLP=0`.

---

## 3. Microbenchmark: `spinbench.cpp`

A standalone lock benchmark. Variant names are listed by
`./spinbench17 list`:

- `A_cur`: old algorithm (atomic_flag, old backoff). Note: this copy uses
  the *correct* branch hint.
- `A2_cur_nodclp`: old algorithm without the DCLP spin.
- `B*`: atomic_flag with a fixed pause.
- `C`: atomic<bool> TTAS with the old backoff.
- `D_article`: the exponential backoff from
  https://david.alvarezrosa.com/posts/optimizing-a-spin-lock/ (cap 64,
  never yields).
- `E*` / `X*`: atomic<bool> TTAS with `Backoff<Pause, Cap, SpinsBeforeYield>`.
  `X_bool_isb_128y64` is what was committed. "isb" means `_mm_pause` on x86.
- `F*`: C++20 `atomic_flag::test()` versions (only in the -std=c++20 build).
- `O_oiio_spin_mutex` / `O2_*`: the real `OIIO::spin_mutex` from the header
  (needs `-DWITH_OIIO`).
- `H_stdmutex`: uncontended only on macOS. On Linux it is also allowed
  contended (futex).

Shapes:
- `tiny`: increment a counter on its own cache line.
- `medium`: sin/fmod work inside and outside the lock. This is the most
  realistic shape.
- `coloc`: the counter shares the lock's cache line, as in `spinlock_test`
  and most real code.

Environment variable `NOBARRIER=1` starts workers immediately, with no
start barrier, as `timed_thread_wedge` does.

```
cd spinlock-work/bench
NEW=/path/to/oiio   # repo root of the lg-spinlock checkout, with build/ configured
c++ -std=c++17 -O3 -fno-math-errno -DNDEBUG -pthread -DWITH_OIIO \
    -I$NEW/src/include -I$NEW/build/include spinbench.cpp -o spinbench17o3
c++ -std=c++17 -O2 -pthread spinbench.cpp -o spinbench17
c++ -std=c++20 -O2 -pthread spinbench.cpp -o spinbench20
./spinbench17 pausecost          # ns per pause instruction: record this!
# smoke first:
BINS=spinbench17o3 SHAPES="tiny" ./run.bash 200000 1 smoke.txt '^(A_cur|X_bool_isb_128y64|O_oiio)' 1 4 16
# then the real run (pick thread counts up to ~2x hw threads):
BINS=spinbench17o3 SHAPES="tiny medium coloc" ./run.bash 4000000 5 x86-r1.txt \
   '^(A_cur|B_flag_isb_16y1|D_article$|E_bool_isb_64y64|X_|O_oiio)' 2 4 8 16 32 48 64 96 128 192 256
python3 table.py x86-r1.txt spinbench17o3
```
(`run.bash` usage: `<ops> <trials> <outfile> [variant-regex] [threads...]`.
Env vars: `BINS`, `SHAPES`. The bench's built-in cap on total ops is 200M.)

Things to determine:
- Does the committed `X_bool_isb_128y64` / `O_oiio_spin_mutex` beat `A_cur`
  at every count up to the core count, in wall time *and* CPU time?
- Best `Cap`/`spins` for x86 (the 128 cap mattered most on M4). Add
  variants by adding `using` lines plus an `F(...)` line in
  `BASE_VARIANTS`. Keep the safety checks.
- SMT: compare thread counts equal to the physical cores vs all hardware
  threads. `pause` matters more with SMT.
- NUMA: `numactl --cpunodebind=0 --membind=0` vs the whole machine. A lock
  line bouncing between sockets costs much more.
- Oversubscription (threads > hw threads): how bad is the new code vs old?
  If it's bad, consider a smarter policy, for example a spin budget scaled
  down when contention is extreme, or a *time*-based budget instead of a
  pause count. Pause cost varies 10x across x86 parts.
- C++20 `atomic::wait/notify` (futex) as a fallback after spinning is safe
  to *try* on Linux, if you want: a hybrid spin-then-park lock. It was
  never benchmarked on Linux. Keep it out of anything that might run on
  macOS, or guard it with `#if !defined(__APPLE__)`.

---

## 4. Real `spinlock_test` A/B (old vs new)

```
for n in 1 2 4 8 16 32 64 128 <hwthreads>; do
  for t in ../oiio-base/build build; do
    /usr/bin/time -f "%e s wall %U user %S sys" $t/bin/spinlock_test --threads $n --iters 160000000 --trials 3 2>&1 | tail -2
  done
done
```
(The printed time has only 0.1 s resolution, hence the large `--iters`.
`--wedge` runs a fixed list of thread counts.)

Note: `timed_thread_wedge` has no start barrier, and in `spinlock_test` the
lock shares a cache line with the counter.

---

## 5. TextureSystem scaling: the main concern

Workloads (`testtex --threadtimes N`):
1. Everybody accesses the same spot in one file (handles)
2. Everybody accesses the same spot in one file
3. Coherent access, one file, each thread in similar spots
4. Coherent access, one file, each thread in different spots
5. Coherent access, many files, each thread in similar spots
6. Coherent access, many files, each thread in different spots
7. Coherent access, many files, partially overlapping texture sets
8. Same as 7, with no extra busy work

The wedge is **weak scaling** (fixed iterations per thread), so ideal time
stays flat as threads increase. It covers thread counts 1, 2, 4, 8, 12, 16,
24, 32, 64, 128 up to `--threads` (default = hw threads).
```
cd build/testsuite/texture-threadtimes   # or any scratch dir
T=../../bin/testtex
$T -t 0 --maketests 10 "test{:04}.exr" --maketest-res 1024 --threadtimes 1 --wedge --runstats
# vary: --threadtimes 1..8, --handle, --cachesize MB, --maxfiles N, --trials 3
```
Do old tree vs new tree first. Then profile the worst workload at high
thread counts:
```
perf stat -e cycles,instructions,cache-misses,LLC-load-misses $T ...
perf record -g $T ... ; perf report
perf c2c record $T ... ; perf c2c report   # *** finds false/true sharing on cache lines ***
```

### Hypotheses, ranked by expected impact. Test one at a time, A/B each.

**H1 (measured on M4, strong): `ImageCacheTile::use()` writes a shared
cache line on every texture lookup.** `src/libtexture/imagecache_pvt.h:802`
is `void use() { m_used = 1; }`, a **seq_cst** store to an `atomic_int`
(a locked `xchg` on x86, a full barrier). It is called from `find_tile()`
(`imagecache_pvt.h:1164, 1172`), which `texturesys.cpp` calls with
`sample == 0`, i.e. about once per lookup. `m_used` sits in the same
object, and likely the same cache line, as the fields every lookup reads:
the refcount, `m_id`, the pixels pointer and the sizes. So threads sampling
the same hot tile bounce that line on every lookup.
The fix (test, then a relaxed store) is commit `114a92be0`. On M4, weak-scaling seconds went:
workload 1, 8 threads 1.37-1.40 -> 1.06-1.14 and 16 threads 2.9-4.8 ->
2.2-2.3; workload 2, 16 threads 2.7-3.3 -> 2.2; workload 4 2.6 -> 2.2;
workload 7 unchanged. Expect a much bigger effect at 64+ cores. Verify with
`perf c2c` before and after.

**H2: `ImageCacheFile::use()` (`imagecache.cpp:1688`, in `find_file`)**
does a plain, non-atomic `bool` store into the shared file object on every
by-name lookup. That is the same line-bouncing problem, and also a
technical data race. Try the same test-before-write (make it
`std::atomic<bool>` with relaxed ops). It shows up with name lookups
(workloads 2-8), not with `--handle`.

**H3: `spin_rw_mutex` read locks do `fetch_add` on a shared word.** Every
main-cache tile lookup (`m_tilecache.retrieve`, `imagecache.cpp:2842`)
takes a bin read lock. With `TILE_CACHE_SHARDS` = 128 bins
(`imagecache_pvt.h:36`), all threads hitting the *same* hot tile hit the
same bin, so readers serialize on that line even with no writers. This
only happens on a per-thread microcache miss, and the microcache holds
just **2 tiles** (`thread_info->tile`, `lasttile`). Options:
- a larger per-thread microcache (for example 4-16 entries,
  direct-mapped), probably the biggest win if microcache misses are
  frequent (see the `--runstats` output: find_tile_microcache_misses vs
  find_tile_calls);
- more shards;
- a read path that doesn't write shared memory, e.g. a per-bin seqlock or
  epoch/RCU-style reads. Harder; only if data shows it matters.

**H4: refcount traffic.** `ImageCacheTileRef` is an intrusive_ptr, so
each microcache miss copies the ref and does atomic inc/dec on the shared
tile's refcount, the same line as H1. A bigger microcache (H3) also
reduces this.

**H5: `m_file_sweep_mutex`** (`imagecache_pvt.h:1346`, used at
`imagecache.cpp:1738-1799`). When far over `max_open_files`, every thread
calls `.lock()` unconditionally. The holder closes files (syscalls) for up
to 100 passes over the file table, while dozens of threads spin on a
spin_mutex. Test with a low `--maxfiles` (for example 16-64) and many
files at high thread counts. Fix: make it a `std::mutex`. The `try_lock()`
fast path is unaffected.

**H6: `ImageCacheFile::m_input_mutex`** (`recursive_timed_mutex`,
`imagecache_pvt.h:527`) serializes all tile reads from one file. At high
thread counts with a small cache, threads queue on one file's I/O. Check
`--runstats` "mutex wait time". Mitigations: larger cache, more
concurrent-read-capable decoders, or per-subimage handles. This is a
design question; report the data before changing anything.

**H7: the thread pool task queue** (`src/libutil/thread.cpp:79`,
`typedef OIIO::spin_mutex Mutex`) is one spin-locked `std::queue` shared
by all workers. Measure `parallel_test` scaling (`build/bin/parallel_test
--help`) at 64+ threads. If it degrades, try `std::mutex`, or a
work-stealing / per-worker queue. Note OIIO can also use TBB
(`oiio_use_tbb`).

**H8: global atomics written per operation**, e.g. `m_mem_used`
(`imagecache_pvt.h:1357`) and `m_stat_*` on tile add/remove. These matter
only when tiles churn (cache thrash). `perf c2c` will show them.

For each hypothesis: measure before, change one thing, measure after
(wall and CPU, several trials, several thread counts), record in
STATUS.md, and commit each successful change separately with a terse
message.

---

## 6. `spin_mutex` vs `std::mutex`: initial recommendations

Rule of thumb: spin only when the hold time is short (well under a
microsecond), bounded, and never includes I/O, allocation of big buffers,
syscalls, or calls into unknown code. Otherwise a waiter should sleep.

Change to `std::mutex` (long or unbounded hold):
- `ImageCacheFile::SubimageInfo::average_color_mutex` (`imagecache.cpp:1472`):
  held across `get_pixels()`, which can read a tile from disk.
- `m_file_sweep_mutex` (H5): held across many file closes on the
  unconditional `lock()` path.
- `set_exr_threads()` (`src/openexr.imageio/exroutput.cpp:312`): held
  across `Imf::setGlobalThreadCount()`, which creates or destroys
  OpenEXR's thread pool.
- `DeepData` `m_mutex` (`src/libOpenImageIO/deepdata.cpp:61`, used at 90
  and 506): `alloc()` loops over all pixels, and `set_capacity()` can
  resize or insert into large vectors (big memmove) while holding it.

Better as an atomic or `std::call_once`:
- `maketx_mutex` (`maketexture.cpp:350`) only guards `++found_nonfinite`;
  use an atomic counter.
- `handler_mutex` in `tiffinput.cpp:729` is a one-time init (DCLP); use
  `std::call_once` or a function-local static.

Probably fine as is: short, bounded, or rare. Switch only if data says so.
- `m_fingerprints_mutex`, `m_perthread_info_mutex` (once per thread or
  per file, or stats).
- `shared_image_cache_mutex`, `shared_texturesys_mutex`.
- The polecolor static mutex in `texturesys.cpp:2451` (once per MIP
  level; a global lock shared by all files, but rare).
- The per-chunk result merges in `imagebufalgo_compare.cpp` and
  `imagebufalgo_pixelmath.cpp`.
- The error-message mutexes: `imagebuf.cpp:799`, `tiffinput.cpp:639`,
  `tiffoutput.cpp:221`, `oiiotool/imagerec.cpp:435`. Error paths only.
  They allocate strings under the lock, so `std::mutex` would be harmless
  there too.
- `TimingLog::mutex` (`imageio.cpp:91`): only when `oiio_log_times` is
  set; does map inserts with string allocation.
- The thread pool queue (H7): measure first.

`spin_rw_mutex` users (`unordered_map_concurrent` bins, the ustring table,
`color_ocio.cpp`'s cache) were not retuned. `spin_rw_mutex` still uses the
default backoff (yields after 5 rounds). Its read lock always writes the
shared counter (see H3). Benchmark `spin_rw_test` at high thread counts.

---

## 7. Deliverables
1. STATUS.md with the survey, the full run log, result tables, and
   conclusions.
2. One commit per verified improvement on a new branch off `lg-spinlock`
   (for example `lg-spinlock-x86`). Do not push without the user's OK.
3. A short report:
   - x86 verdict on `bad4c87e4`, and any constant changes, with numbers;
   - TextureSystem scaling before and after each fix, at 16/32/64/128/max
     threads;
   - which hypotheses were confirmed or rejected;
   - remaining bottlenecks with `perf c2c` evidence;
   - which `spin_mutex` -> `std::mutex` changes you made, and their
     measured effect.
