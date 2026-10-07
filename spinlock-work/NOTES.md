# spin_mutex / TextureSystem contention: findings so far

This is a working log for branch `lg-spinlock`. When conclusions are final,
port just the code changes to a clean branch for a PR. The live step log is
`STATUS.md`; the remote-run instructions are `README.md`.

## Commits on this branch

| commit | what |
|---|---|
| `bad4c87e4` | spin_mutex: atomic<bool> TTAS, backoff(128,64), branch hint fix, arm64 `isb` pause, no-unroll |
| `114a92be0` | ImageCacheTile::use(): test before write (no per-lookup store to a shared line) |
| `fdf355203` | ImageCacheFile::use(): atomic, test before write |
| `7624b8e90` | ImageCache file sweep lock: std::mutex instead of spin_mutex |

`remote/variants.txt` maps these to the variant names the scripts build:
base, spin, tileuse, fileuse, sweep.

## The two macOS reboots (attempts 1 and 2)
Both were XNU kernel panics: `Spinlock[...] timeout ... @locks.c:798`,
macOS 26 / xnu-12377.161.14, M4 Max. The panic logs are
`/Library/Logs/DiagnosticReports/panic-full-2026-10-06-{231920,234209}.0002.panic`.
Each time the panicking thread was in our benchmark, inside the C++20
`std::atomic_flag::wait()` path (`__ulock_wait`) or `notify_one()`
(`__ulock_wake`), with 16 threads competing. So heavy user-space ulock
traffic can trigger a kernel bug. **Never benchmark atomic wait/notify on
macOS.** The benchmark refuses it, and also refuses contended std::mutex on
macOS. Worth an Apple Feedback report.

## spin_mutex findings (M4 Max, 12P+4E cores)
- `pause()` matched only `__arm__`, so arm64 (Apple Silicon, Linux arm64)
  got an empty loop. Fixed with `isb` (about 8 ns). The `yield` hint is
  about 0.25 ns, effectively a nop, and benchmarks badly.
- The `lock()` branch hint was inverted (`!OIIO_UNLIKELY(try_lock())`).
  Fixing it was worth up to 2x at 16-32 threads.
- clang -O3 fully unrolled `pause(128)`, and the bloated lock() stopped
  inlining. Fixed with `OIIO_PAUSE_NOUNROLL`.
- atomic_flag vs atomic<bool>: identical arm64 codegen (swpab/stlrb/ldrb).
  Switching makes the test-and-test-and-set read legal (the old volatile
  cast was UB) and TSan-clean without `OIIO_THREAD_ALLOW_DCLP=0`.
  On MSVC, sizeof goes from 4 to 1 (ABI): main only.
- C++20: `atomic_flag::test()` gives nothing over `atomic<bool>::load()`.
  `wait()/notify()` is 4x slower uncontended, and it panicked macOS.
  No C++20-conditional code is needed.
- The article's exponential backoff (cap 64, never yields) is best at 2-8
  threads, bad at 16 (all cores busy), and unsafe when oversubscribed.
- Yielding early was the big cost at 8-12 threads. Chosen policy:
  `atomic_backoff(128, 64)`, i.e. pauses doubling to 128, then 64 rounds
  of 128, then yield.
- Tiny critical-section benchmarks mostly measure lock *unfairness*
  (whether the releasing thread re-acquires immediately), so they swing a
  lot with codegen. Weight the "medium" (work inside and outside the lock)
  and "coloc" (data on the lock's cache line) shapes more.

Real `spinlock_test` at -O3 on M4, wall ns/op (CPU ns/op):

| threads | 4 | 8 | 12 | 16 | 32 |
|---|---|---|---|---|---|
| old | 3.8 (21) | 26.2 (216) | 18.8 (222) | 5.0 (62) | 2.5 (41) |
| new | 1.2 (9) | 2.5 (23) | 3.8 (40) | 5.0 (78) | 6.2 (95) |

The old code wins only when oversubscribed with an empty critical section.
It does so by starvation: early yields let one thread run alone.

## TextureSystem findings
- The spin_mutex change alone left `testtex --threadtimes 1/4/7` unchanged
  within noise. ImageCache's hot path is not spin_mutex-bound.
- **ImageCacheTile::use()** did a seq_cst store (`xchg` on x86) of
  `m_used = 1` on about every lookup. That line also holds the fields every
  lookup reads, so threads bounce it. Test-before-write (`114a92be0`) on M4,
  weak-scaling seconds:
  - workload 1: 8 threads 1.37-1.40 -> 1.06-1.14; 16 threads 2.9-4.8 -> 2.2-2.3
  - workload 2: 16 threads 2.7-3.3 -> 2.2
  - workload 4: 16 threads 2.6-2.7 -> 2.2
  - workload 7: unchanged

  On M4, 16 threads includes the 4 slower E-cores, so those 16-thread
  numbers overstate the problem.
- `--runstats` (dry run on M4, workload 7, 16 threads) shows **94.9%
  micro-cache misses**. The per-thread 2-tile micro-cache is almost useless
  there, so nearly every lookup goes to the shared tile cache: a bin
  `spin_rw_mutex` read lock (`fetch_add` on a shared word) plus a refcount
  inc/dec. Workload 1 is about 0% misses. Prime suspect for many-core
  scaling of real (multi-texture) workloads; a bigger micro-cache is the
  next experiment.
- ImageCacheFile::use() is the same pattern (a plain bool, also a data
  race) on every by-name lookup (`fdf355203`). Not yet measured.
- Other hypotheses, for the many-core box (details in
  `x86-investigation-plan.md`, section 5):
  - The per-thread microcache holds only 2 tiles. Each miss takes a
    `spin_rw_mutex` read lock, whose `fetch_add` writes the bin's shared
    counter even with no writers, plus a tile refcount inc/dec.
  - File sweep lock (fixed in `7624b8e90`).
  - Per-file `m_input_mutex` I/O serialization.
  - The thread-pool queue is one spin-locked `std::queue`.
  - Global atomics written during tile churn.

## spin_mutex vs std::mutex
Spin only for short, bounded holds with no I/O, syscalls, or large
allocations.
- **Use std::mutex:**
  - `average_color_mutex` (`imagecache.cpp:1472`): held across
    `get_pixels`, which can read from disk. Not done yet: it lives in the
    copyable SubimageInfo, so it needs a copyable wrapper.
  - `m_file_sweep_mutex` (done, `7624b8e90`).
  - `set_exr_threads` (`exroutput.cpp:312`): held across OpenEXR thread
    pool rebuild.
  - DeepData `m_mutex` (`deepdata.cpp:90, 506`): held across per-pixel
    loops and large vector inserts.
- **Use an atomic / call_once:**
  - `maketx_mutex` (`maketexture.cpp:350`): it only guards a counter.
  - TIFF `handler_mutex` (`tiffinput.cpp:729`): one-time init.
- **Measure first:** the thread pool queue (`thread.cpp:79`).
- **Fine as is:** fingerprints, perthread_info, shared-cache singletons,
  polecolor, IBA per-chunk merges, the error-message mutexes, TimingLog.
