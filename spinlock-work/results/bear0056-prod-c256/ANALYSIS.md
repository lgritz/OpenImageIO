# Analysis: bear0056-prod-c256 (production textures, 256 MB cache, 1000 files)

Only this config was pushed; the c8192 and c1024 runs are missing.
base vs sweep, 392 production EXR files, `--cachesize 256 --maxfiles 1000`.

## Key finding: the file limit, not the cache size, drove the 1738 numbers
prod-stats-w7, base, 32T:

| | 1738: 1 GB, 100 files | c256: 256 MB, 1000 files |
|---|---|---|
| ImageInputs created | 17825 | 392 |
| File I/O time (summed) | 1m44s | 32s |
| ImageInput mutex wait (summed) | **1m03s** | **0.2s** |
| redundant tile reads | 1.2 GB | 1.3 GB |
| w7 time at 1T | 3.47 s | 1.67 s |

- With 4x less cache but no reopens, it is 2x faster. The reopen cost
  dominated the earlier run.
- **Correction to the 1738 analysis:** the `m_input_mutex` wait was
  open/close under the lock, not tile reads serializing. With no reopens it is
  0.2 s summed. That is **not** a phase 3 target at production file limits.
- Redundant reads barely move from 1 GB to 256 MB (1.2 -> 1.3 GB, ~1.4 reads per
  tile). testtex w7 access is coherent, so an evicted tile is rarely needed
  again. This matches "read once, maybe twice" in production.

## Scaling (weak scaling, seconds)

| workload | 1T | base 16T | base 32T | sweep 16T | sweep 32T | gain @32T |
|---|---|---|---|---|---|---|
| prod-w1 | 0.67 | 1.44 | 1.85 | 0.81 | 1.04 | 1.78x |
| prod-w4 | 0.78 | 1.10 | 1.51 | 0.86 | 1.15 | 1.31x |
| prod-w7 | 1.66 | 1.87 | 2.79 | 1.74 | 2.68 | 1.04x |
| prod-w8 | 1.22 | 1.41 | 1.98 | 1.32 | 1.80 | 1.10x |

- Many files (w7/w8) at a realistic file limit: base already scales well to 16
  cores (+12-16% per-thread time at 16T). sweep cuts that to +5-8%. The 32T jump
  is SMT. On this 16-core box there is no contention collapse for the
  many-file case; the remaining loss is small.
- Hot single file (w1/w4): same 1.3-1.8x gain as in every other run.
- Microcache misses are still 97.1%, but at 16 cores the main-cache hit path
  costs only about 5% here.

## Effect on the phases
- Phase 2 holds: it helps in every config, with nothing worse anywhere.
- Phase 3: the microcache and bin-lock work have a small payoff at 16 cores
  for many-file workloads. Their case rests on hot-file contention (already
  mostly fixed by use()) and on higher core counts, which this machine can't
  show. `m_input_mutex` is dropped as a target.
- Still wanted: the c8192 and c1024 runs, and ideally a 64+ core machine.
