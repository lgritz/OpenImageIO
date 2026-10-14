# goat0225: sweep skips tiles in use (2026-10-14)

Build: lg-spinlock bf7f4335e (3064a6bea: skip referenced tiles, stop when
stuck). ctest 80/80.

## Untiled single-tile images (16 x 16 MB, 64T, --runstats)
| cache | mc0 | mc64 | mc256 |
|---|---|---|---|
| 32 MB | 19.2 s, 29.9 GB redundant | 46.9 s, 67.4 GB | 14.0 s, 34.0 GB |
| 128 MB | 7.9 s, 13.0 GB | 0.59 s, 1.3 GB | 0.19 s, 0.1 GB |
| 512 MB | 0.13 s, 0 | 0.12 s, 0 | 0.13 s, 0 |

- Bounded: no runaway, peak 256 MB (all 16 images) at every cache size.
  Stuck sweeps are counted and rate-limited.
- 32 MB mc64 is the bad cell: victim slots holding 16 MB tiles, controller
  only reacted on stuck sweeps (6 flushes).

## Regression: prod 256 MB, mc256
- Ended at 363.7 MB (limit 256), 16 stuck sweeps; mc2 held 256 MB. Victim-held
  tiles are no longer evicted, so they pin cache memory, and the controller
  only reacted to stuck sweeps.
- mc64 at 256 MB and everything at 1 GB stayed at the limit.

## Timings
Within noise of goat0225-mc2 in every cell (e.g. prod c8192 w7 allcores-32
mc64 2.32 -> 2.16, free-64 3.67 -> 3.51).

## Fix (committed after this run)
Each thread tracks bytes in its victim slots; the sweep sums them and halves the
slots above max/2 (grows back below max/4). M4 16T: mc256 at 10/20 MB caches
stays at the limit; untiled unchanged; ctest 84/84.
