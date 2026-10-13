# Analysis: bear0056-20261012-1738 (full run + production textures)

Same machine as 1629: Threadripper PRO 5955WX, 16C/32T, 1 NUMA node,
gcc 11. ctest 86/86. File-name scrubbing checked: the only path left is
the testtex binary path.

## Reproducibility
Synthetic testtex, spinlock_test, parallel_test and spinbench all match
the 1629 run within a few percent. **The w1 spin 32T outlier is gone**
(spin 1.75 vs base 1.80; it was 3.24). That was noise, as suspected.

## Production textures: 392 OpenEXR files
49 GB pixel data, 19.6 GB on disk. testtex defaults:
max_memory_MB=1024, max_open_files=100.

Weak scaling, seconds at 32T (1T in parentheses):

| workload | base | spin | tileuse | fileuse | sweep | sweep vs base |
|---|---|---|---|---|---|---|
| prod-w1 (0.67) | 1.87 | 1.79 | 1.28 | 1.01 | 1.05 | 1.78x |
| prod-w4 (0.78) | 1.46 | 1.47 | 1.43 | 1.13 | 1.14 | 1.28x |
| prod-w7 (3.45) | 4.87 | 4.79 | 4.76 | 4.54 | 4.70 | 1.04x |
| prod-w8 (3.0) | 4.09 | 4.03 | 3.99 | 3.94 | 4.05 | 1.01x |

- Single production file (w1, w4): same result as the synthetic files.
  The use() fixes give 1.3-1.8x, and w1 is flat to 12T after them.
- Many production files (w7, w8): only 1-7%. That's not because contention
  got worse; per-thread time is **flat from 1T to 16T** (3.47 -> 3.33),
  then +45% at 32T (SMT). This run is bound by **I/O and file churn**, not
  locks. From runstats (base; sweep is similar):
  - ImageInputs: **17825 created for 392 files**, 336 peak open vs
    max_open_files=100. The open-file limit forces constant close/reopen.
  - File I/O time 1m44s summed (3.1 s per thread of about 4.8 s wall).
  - **ImageInput mutex wait 1m03s summed**, about 2 s per thread. Threads
    queue on the per-file `m_input_mutex` (held across tile read and
    decode) while another thread reads a different tile of the same file.
  - The 1 GB cache is full: 1.2 GB of redundant reads (tiles evicted, then
    read again).
  - Microcache misses: 97.1%.
- sweep (std::mutex for the file sweep): 17825 -> 14305 ImageInputs
  created (-20%), stats run 5.13 -> 4.85 s. A small but real plus in exactly
  the file-thrash case it targets.

**Caveat:** 1 GB / 100 files is a thrash configuration for 392 files.
Production uses about 8 GB. Before drawing conclusions about the many-file
workloads, rerun prodtex with a realistic cache and file limit (command
below).

## Effect on the phases
- **Phase 1** (spin_mutex, branch lg-spinmutex): confirmed again. No change.
- **Phase 2** (use() fixes, sweep): production data agree with synthetic.
  The use() fixes are as solid as phase 1. sweep shows a small gain under
  file thrash. Ready once the 8 GB rerun shows no surprises.
- **Phase 3** gets a new candidate, likely bigger than the microcache for
  real I/O-bound renders: **`m_input_mutex` serializes all tile reads per
  file**. Options: a small pool of ImageInputs per hot file, or skipping the
  lock for readers that can do concurrent tile reads (the OpenEXR core
  path). Also: peak open files 3.4x over the limit suggests the file sweep
  can't keep up while files are busy.

## Next remote run
Three production numbers matter (big renders):
- referenced by the scene: 1-2 TB;
- actually read from disk: tens of GB (unseen textures, coarse MIP levels);
- tile cache: 4-8 GB, so cache/read is about 10-40%. Most tiles are read once,
  some twice after eviction; there are few repeated evict/read cycles.

This test set: 49 GB referenced, 3.5 GB read (7%). So:

| config | cache/read | models | expect |
|---|---|---|---|
| `--cachesize 8192` | > 100% | light scenes, no eviction | pure contention |
| `--cachesize 1024` | 29% | **typical big render** | ~1.3 reads/tile (1738 run at 100 files: 1.2 GB redundant / 3.5 GB) |
| `--cachesize 256` | 7% | stress: heavy eviction | many re-reads |

All three use `--maxfiles 1000` (> 392 files, so no reopens, matching production's
~10000). Use base and sweep only, prodtex only, and a NEW `RUN` name per config (a reused
name skips finished variants). Run the first two; the third is optional:

```bash
cd ~/code/openimageio-feedstock
git -C oiio pull
for c in 8192 1024 256; do
  RUN=bear0056-prod-c$c VARIANTS="base sweep" STEPS="survey build prodtex" \
    PROD_ARGS="--cachesize $c --maxfiles 1000" PROD_TAG=-c$c \
    TEXLIST=/path/to/prodtextures.txt oiio/spinlock-work/remote/run_all.sh
done
```

Each prodtex pass takes about 4 min per variant, plus builds (incremental
after the first config). Every proposed change (phase 2, and later phase 3)
should be judged on both the 8 GB and 1 GB rows: a change can help one and hurt the
other (for example, a bigger microcache adds eviction pressure only when the cache is full).
