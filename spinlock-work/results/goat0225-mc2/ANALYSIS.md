# goat0225 phase 3: memory controller check (2026-10-14)

Build: lg-spinlock 3d7a9324d (victim cache + controller fcfcb1180). ctest 80/80.

## Controller: fixed the 256 MB / 64T failure
prod w7, 64T, --runstats (single run each):

| cache | mc | time | redundant reads | peak mem | held peak | flushes |
|---|---|---|---|---|---|---|
| 256 MB | 0 | 5.31 | 2.6 GB | 256 MB | 144 KB | 0 |
| 256 MB | 64 | 3.69 | 2.6 GB | 256 MB | 21.0 MB | 722 |
| 256 MB | 256 | 3.51 | 2.6 GB | 256 MB | 35.4 MB | 32 |
| 1 GB | 0 | 5.68 | 2.6 GB | 1024 MB | - | 0 |
| 1 GB | 64 | 3.70 | 2.6 GB | 1024 MB | 2.0 MB | 0 |
| 1 GB | 256 | 3.59 | 2.6 GB | 1024 MB | 44.5 MB | 0 |

- mc256 at 256 MB before the controller (goat0225-mc): 5.66 s, 6.2 GB redundant,
  384.5 MB. Now: 3.51 s, 2.6 GB, 256 MB. Fixed.
- At 1 GB the controller never fires. Held memory stays small.
- mc64 at 256 MB flushes often (722) but the cost is small: 3.69 vs 3.53 s
  before the controller, a single run, within noise. Flushes come in bursts:
  back-to-back over-limit sweeps see the same held bytes before threads drop
  their victims. Harmless; could rate-limit later if it ever shows up.

## Timing vs goat0225-mc: no regression
Same cells, best of 3. Differences are within the per-run trial range
(e.g. prod c8192 w7 allcores-32 mc64: 2.11 -> 2.32, but that run's trial
range was 0.42 s). With an 8 GB cache nothing is evicted and the controller
does not run. mc64 vs mc0 at 64T: w7 1.83x, w8 3.06x, prod 8 GB w7 1.42x,
w8 1.86x, prod 256 MB w7 1.48x, w8 1.74x.

## Decision
Default `microcache_size` = 64 stays. Phase 3 is ready to squash for a PR.
