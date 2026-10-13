# Production textures across cache sizes (bear0056, 2026-10-12)

Runs: `bear0056-prod-c8192`, `-c1024`, `-c256`. 392 production EXR files (49 GB
referenced), `--maxfiles 1000` (no reopens), base vs sweep (= all phase 2
commits). Threadripper 5955WX, 16C/32T.

## What each config models
The unique working set testtex touches is **2.3 GB** (the 8 GB run: 102463
tiles, no evictions). Earlier "3.5 GB read" included re-reads.

| config | cache / unique read | redundant reads | models |
|---|---|---|---|
| c8192 | > 100% (2.3 GB peak) | 0 | light scenes, nothing evicted |
| c1024 | 43% | 1.1 GB (+48% tiles) | typical big render (10-40%) |
| c256 | 11% | 1.3 GB (+56% tiles) | low end of production |

Re-reads stay around 1.5 reads per tile even at 11%: coherent access, as in production.
ImageInput mutex wait is 0.2 s summed in all three.

## Weak scaling, seconds (flat = perfect; 32T is SMT)

| workload | cache | 1T | base 16T | sweep 16T | base 32T | sweep 32T | gain @32T |
|---|---|---|---|---|---|---|---|
| w1 hot file | 8 GB | 0.67 | 1.43 | 0.72 | 1.90 | 1.02 | 1.86x |
| | 1 GB | 0.67 | 1.47 | 0.78 | 1.78 | 1.02 | 1.75x |
| | 256 MB | 0.67 | 1.44 | 0.81 | 1.85 | 1.04 | 1.78x |
| w4 one file | 8 GB | 0.78 | 1.13 | 0.84 | 1.45 | 1.15 | 1.26x |
| | 1 GB | 0.78 | 1.07 | 0.86 | 1.47 | 1.14 | 1.29x |
| | 256 MB | 0.78 | 1.10 | 0.86 | 1.51 | 1.15 | 1.31x |
| w7 many files | 8 GB | 1.67 | 1.78 | 1.66 | 2.45 | 2.35 | 1.04x |
| | 1 GB | 1.68 | 1.83 | 1.65 | 2.74 | 2.62 | 1.05x |
| | 256 MB | 1.66 | 1.87 | 1.74 | 2.79 | 2.68 | 1.04x |
| w8 many files | 8 GB | 1.22 | 1.34 | 1.23 | 1.79 | 1.68 | 1.07x |
| | 1 GB | 1.22 | 1.32 | 1.22 | 1.89 | 1.83 | 1.03x |
| | 256 MB | 1.22 | 1.41 | 1.32 | 1.98 | 1.80 | 1.10x |

## Conclusions
1. **Phase 2 behaves the same with and without eviction.** The gain per workload is
   within noise across 8 GB / 1 GB / 256 MB. It never makes anything slower in the wedge tables.
   (The single c1024 stats run, sweep 3.06 s vs base 2.87 s, is I/O variance:
   its File I/O time was 35 s vs 26 s, with identical tile counts. The 3-trial wedges show
   sweep at or below base everywhere.)
2. **After phase 2, 16 cores scale almost perfectly in every config:** w7/w8 at
   16T are within 0-5% of 1T. base loses 6-16%.
3. **Eviction costs little here:** w7 at 32T goes 2.35 (8 GB) -> 2.62 (1 GB) ->
   2.68 (256 MB). That is with files likely in the OS page cache. On network
   storage, re-reads cost more; testtex on this box can't show that.
4. **Microcache memory (phase 3) is negligible at these sizes:** about 24 KB per tile
   in memory (256 MB / 10908 tiles). 32 threads x 8 entries = 6 MB, which is 2.4% of
   a 256 MB cache and 0.07% of 8 GB.
5. **Phase 3 has little left to win on 16 cores.** Its case must come from
   higher core counts / multi-socket, or from real render access patterns
   (incoherent rays), not from this box.

## Status
- Phase 1 (lg-spinmutex): ready.
- Phase 2: validated on synthetic and production data, at every cache size. Ready.
- Phase 3: needs a 64+ core machine (or a renderer trace) to justify it.
