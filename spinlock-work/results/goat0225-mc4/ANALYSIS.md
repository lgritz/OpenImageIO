# goat0225: victim-slot memory limit (2026-10-14)

Build: lg-spinlock 8d815c21f (7f947c3af: per-thread victim bytes, halve
above max/2). ctest 80/80.

## Memory limit holds
- prod 256 MB, mc256, 64T: 256 MB at the end (mc3: 363.7), 2.6 GB redundant
  (= mc0), 3.69 s (mc0 5.33), 71 flushes, victim sum peak 219.5 MB.
- prod 256 MB mc64 and all of 1 GB: no flushes, at the limit. The per-thread sum
  (95 MB at mc64) stays under max/2, so the overcount doesn't hurt mc64.
- Untiled: peak 256 MB (all 16 images) everywhere, no runaway. Times are noisy
  between runs (mc0 c128: 7.9 s in mc3, 23.9 s here, same code path), so only
  boundedness is meaningful.

## Trial ranges: mc3 had hidden blowups
prod c256 64T, range across 3 trials (best-of-3 hid it):

| run | w7 mc64 | w7 mc256 | w8 mc64 | w8 mc256 |
|---|---|---|---|---|
| mc2 | 2.03 | 2.34 | 1.95 | 2.06 |
| mc3 | 8.49 | 139.17 | 20.68 | 92.75 |
| mc4 | 1.61 | 1.94 | 1.80 | 1.88 |

mc3 (skip-in-use without the victim limit) had trials up to 143 s. mc4 is
back to mc2 levels.

## Timings
Best-of-3 within noise of mc2/mc3 (64T prod cells have 0.5-2 s ranges).

## Done
Phase 3 squashed onto lg-ic-contention as lg-ic-microcache (8e9f1d317, amended
to drop two stray files).
