# goat0225 phase 3: victim microcache (2026-10-14)

Build: lg-spinlock 16c091bc3 (phase 1 + 2 + victim cache 099b1ada5).
`microcache_size` set at run time; mc0 = phase 2 behavior. 2 sockets x 16C x 2 SMT.
ctest 80/80.

## Speedup, mc64 vs mc0 (seconds in SUMMARY.md)
| workload | free-1 | s0cores-16 | split-16 | allcores-32 | free-64 |
|---|---|---|---|---|---|
| w1 | 0.99 | 1.03 | 0.94 | 0.98 | 1.01 |
| w4 | 0.99 | 1.02 | 1.11 | 1.10 | 1.12 |
| w7 | 1.05 | 1.31 | 1.67 | 1.89 | 1.93 |
| w8 | 1.15 | 1.85 | 2.52 | 3.02 | 3.10 |
| prod 8 GB w7 | 1.01 | 1.24 | 1.35 | 1.42 | 1.53 |
| prod 8 GB w8 | 1.00 | 1.46 | 1.48 | 1.80 | 1.96 |
| prod 1 GB w7 | 1.03 | 1.24 | 1.34 | 1.37 | 1.51 |
| prod 1 GB w8 | 0.94 | 1.41 | 1.44 | 1.79 | 1.72 |

- w1 (all hits in tile/lasttile) is unchanged, as expected; w4 gains up to 12%.
- The many-texture workloads gain 1.3-1.9x even on one socket, and up to 3x across
  sockets. That is more than the ceiling run predicted for prod (we expected
  <= 1.24x at 64T and got 1.5x). So the "remote NUMA memory" share was small,
  and most of it was the cross-socket line traffic of the main-cache lookup.
- Residual vs the machine limit (from goat0225-ceiling): synthetic w7 at
  allcores-32 is 1.28 / 1.18 = 1.08, under the 1.13 machine factor. **It is at
  the ceiling now** (it was 1.67 over).
- At 1 thread, w8 is 15% faster (it does no ALU work, so lookup cost dominates).
  Prod w8 1 GB free-1 0.94 is within the noise of that row (mc16 1.41, mc256 1.40).

## Size: mc16 is too small; mc256 adds a little more and breaks small caches
- mc16 gets about half the gain (prod 1 GB w7: 50.9% microcache misses).
- mc64 to mc256: up to another 10% (w7 split 1.26 -> 1.17, prod 8 GB w7
  split 2.03 -> 1.86, prod w8 64T 2.18 -> 2.03).
- **mc256 at a 256 MB cache, 64T, failed**: 6.2 GB of redundant reads (vs 2.6), 384.5 MB
  of memory at the end (limit 256 MB), and 5.66 s (vs mc0 5.08, mc64 3.53). 64 threads x
  258 held tiles x ~24 KB is about 400 MB, more than the whole cache. Evicted tiles still held by
  threads count in m_mem_used but can't be freed, so the sweep evicts live tiles
  instead.
- mc64 at 256 MB is fine: same redundant reads as mc0 (2.6 GB), 256 MB, and 1.44x
  faster. Same at 1 GB.

## Fix (committed after this run, not yet tested on goat)
Bound the memory that evicted-but-held tiles can take:
- The sweep marks a tile evicted before erasing it, and `m_mem_held` counts
  evicted tiles until they are destroyed.
- At the end of a sweep that was over the limit: if held > max_memory/16, halve the
  victim slots threads may use (published with an epoch; each thread drops its
  victims and adopts the new size at its next victim lookup). After 64 calm sweeps
  in a row (held < 1/64), grow one step back toward `microcache_size`.
- Stats: "Evicted tiles held by microcaches : <peak>, <n> flushes".
- M4 at a 10 MB cache with 16T w7 (extreme pressure): mc256 redundant reads went from
  13.6 to 8.7 GB (mc0 8.1), and time went from 3.47 to 2.40 s (mc0 2.82). mc64 is +2.5% reads and still
  faster than mc0. No change when nothing is evicted (the controller runs only
  in over-limit sweeps).

## Decision
- Default `microcache_size` = 64: nearly all the gain, safe at a 256 MB / 64T cache even
  without the controller. The controller is the safety net for bigger
  settings, smaller caches or more threads.
- Next goat run: confirm the controller (stats at 1 GB / 256 MB for mc0/64/256)
  and that it costs nothing in the timing configs.
