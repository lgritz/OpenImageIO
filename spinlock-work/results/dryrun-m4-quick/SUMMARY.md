# Results: dryrun-m4-quick

## Machine
<details><summary>machine.txt</summary>

```
## date
Wed  7 Oct 2026 09:15:59 PDT
## git
7624b8e9070076501112ff99acc0c8cacb5c31ae perf(IC): file sweep lock should block, not spin
## uname
Darwin unagi3.local 25.6.0 Darwin Kernel Version 25.6.0: Fri Jul 31 19:17:26 PDT 2026; root:xnu-12377.161.14~5/RELEASE_ARM64_T6041 arm64
## nproc
16
## lscpu
## numa
numactl not available
## smt
n/a
## governor
n/a
## memory
sysctl: sysctl fmt -1 1024 1: Operation not permitted
## compiler
c++: error: couldn't create cache file '/var/folders/py/q8rnbld17pj29665hxgmrz_m0000gn/T/xcrun_db-e34yOOuv' (errno=Operation not permitted)
Apple clang version 21.0.0 (clang-2100.3.34.2)
## cmake
cmake version 4.4.4
## python
Python 3.14.8
## perf
no perf
## OIIO_CMAKE_ARGS
-DUSE_PYTHON=0
```
</details>

## Pause instruction cost
```
empty 0.000 ns
isb 7.861 ns
yield-insn 0.248 ns
```

## ctest (newest variant)
```
100% tests passed out of 1
100% tests passed out of 84
```

## testtex --threadtimes (seconds)
Weak scaling (fixed iters/thread): flat = perfect. Strong scaling (quick runs with --iters): halving = perfect.

**cache64 (strong scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 |
|---|---|---|---|---|---|---|
| base | 0.14 | 0.08 | 0.05 | 0.04 | 0.04 | 0.04 |
| sweep | 0.14 | 0.08 | 0.05 | 0.04 | 0.04 | 0.04 |

Speedup vs base at 16 threads: sweep 1.00x

**maxfiles16 (strong scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 |
|---|---|---|---|---|---|---|
| base | 0.15 | 0.09 | 0.07 | 0.07 | 0.07 | 0.07 |
| sweep | 0.15 | 0.09 | 0.07 | 0.07 | 0.06 | 0.07 |

Speedup vs base at 16 threads: sweep 1.00x

**prod-stats-w7 (weak scaling)**

| variant | 16 |
|---|---|
| base | 0.00 |
| sweep | 0.00 |

**prod-w7 (strong scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 |
|---|---|---|---|---|---|---|
| base | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| sweep | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |

**stats-w1 (weak scaling)**

| variant | 16 |
|---|---|
| base | 0.02 |
| sweep | 0.01 |

Speedup vs base at 16 threads: sweep 2.00x

**stats-w7 (weak scaling)**

| variant | 16 |
|---|---|
| base | 0.02 |
| sweep | 0.02 |

Speedup vs base at 16 threads: sweep 1.00x

**w1 (strong scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 |
|---|---|---|---|---|---|---|
| base | 0.10 | 0.05 | 0.03 | 0.02 | 0.02 | 0.02 |
| sweep | 0.10 | 0.05 | 0.02 | 0.01 | 0.01 | 0.01 |

Speedup vs base at 16 threads: sweep 2.00x

**w7 (strong scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 |
|---|---|---|---|---|---|---|
| base | 0.12 | 0.06 | 0.04 | 0.02 | 0.02 | 0.02 |
| sweep | 0.12 | 0.06 | 0.04 | 0.02 | 0.02 | 0.02 |

Speedup vs base at 16 threads: sweep 1.00x

<details><summary>testtex-base-prod-stats-w7.txt</summary>

```
$ /Users/lg/tmp/claude-501/spinwork/base/build/bin/testtex --threadtimes 7 --threads 16 --runstats --iters 200000 prodtex0 prodtex1
hw threads = 16
threads  time (s)   speedup efficiency
    ImageInputs : 0 created, 0 current, 0 peak
    Total pixel data size of all images referenced : 0 B
    File I/O time : 0.1s (0.0s average per thread, for 17 threads)
    ImageInput mutex locking time : 0.1s
    Peak cache memory : 0 B
wall=0.061 cpu=0.061 rc=0
```
</details>

<details><summary>testtex-base-stats-w1.txt</summary>

```
$ /Users/lg/tmp/claude-501/spinwork/base/build/bin/testtex -t 0 --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 1 --threads 16 --runstats --iters 200000
hw threads = 16
threads  time (s)   speedup efficiency
    ImageInputs : 1 created, 1 current, 1 peak
    Total pixel data size of all images referenced : 10.7 MB
    ImageInput mutex locking time : 0.0s
  Tiles: 2 created, 2 current, 2 peak
    micro-cache misses : 32 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=0.311 cpu=1.719 rc=0
```
</details>

<details><summary>testtex-base-stats-w7.txt</summary>

```
$ /Users/lg/tmp/claude-501/spinwork/base/build/bin/testtex -t 0 --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 7 --threads 16 --runstats --iters 200000
hw threads = 16
threads  time (s)   speedup efficiency
    ImageInputs : 10 created, 10 current, 10 peak
    Total pixel data size of all images referenced : 106.7 MB
    File I/O time : 0.1s (0.0s average per thread, for 17 threads)
    ImageInput mutex locking time : 0.0s
  Tiles: 801 created, 800 current, 800 peak
    micro-cache misses : 407315 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=0.316 cpu=1.701 rc=0
```
</details>

<details><summary>testtex-sweep-prod-stats-w7.txt</summary>

```
$ /Users/lg/tmp/claude-501/spinwork/sweep/build/bin/testtex --threadtimes 7 --threads 16 --runstats --iters 200000 prodtex0 prodtex1
hw threads = 16
threads  time (s)   speedup efficiency
    ImageInputs : 0 created, 0 current, 0 peak
    Total pixel data size of all images referenced : 0 B
    File I/O time : 0.1s (0.0s average per thread, for 17 threads)
    ImageInput mutex locking time : 0.1s
    Peak cache memory : 0 B
wall=0.062 cpu=0.061 rc=0
```
</details>

<details><summary>testtex-sweep-stats-w1.txt</summary>

```
$ /Users/lg/tmp/claude-501/spinwork/sweep/build/bin/testtex -t 0 --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 1 --threads 16 --runstats --iters 200000
hw threads = 16
threads  time (s)   speedup efficiency
    ImageInputs : 1 created, 1 current, 1 peak
    Total pixel data size of all images referenced : 10.7 MB
    ImageInput mutex locking time : 0.0s
  Tiles: 3 created, 2 current, 3 peak
    micro-cache misses : 32 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=0.302 cpu=1.550 rc=0
```
</details>

<details><summary>testtex-sweep-stats-w7.txt</summary>

```
$ /Users/lg/tmp/claude-501/spinwork/sweep/build/bin/testtex -t 0 --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 7 --threads 16 --runstats --iters 200000
hw threads = 16
threads  time (s)   speedup efficiency
    ImageInputs : 10 created, 10 current, 10 peak
    Total pixel data size of all images referenced : 106.7 MB
    File I/O time : 0.1s (0.0s average per thread, for 17 threads)
    ImageInput mutex locking time : 0.0s
  Tiles: 802 created, 800 current, 800 peak
    micro-cache misses : 407315 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=0.315 cpu=1.660 rc=0
```
</details>

## spinlock_test

**program-reported time (s, best of trials)**

| variant | 1 | 4 | 16 |
|---|---|---|---|
| base | 0.0 | 0.1 | 0.1 |
| sweep | 0.0 | 0.0 | 0.1 |

**whole process wall/cpu seconds (all trials)**

| variant | 1 | 4 | 16 |
|---|---|---|---|
| base | 0.367/0.366 | 0.579/1.096 | 0.645/4.178 |
| sweep | 0.373/0.372 | 0.453/0.595 | 0.708/5.066 |

## parallel_test

**parallel_for task launches per second (higher is better)**

| variant | 1 | 4 | 16 |
|---|---|---|---|
| base | 7.01e+07 | 2.35e+08 | 6.12e+08 |
| sweep | 6.07e+07 | 2.61e+08 | 6.25e+08 |

## spin_rw_test (strong scaling; seconds, best of 3)

**spin_rw_test --wedge**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 20 | 24 | 28 | 32 |
|---|---|---|---|---|---|---|---|---|---|---|
| base | 0.0 | 0.0 | 0.1 | 0.2 | 0.4 | 0.1 | 0.1 | 0.1 | 0.1 | 0.1 |
| sweep | 0.0 | 0.0 | 0.1 | 0.2 | 0.5 | 0.2 | 0.1 | 0.1 | 0.1 | 0.1 |

## spinbench (all nodes): wall/cpu ns per op

Uncontended ns: A_cur=0.92, X_bool_isb_128y64=1.06, H_stdmutex=4.33, O_oiio_spin_mutex=0.91

**spinbench17o3 tiny**

| lock | 1 | 4 | 16 |
|---|---|---|---|
| A_cur | 1.1/1.1 | 12.5/48.6 | 106.1/1151.7 |
| X_bool_isb_128y64 | 1.3/1.3 | 4.9/18.3 | 11.0/124.0 |
| O_oiio_spin_mutex | 1.2/1.2 | 5.8/22.0 | 11.0/110.2 |

**spinbench17o3 medium**

| lock | 1 | 4 | 16 |
|---|---|---|---|
| A_cur | 12.5/12.5 | 15.8/42.7 | 91.9/993.1 |
| X_bool_isb_128y64 | 12.9/12.9 | 16.3/49.7 | 24.7/279.5 |
| O_oiio_spin_mutex | 12.9/13.0 | 15.3/47.4 | 22.2/232.7 |

**spinbench17o3 coloc**

| lock | 1 | 4 | 16 |
|---|---|---|---|
| A_cur | 1.1/1.1 | 3.8/14.6 | 4.5/49.7 |
| X_bool_isb_128y64 | 1.2/1.3 | 1.7/5.3 | 4.5/48.3 |
| O_oiio_spin_mutex | 1.1/1.2 | 1.6/4.5 | 5.8/81.2 |

**spinbench20o3 tiny**

| lock | 1 | 4 | 16 |
|---|---|---|---|
| A_cur | 1.1/1.1 | 23.6/93.4 | 123.2/1367.9 |
| X_bool_isb_128y64 | 1.3/1.4 | 8.0/31.4 | 16.7/163.4 |
| O_oiio_spin_mutex | 1.1/1.1 | 2.8/9.3 | 10.9/111.6 |

**spinbench20o3 medium**

| lock | 1 | 4 | 16 |
|---|---|---|---|
| A_cur | 12.5/12.6 | 15.3/39.9 | 108.7/1255.9 |
| X_bool_isb_128y64 | 12.3/12.3 | 15.9/47.1 | 23.1/241.7 |
| O_oiio_spin_mutex | 13.0/13.0 | 16.1/59.5 | 27.6/372.6 |

**spinbench20o3 coloc**

| lock | 1 | 4 | 16 |
|---|---|---|---|
| A_cur | 1.1/1.1 | 3.1/10.5 | 20.4/228.2 |
| X_bool_isb_128y64 | 1.4/1.4 | 1.6/4.9 | 7.2/102.6 |
| O_oiio_spin_mutex | 1.1/1.1 | 1.6/5.0 | 5.2/67.5 |

## Progress log tail
```
2026-10-07 09:20:50 === done. Summary: /Users/lg/code/oiio/oiio.lg/spinlock-work/results/dryrun-m4-quick/SUMMARY.md
2026-10-07 09:21:33 === run_all.sh RUN=dryrun-m4-quick NCPU=16 QUICK=1 STEPS='ctest'
2026-10-07 09:21:33 variants: base sweep   threads: 1 4 16    WORK=/Users/lg/tmp/claude-501/spinwork
2026-10-07 09:21:33 START ctest on sweep
2026-10-07 09:22:16 END ctest rc=8 : 90% tests passed, 8 tests failed out of 84
2026-10-07 09:22:16 === done. Summary: /Users/lg/code/oiio/oiio.lg/spinlock-work/results/dryrun-m4-quick/SUMMARY.md
2026-10-07 09:24:05 === run_all.sh RUN=dryrun-m4-quick NCPU=16 QUICK=1 STEPS='ctest'
2026-10-07 09:24:05 variants: base sweep   threads: 1 4 16    WORK=/Users/lg/tmp/claude-501/spinwork
2026-10-07 09:24:05 START ctest on sweep
2026-10-07 09:24:39 END ctest rc=8 : 95% tests passed, 4 tests failed out of 88
2026-10-07 09:24:39 === done. Summary: /Users/lg/code/oiio/oiio.lg/spinlock-work/results/dryrun-m4-quick/SUMMARY.md
2026-10-07 10:10:35 === run_all.sh RUN=dryrun-m4-quick NCPU=16 QUICK=1 STEPS='ctest'
2026-10-07 10:10:35 variants: base sweep   threads: 1 4 16    WORK=/Users/lg/tmp/claude-501/spinwork
2026-10-07 10:10:35 START ctest on sweep
2026-10-07 10:10:59 END ctest rc=0 : 100% tests passed out of 84
```
