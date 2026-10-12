# Results: dryrun-faketop-quick

## Machine
<details><summary>machine.txt</summary>

```
## date
Mon 12 Oct 2026 15:36:52 PDT
## git
83a1dcb0a966147b4a52d14510fe250670e69e70 spinlock-work: run_all.sh drives TOP/build.sh on the feedstock layout
## uname
Darwin unagi3.local 25.6.0 Darwin Kernel Version 25.6.0: Fri Jul 31 19:17:26 PDT 2026; root:xnu-12377.161.14~5/RELEASE_ARM64_T6041 arm64
## nproc
16
## lscpu
hw.ncpu: 16
hw.physicalcpu: 16
hw.physicalcpu_max: 16
hw.logicalcpu: 16
hw.logicalcpu_max: 16
machdep.cpu.cores_per_package: 16
machdep.cpu.core_count: 16
machdep.cpu.logical_per_package: 16
machdep.cpu.thread_count: 16
machdep.cpu.brand_string: Apple M4 Max
## numa
numactl not available
## smt
n/a
## governor
n/a
## memory
68719476736
## compiler
Apple clang version 21.0.0 (clang-2100.3.34.2)
Target: arm64-apple-darwin25.6.0
## cmake
cmake version 4.4.4
## python
Python 3.9.6
## perf
no perf
## EXTRA_CMAKE_ARGS

## build CMakeCache (selected)
```
</details>

## Pause instruction cost
```
empty 0.000 ns
isb 8.417 ns
yield-insn 0.245 ns
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
| base | 0.15 | 0.09 | 0.07 | 0.07 | 0.06 | 0.06 |
| sweep | 0.15 | 0.09 | 0.07 | 0.08 | 0.06 | 0.06 |

Speedup vs base at 16 threads: sweep 1.00x

**prod-stats-w7 (weak scaling)**

| variant | 16 |
|---|---|
| base | 2.15 |
| sweep | 1.80 |

Speedup vs base at 16 threads: sweep 1.19x

**prod-w7 (strong scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 |
|---|---|---|---|---|---|---|
| base | 0.11 | 0.06 | 0.03 | 0.02 | 0.02 | 0.02 |
| sweep | 0.11 | 0.06 | 0.03 | 0.02 | 0.02 | 0.02 |

Speedup vs base at 16 threads: sweep 1.00x

**stats-w1 (weak scaling)**

| variant | 16 |
|---|---|
| base | 2.74 |
| sweep | 1.20 |

Speedup vs base at 16 threads: sweep 2.28x

**stats-w7 (weak scaling)**

| variant | 16 |
|---|---|
| base | 1.87 |
| sweep | 1.79 |

Speedup vs base at 16 threads: sweep 1.04x

**w1 (strong scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 |
|---|---|---|---|---|---|---|
| base | 0.10 | 0.05 | 0.03 | 0.02 | 0.02 | 0.02 |
| sweep | 0.10 | 0.05 | 0.02 | 0.01 | 0.01 | 0.01 |

Speedup vs base at 16 threads: sweep 2.00x

**w7 (strong scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 |
|---|---|---|---|---|---|---|
| base | 0.12 | 0.06 | 0.04 | 0.03 | 0.02 | 0.02 |
| sweep | 0.12 | 0.06 | 0.04 | 0.02 | 0.02 | 0.02 |

Speedup vs base at 16 threads: sweep 1.00x

<details><summary>testtex-base-prod-stats-w7.txt</summary>

```
$ /Users/lg/tmp/claude-501/faketop/build/bin/testtex --threadtimes 7 --threads 16 --runstats prodtex0 prodtex1
hw threads = 16
threads  time (s)   speedup efficiency
    ImageInputs : 2 created, 2 current, 2 peak
    Total pixel data size of all images referenced : 6.3 MB
    File I/O time : 0.0s (0.0s average per thread, for 17 threads)
    ImageInput mutex locking time : 0.0s
  Tiles: 162 created, 160 current, 160 peak
    micro-cache misses : 59540152 (80.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 2.2 MB
wall=2.213 cpu=31.701 rc=0
```
</details>

<details><summary>testtex-base-stats-w1.txt</summary>

```
$ /Users/lg/tmp/claude-501/faketop/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 1 --threads 16 --runstats
hw threads = 16
threads  time (s)   speedup efficiency
    ImageInputs : 1 created, 1 current, 1 peak
    Total pixel data size of all images referenced : 10.7 MB
    ImageInput mutex locking time : 0.0s
  Tiles: 6 created, 2 current, 6 peak
    micro-cache misses : 32 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=3.025 cpu=43.091 rc=0
```
</details>

<details><summary>testtex-base-stats-w7.txt</summary>

```
$ /Users/lg/tmp/claude-501/faketop/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 7 --threads 16 --runstats
hw threads = 16
threads  time (s)   speedup efficiency
    ImageInputs : 10 created, 10 current, 10 peak
    Total pixel data size of all images referenced : 106.7 MB
    File I/O time : 0.1s (0.0s average per thread, for 17 threads)
    ImageInput mutex locking time : 0.0s
  Tiles: 805 created, 800 current, 800 peak
    micro-cache misses : 64480829 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.170 cpu=29.403 rc=0
```
</details>

<details><summary>testtex-sweep-prod-stats-w7.txt</summary>

```
$ /Users/lg/tmp/claude-501/faketop/build/bin/testtex --threadtimes 7 --threads 16 --runstats prodtex0 prodtex1
hw threads = 16
threads  time (s)   speedup efficiency
    ImageInputs : 2 created, 2 current, 2 peak
    Total pixel data size of all images referenced : 6.3 MB
    File I/O time : 0.0s (0.0s average per thread, for 17 threads)
    ImageInput mutex locking time : 0.0s
  Tiles: 160 created, 160 current, 160 peak
    micro-cache misses : 59540152 (80.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 2.2 MB
wall=1.865 cpu=27.160 rc=0
```
</details>

<details><summary>testtex-sweep-stats-w1.txt</summary>

```
$ /Users/lg/tmp/claude-501/faketop/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 1 --threads 16 --runstats
hw threads = 16
threads  time (s)   speedup efficiency
    ImageInputs : 1 created, 1 current, 1 peak
    Total pixel data size of all images referenced : 10.7 MB
    ImageInput mutex locking time : 0.0s
  Tiles: 3 created, 2 current, 3 peak
    micro-cache misses : 32 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=1.493 cpu=18.612 rc=0
```
</details>

<details><summary>testtex-sweep-stats-w7.txt</summary>

```
$ /Users/lg/tmp/claude-501/faketop/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 7 --threads 16 --runstats
hw threads = 16
threads  time (s)   speedup efficiency
    ImageInputs : 10 created, 10 current, 10 peak
    Total pixel data size of all images referenced : 106.7 MB
    File I/O time : 0.1s (0.0s average per thread, for 17 threads)
    ImageInput mutex locking time : 0.0s
  Tiles: 801 created, 800 current, 800 peak
    micro-cache misses : 64480829 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.075 cpu=27.906 rc=0
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
| base | 0.366/0.362 | 0.579/1.083 | 0.712/4.858 |
| sweep | 0.364/0.362 | 0.467/0.634 | 0.679/4.281 |

## parallel_test

**parallel_for task launches per second (higher is better)**

| variant | 1 | 4 | 16 |
|---|---|---|---|
| base | 6.16e+07 | 2.06e+08 | 5.71e+08 |
| sweep | 6.45e+07 | 2.13e+08 | 6.04e+08 |

## spin_rw_test (strong scaling; seconds, best of 3)

**spin_rw_test --wedge**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 20 | 24 | 28 | 32 |
|---|---|---|---|---|---|---|---|---|---|---|
| base | 0.0 | 0.0 | 0.1 | 0.2 | 0.3 | 0.2 | 0.1 | 0.1 | 0.1 | 0.1 |
| sweep | 0.0 | 0.0 | 0.1 | 0.2 | 0.4 | 0.1 | 0.1 | 0.1 | 0.1 | 0.1 |

## spinbench (all nodes): wall/cpu ns per op

Uncontended ns: A_cur=1.06, X_bool_isb_128y64=1.14, H_stdmutex=4.25, O_oiio_spin_mutex=0.86

**spinbench17o3 tiny**

| lock | 1 | 4 | 16 |
|---|---|---|---|
| A_cur | 1.2/1.2 | 4.8/17.2 | 25.0/248.6 |
| X_bool_isb_128y64 | 1.3/1.3 | 4.4/16.5 | 10.7/116.6 |
| O_oiio_spin_mutex | 1.1/1.2 | 2.9/9.4 | 14.6/135.3 |

**spinbench17o3 medium**

| lock | 1 | 4 | 16 |
|---|---|---|---|
| A_cur | 13.6/13.6 | 16.7/51.8 | 69.8/721.3 |
| X_bool_isb_128y64 | 13.2/13.2 | 16.1/44.8 | 24.0/256.9 |
| O_oiio_spin_mutex | 13.5/13.5 | 15.7/41.3 | 21.2/243.7 |

**spinbench17o3 coloc**

| lock | 1 | 4 | 16 |
|---|---|---|---|
| A_cur | 1.2/1.2 | 5.5/21.3 | 22.5/253.4 |
| X_bool_isb_128y64 | 1.2/1.2 | 1.6/5.1 | 3.9/42.2 |
| O_oiio_spin_mutex | 1.2/1.2 | 1.4/4.3 | 4.6/58.2 |

**spinbench20o3 tiny**

| lock | 1 | 4 | 16 |
|---|---|---|---|
| A_cur | 1.2/1.2 | 2.5/7.1 | 47.8/451.6 |
| X_bool_isb_128y64 | 1.2/1.2 | 2.6/8.5 | 11.4/116.4 |
| O_oiio_spin_mutex | 1.2/1.2 | 2.5/8.4 | 11.7/114.3 |

**spinbench20o3 medium**

| lock | 1 | 4 | 16 |
|---|---|---|---|
| A_cur | 14.1/14.1 | 15.8/42.9 | 21.0/199.8 |
| X_bool_isb_128y64 | 13.2/13.2 | 17.0/51.8 | 23.1/259.4 |
| O_oiio_spin_mutex | 13.2/13.2 | 15.8/44.9 | 23.0/265.2 |

**spinbench20o3 coloc**

| lock | 1 | 4 | 16 |
|---|---|---|---|
| A_cur | 1.3/1.3 | 3.2/11.4 | 16.4/162.5 |
| X_bool_isb_128y64 | 1.4/1.4 | 1.6/5.1 | 6.1/79.3 |
| O_oiio_spin_mutex | 1.2/1.2 | 1.4/4.1 | 3.7/40.6 |

## Progress log tail
```
2026-10-12 15:39:36 START testtex sweep w7
2026-10-12 15:39:36 END testtex sweep w7 wall=0.573 cpu=2.448 rc=0
2026-10-12 15:39:36 START testtex sweep maxfiles16
2026-10-12 15:39:38 END testtex sweep maxfiles16 wall=1.186 cpu=4.360 rc=0
2026-10-12 15:39:38 START testtex sweep cache64
2026-10-12 15:39:39 END testtex sweep cache64 wall=1.056 cpu=3.958 rc=0
2026-10-12 15:39:39 START testtex sweep stats-w1
2026-10-12 15:39:41 END testtex sweep stats-w1 wall=1.493 cpu=18.612 rc=0
2026-10-12 15:39:41 START testtex sweep stats-w7
2026-10-12 15:39:43 END testtex sweep stats-w7 wall=2.075 cpu=27.906 rc=0
2026-10-12 15:39:43 START testtex sweep prod-w7
2026-10-12 15:39:43 END testtex sweep prod-w7 wall=0.318 cpu=0.849 rc=0
2026-10-12 15:39:44 START testtex sweep prod-stats-w7
2026-10-12 15:39:45 END testtex sweep prod-stats-w7 wall=1.865 cpu=27.160 rc=0
2026-10-12 15:39:46 === variant sweep complete
```
