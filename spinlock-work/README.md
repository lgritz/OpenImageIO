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

## CURRENT NEXT RUN (updated 2026-10-14b): phase 3 memory controller, about 25 min

On goat0225. One build of lg-spinlock (phase 1 + 2 + victim microcache + the
memory controller for held tiles, fcfcb1180). microcache_size 0/64/256 at run time;
configs free-1, allcores-32, free-64; synthetic w7/w8, prod w7/w8 at 8 GB and
256 MB, `--runstats` at 1 GB / 256 MB, and ctest. Writes `results/<host>-mc2/`.
From TOP:

```bash
cd ~/code/openimageio-feedstock
git -C oiio checkout lg-spinlock && git -C oiio pull
oiio/spinlock-work/remote/next_run.sh /path/to/prodtextures.txt
```

- At the end it commits **all** of `spinlock-work/results` and pushes with
  `(cd oiio ; git50 push lgritz lg-spinlock)`.
- Safe to rerun; finished result files are skipped. Env knobs are listed at the
  top of the script.
- TOP/build is shared: don't run on two machines at once.
- Earlier run scripts are in git history (94fe319a6 full+prod, 61fa1c335 ceiling,
  16c091bc3 mc).

## Machine layout this assumes (the "feedstock" machine)

```
TOP/                  ~/code/openimageio-feedstock  -- run everything from here
TOP/build.sh          the special-environment build (--clean, EXTRA_CMAKE_ARGS)
TOP/oiio/             OIIO checkout (submodule), on branch lg-spinlock
TOP/build/            build tree (written by build.sh)
TOP/dist/             install tree (written by build.sh)
TOP/spinwork/         created by the scripts: script copy, generated textures,
                      spinbench binaries, perf data (never committed)
```

## What the run does

For each variant in `remote/variants.txt`:
- `base`: main, before this work
- `spin`: new spin_mutex
- `tileuse`: + the tile `use()` fix
- `fileuse`: + the file `use()` fix
- `sweep`: + the file sweep lock as std::mutex

it does the following:
1. `git checkout --detach <commit>` in `TOP/oiio`.
2. `./build.sh`, with `-DOIIO_BUILD_TESTS=ON` appended to your
   `EXTRA_CMAKE_ARGS`. Incremental; pass `CLEAN_FIRST=1` to do `--clean`
   on the first variant.
3. Runs that variant's benchmarks from `TOP/build/bin`:
   - `testtex --threadtimes` on generated textures (workloads 1, 2, 4, 7, 8;
     low `--maxfiles`; small `--cachesize`; `--runstats` at full thread
     count), and on **your production textures** if you give it a list;
   - for `base` and `spin` only: `spinlock_test`, `spin_rw_test` and
     `parallel_test` at each thread count;
   - for `spin` only: the `spinbench` lock microbenchmark, at up to 2x the
     hardware threads, plus a single-NUMA-node run if there is more than
     one node;
   - for the newest variant: ctest (spin/thread/parallel/ustring units,
     texture, imagecache, maketx);
   - optionally (`PERF=1`), `perf c2c` / `perf record` profiles for base,
     spin and newest.

At the end it writes `results/<run>/SUMMARY.md`, and **restores `TOP/oiio`
to the checkout it started from**, even if it fails or you hit ^C. Results
are written as untracked files, so they survive the checkouts.

Note: when it finishes, `TOP/build` and `TOP/dist` contain the **last
variant built** (`sweep`, which matches the lg-spinlock code). Run
`./build.sh` afterwards if you need them to match something else.

## Requirements
- A working `TOP/build.sh` (it already handles the special environment).
- `git`, `bash`, `python3` (standard library only), a C++ compiler on PATH
  for the small standalone benchmark (`CXX=...` to choose it).
- Optional: `numactl` (NUMA comparison), `perf` (profiles).
- Optional, for ctest: `oiio-images` (and other test image repos) cloned
  in `TOP/`, next to `oiio/`. Without them, the texture tests that need
  them fail; that is expected and harmless here.

## Steps

```bash
cd ~/code/openimageio-feedstock          # TOP
cd oiio && git fetch && git checkout lg-spinlock && git pull && cd ..

# 1. Smoke test (short; mostly build time: 5 incremental builds)
QUICK=1 oiio/spinlock-work/remote/run_all.sh
#    Check oiio/spinlock-work/results/<host>-<date>-quick/SUMMARY.md looks sane.

# 2. Full run (several hours). Use tmux/screen or nohup, and keep the
#    machine otherwise idle (it loads every core).
#    Production textures: one path per line in a list file. Use typical
#    tiled, MIP-mapped .tx/.exr files from a real asset; 10-100 files is
#    plenty, and UDIM patterns work too.
TEXLIST=/path/to/prodtextures.txt oiio/spinlock-work/remote/run_all.sh
#    (add PERF=1 if `perf` works for your user; see below)

# 3. Commit and push the results (small text files only). Add the whole
#    results dir, not one run: a loop of runs leaves several new dirs.
cd oiio
git add spinlock-work/results
git commit -m "spinlock-work: results from <run-name(s)>"
git push
```

The script refuses to start if `TOP/oiio` has uncommitted changes to
tracked files, because it checks out other commits.

### If it is interrupted or the machine crashes
The last `START` line without a matching `END` in
`results/<run>/progress.log` (and `results/<run>/bench.log` for spinbench)
names exactly what was running. Push that directory anyway; it is useful.
After a crash, `TOP/oiio` may be left on a variant commit, so first run
`git -C oiio checkout lg-spinlock`. To **resume**, rerun with the same run
name; finished steps and variants are skipped:
```bash
RUN=<run-name> TEXLIST=... oiio/spinlock-work/remote/run_all.sh
```
To run only some steps, set e.g. `STEPS="build testtex prodtex"` (keep
`build` in the list, or each variant uses whatever is in `TOP/build`). To
run only some variants, set e.g. `VARIANTS="base sweep"`.

### Production file names
By default (`SCRUB_NAMES=1`), production texture paths are replaced with
`prodtex0`, `prodtex1`, ... in the results. Still, glance at `SUMMARY.md`
and the `testtex-*-prod-*.txt` files before pushing to GitHub. No texture
data is ever copied into `results/`.

### perf (optional)
`PERF=1` adds `perf c2c` (finds cache lines bounced between cores) and
`perf record` profiles. It needs `/proc/sys/kernel/perf_event_paranoid`
<= 1, or a user allowed to use perf. The big `.data` files stay in
`TOP/spinwork`; only text reports go into `results/`.

## Safety notes
- The C++20 atomic wait/notify variant that panicked macOS is not in the
  benchmark, and the driver refuses to run if it appears.
- Benchmark processes kill themselves after 300 s (`alarm`).
- No-yield spin variants are never run with more threads than hardware
  threads.
- Every command is logged and `sync`ed before it runs.
