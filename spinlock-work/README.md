# spinlock-work: contention benchmarks to run on the many-core x86 machine

Working area for branch `lg-spinlock`, not for merging. Contents:

- `NOTES.md`: findings so far, and the commits being tested.
- `STATUS.md`: step-by-step log from the M4 Max sessions (also the crash log).
- `x86-investigation-plan.md`: detailed plan and hypotheses, originally
  written for an agent.
- `bench/`: the standalone lock microbenchmark (`spinbench.cpp`) plus its
  driver and helpers.
- `remote/`: the scripts to run on the remote machine (no agent needed).
  - `run_all.sh`: does everything.
  - `variants.txt`: which commits get built and compared.
  - `timed.py`, `scrub.py`, `summarize.py`: helpers.
- `results/<run>/`: outputs. Commit these and push them back.

## What the run does

1. Records machine info: `lscpu`, NUMA, SMT, compiler.
2. Builds 5 variants of OIIO, each in its own git worktree under `$WORK`:
   - `base`: main, before this work
   - `spin`: new spin_mutex
   - `tileuse`: + the tile `use()` fix
   - `fileuse`: + the file `use()` fix
   - `sweep`: + the file sweep lock as std::mutex

   Only the needed targets are built, except for the newest variant, which
   is fully built for ctest.
3. ctest on the newest variant: spin/thread/parallel/ustring units,
   texture, imagecache and maketx tests.
4. `spinbench`: lock variants, three critical-section shapes, thread
   counts up to 2x hardware threads; plus a single-NUMA-node run if there
   is more than one node.
5. `spinlock_test`, `spin_rw_test`, `parallel_test`: base vs new, at each
   thread count.
6. `testtex --threadtimes` on generated textures, for every variant:
   workloads 1, 2, 4, 7, 8, a low `--maxfiles`, a small `--cachesize`,
   and `--runstats` at full thread count.
7. The same on **your production textures**, if you give it a list.
8. Optional: `perf c2c` / `perf record` (cache-line sharing profiles).
9. Writes `results/<run>/SUMMARY.md`.

## Requirements on the remote machine
- Everything you normally need to build OIIO there: compiler, CMake, deps.
- `git`, `python3` (standard library only), `bash`.
- Optional: `numactl` (NUMA comparison), `perf` (profiles).
- Disk: 5 build trees under `$WORK` (default `<repo>/../oiio-spinwork`).
- Optional, for the ctest step: test image repos (`oiio-images`,
  `openexr-images`, ...) cloned *next to* the OIIO checkout. The script
  symlinks them into each worktree. Without them, the texture tests that
  need them fail; that is expected and harmless here.

## Steps

```bash
# 0. Get the branch
git fetch origin && git checkout lg-spinlock && git pull

# 1. Smoke test first (short, about 15-30 min, mostly build time).
#    Pass whatever cmake args you normally use to find dependencies.
export OIIO_CMAKE_ARGS="-DCMAKE_PREFIX_PATH=/your/deps -DUSE_PYTHON=0"
QUICK=1 spinlock-work/remote/run_all.sh
#    Check spinlock-work/results/<host>-<date>-quick/SUMMARY.md looks sane.
#    The full run reuses those builds (same WORK dir), so it won't rebuild.

# 2. Full run (several hours on a big machine). Use tmux/screen or nohup,
#    and keep the machine otherwise idle (it loads every core).
#    Production textures: one path per line in a list file. Use typical
#    tiled, MIP-mapped .tx/.exr files from a real asset. 10-100 files is
#    plenty; UDIM patterns work too.
TEXLIST=/path/to/prodtextures.txt spinlock-work/remote/run_all.sh
#    (add PERF=1 if `perf` works for your user; see below)

# 3. Commit and push the results (small text files only)
git add spinlock-work/results/<run-name>
git commit -m "spinlock-work: results from <run-name>"
git push
```

### If it is interrupted or the machine crashes
The last `START` line without a matching `END` in
`results/<run>/progress.log` (and `results/<run>/bench.log` for spinbench)
names exactly what was running. Push that directory anyway; it is useful.
To **resume**, rerun with the same run name; finished steps are skipped:
```bash
RUN=<run-name> TEXLIST=... spinlock-work/remote/run_all.sh
```
To run only some steps: `STEPS="testtex prodtex"`. To build/compare only
some variants: `VARIANTS="base sweep"`.

### Production file names
By default (`SCRUB_NAMES=1`), production texture paths are replaced with
`prodtex0`, `prodtex1`, ... in the results. Still, glance at
`SUMMARY.md` and the `testtex-*-prod-*.txt` files before pushing to
GitHub. No texture data is ever copied into `results/`.

### perf (optional)
`PERF=1` adds `perf c2c` (finds cache lines bounced between cores) and
`perf record` profiles for base / spin / newest. It needs
`/proc/sys/kernel/perf_event_paranoid` <= 1, or run as a user allowed to
use perf. The large `.data` files stay in `$WORK`; only text reports go
into `results/`.

## Safety notes
- The C++20 atomic wait/notify variant that panicked macOS is not in the
  benchmark, and the driver refuses to run if it appears.
- Benchmark processes kill themselves after 300 s (`alarm`).
- No-yield spin variants are never run with more threads than hardware
  threads.
- Every command is logged and `sync`ed before it runs.
