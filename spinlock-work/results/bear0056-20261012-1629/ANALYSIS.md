# Analysis: bear0056-20261012-1629 (full run)

## Machine caveat
- AMD Threadripper PRO 5955WX: 16 cores / 32 threads, 1 socket, 1 NUMA
  node, 2 CCDs (2 x 32 MB L3). GCC 11.5, -O3.
- **Not the 64+ core box we planned for.** testtex only went to 32
  threads (= hw threads). The ">32 threads" question is still open.
- No TEXLIST (no production textures), no perf, no numactl.
- `pause` costs 14.6 ns here (about 65 cycles, Zen3), vs `isb` at 8 ns on
  M4. backoff(128,64) spins about 120 us here before its first yield.
- ctest on sweep: 86/86 pass.
- The quick run (1602-quick) is too short to tell anything (0.01-0.02 s
  cells). Ignore it.

## 1. spin_mutex change (base -> spin)
Validated on x86. It wins or ties up to hw threads. No x86-specific
constants are needed.

spinlock_test (s, lower is better):

| threads | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|
| base | 1.7 | 3.7 | 6.3 | 10.9 | 10.2 | 9.2 |
| spin | 1.4 | 2.3 | 4.3 | 7.5 | 8.1 | 9.1 |

- CPU time is also lower up to 48 threads (16T: 195 vs 297 cpu-s).
  Unlike on M4, the new lock ties even at 2x oversubscription.
- spinbench, tiny critical section, ns/op wall: 16T 29 vs 52 (old), 32T
  48 vs 48. The old lock wins only oversubscribed (64T: 19 vs 53, and
  3x less CPU), because it yields almost at once. That is the same
  tradeoff as on M4.
- Backoff grid: `128y64` (the committed choice) is the best family on
  x86 too. `128y16` is a bit better oversubscribed, worse at 32T. Keep 128,64.
- std::mutex: about 2x slower than the new spin lock up to 16T, about equal at
  32T+, and better in `coloc` at 32T+ (29 vs 45 ns). This confirms the rule: spin
  only for tiny critical sections with at most about core-count contenders.
- C++20 `atomic_flag::test()` (F_flagtest): worse at 16T+. No reason
  for any C++20 path.
- parallel_test: +5-12% launch rate (64T: 4.45e8 vs 3.97e8).
- spin_rw_test: unchanged (it shares atomic_backoff; no harm).
- testtex: spin alone does nothing (1.00x everywhere), as on M4. The
  lock is not the TextureSystem bottleneck.

**Outlier:** testtex w1 spin at 32T = 3.24 s vs base 1.82, consistent
over 3 trials (range 0.08). But stats-w1 (a single 32T run) gives spin 1.74
vs base 1.80, and w1 with handles takes no spin_mutex on the hit path.
Most likely a bimodal SMT/placement effect in the pathological
"everybody writes one cache line" regime, which tileuse/fileuse remove.
Worth one rerun of `VARIANTS="base spin" STEPS="build testtex"`, but not
a blocker.

## 2. The use() fixes are the real TextureSystem win
Weak scaling (fixed work per thread, so flat is perfect; 32T is SMT, so
~1.3-1.5x over 16T is expected):

| workload | 1T | base 16T | base 32T | sweep 16T | sweep 32T | gain @32T |
|---|---|---|---|---|---|---|
| w1 same spot, handles | 0.66 | 1.49 | 1.82 | 0.84 | 1.01 | 1.80x |
| w2 same spot, names | 0.72 | 1.48 | 1.93 | 0.78 | 1.08 | 1.79x |
| w4 one file, diff spots | 0.75 | 1.25 | 1.71 | 0.87 | 1.22 | 1.40x |
| w7 many files, overlap | 0.90 | 1.57 | 2.08 | 1.38 | 1.87 | 1.11x |
| w8 = w7, no busywork | 0.45 | 1.10 | 1.47 | 0.92 | 1.26 | 1.17x |
| maxfiles16 (64 files) | 1.15 | 1.70 | 2.14 | 1.49 | 1.95 | 1.10x |
| cache64 | 0.96 | 1.64 | 2.11 | 1.48 | 1.95 | 1.08x |

- With w1/w2/w4, base is about 2x worse per thread at only **2** threads (0.66 ->
  1.33). That is pure cache-line ping-pong on `m_used`. After the fixes,
  w1/w2 are flat to 12 threads.
- tileuse gives most of the gain on 1 thread -> 2 threads. fileuse finishes the job (w1 2T:
  0.88 -> 0.66).
- sweep (std::mutex for the file sweep) is neutral, even in maxfiles16, where
  sweeps happen. Keep it on CPU-waste grounds (the sweep does file
  close I/O while others would spin), not for speed.

## 3. What still does not scale: w7/w8 (microcache misses)
After all fixes, w8 is still 2.0x worse per thread at 16T (0.45 -> 0.92),
and w7 is 1.5x worse. runstats: **94.9% microcache misses** (129M lookups go to
the main cache). Each miss in `find_tile_main_cache` does:
1. a `spin_rw_mutex` read lock on the bin: `fetch_add` plus `fetch_sub` on
   a word shared by every thread that hashes to that bin (the 2
   files everyone reads make a few very hot bins);
2. a tile intrusive refcount ++ on the shared tile (and -- when it leaves
   the 2-entry microcache);
3. `wait_pixels_ready()` (a load only, so fine).

So that is 3-4 RMWs on shared cache lines per lookup. That's the next
target, roughly in order of expected payoff and inverse risk:
- **A. Bigger per-thread microcache** (2 -> 8 or 16 entries, small
  direct-mapped or LRU). w7 cycles over 8 files per pixel, so a
  2-entry cache misses almost always. Cheap, local, and it removes most main-cache
  traffic. Try first.
- **B. Bin read lock without a shared write**: for example a seqlock or
  per-bin version counter for readers (retrieve = read version, find, inc
  refcount, recheck), or a sharded reader count. Bigger change.
- **C. Avoid the refcount RMW on hit**: hard because of eviction safety
  (would need epoch/hazard-style reclamation). Last resort.

## Recommendations
1. Keep all 4 commits (spin, tileuse, fileuse, sweep). Port them to a
   clean PR branch whenever convenient; the data support them on both arm64 and x86.
2. Next experiment: microcache size (A), measured with w7/w8, maxfiles16,
   cache64, and `--runstats` to see the miss % drop.
3. Still needed from the remote: (a) a run with `TEXLIST` on production
   textures; (b) ideally a real 64+ core / multi-socket machine, which is where
   the ">32 threads gets very bad" report comes from. On a 16-core box
   we can't reproduce that regime; and (c) `perf c2c` if it ever becomes available.
