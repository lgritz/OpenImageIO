# Results: bear0056-20261012-1602-quick

## Machine
<details><summary>machine.txt</summary>

```
## date
Mon Oct 12 04:02:25 PM PDT 2026
## git
d11c9434f30c15fe0a9522e6ea9004609bd8eb2e spinlock-work: update variant SHAs after rebase onto main
## uname
Linux bear0056.spimageworks.com 5.14.0-687.46.1.el9_8.x86_64 #1 SMP PREEMPT_DYNAMIC Thu Sep 10 14:23:01 UTC 2026 x86_64 x86_64 x86_64 GNU/Linux
## nproc
32
## lscpu
Architecture:                            x86_64
CPU op-mode(s):                          32-bit, 64-bit
Address sizes:                           48 bits physical, 48 bits virtual
Byte Order:                              Little Endian
CPU(s):                                  32
On-line CPU(s) list:                     0-31
Vendor ID:                               AuthenticAMD
Model name:                              AMD Ryzen Threadripper PRO 5955WX 16-Cores
CPU family:                              25
Model:                                   8
Thread(s) per core:                      2
Core(s) per socket:                      16
Socket(s):                               1
Stepping:                                2
Frequency boost:                         enabled
CPU(s) scaling MHz:                      91%
CPU max MHz:                             4551.9180
CPU min MHz:                             1800.0000
BogoMIPS:                                7999.43
Virtualization:                          AMD-V
L1d cache:                               512 KiB (16 instances)
L1i cache:                               512 KiB (16 instances)
L2 cache:                                8 MiB (16 instances)
L3 cache:                                64 MiB (2 instances)
NUMA node(s):                            1
NUMA node0 CPU(s):                       0-31
## numa
numactl not available
## smt
1
## governor
performance
## memory
               total        used        free      shared  buff/cache   available
Mem:             125          11          93           0          21         113
Swap:            255           2         253
## compiler
c++ (GCC) 11.5.0 20240719 (Red Hat 11.5.0-14)
Copyright (C) 2021 Free Software Foundation, Inc.
## cmake
cmake version 3.31.11
## python
Python 3.11.4
## perf
no perf
## EXTRA_CMAKE_ARGS

## build CMakeCache (selected)
CMAKE_BUILD_TYPE:STRING=Release
CMAKE_CXX_COMPILER:FILEPATH=/usr/lib64/ccache/c++
CMAKE_CXX_COMPILER_AR:FILEPATH=/usr/bin/gcc-ar
CMAKE_CXX_COMPILER_RANLIB:FILEPATH=/usr/bin/gcc-ranlib
CMAKE_CXX_FLAGS:STRING=
CMAKE_CXX_FLAGS_DEBUG:STRING=-g
CMAKE_CXX_FLAGS_MINSIZEREL:STRING=-Os -DNDEBUG
CMAKE_CXX_FLAGS_RELEASE:STRING=-O3 -DNDEBUG
CMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=-O2 -g -DNDEBUG
CMAKE_CXX_STANDARD:STRING=17
OIIO_BUILD_FUZZ_TARGETS:BOOL=OFF
OIIO_BUILD_PROFILER:BOOL=OFF
OIIO_BUILD_TESTS:BOOL=ON
OIIO_BUILD_TOOLS:BOOL=ON
OIIO_CI:BOOL=1
OIIO_DOWNLOAD_MISSING_TESTDATA:BOOL=OFF
OIIO_HARDENING:STRING=1
OIIO_INNER_NAMESPACE_INCLUDE_PATCH:BOOL=ON
OIIO_INTERNALIZE_FMT:BOOL=ON
OIIO_IV_EXTRA_IV_LIBRARIES:STRING=/spfs/lib/libz.so
OIIO_LIBNAME_SUFFIX:STRING=
OIIO_LOCAL_DEPS_PATH:STRING=/net/soft_scratch/users/lg/code/openimageio-feedstock/oiio/ext/dist
OIIO_LOCAL_TESTDATA_ROOT:PATH=/net/soft_scratch/users/lg/code/oiio
```
</details>

## Pause instruction cost
```
empty 0.000 ns
isb 14.566 ns
yield-insn 14.574 ns
```

## ctest (newest variant)
```
100% tests passed, 0 tests failed out of 1
100% tests passed, 0 tests failed out of 86
```

## testtex --threadtimes (seconds)
Weak scaling (fixed iters/thread): flat = perfect. Strong scaling (quick runs with --iters): halving = perfect.

**cache64 (strong scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.29 | 0.11 | 0.07 | 0.03 | 0.03 | 0.02 | 0.02 | 0.02 |
| spin | 0.27 | 0.11 | 0.07 | 0.03 | 0.03 | 0.02 | 0.02 | 0.02 |
| tileuse | 0.27 | 0.11 | 0.07 | 0.03 | 0.03 | 0.02 | 0.02 | 0.02 |
| fileuse | 0.26 | 0.10 | 0.06 | 0.03 | 0.03 | 0.02 | 0.02 | 0.02 |
| sweep | 0.27 | 0.10 | 0.07 | 0.03 | 0.03 | 0.02 | 0.02 | 0.02 |

Speedup vs base at 32 threads: spin 1.00x, tileuse 1.00x, fileuse 1.00x, sweep 1.00x

**maxfiles16 (strong scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.29 | 0.12 | 0.12 | 0.07 | 0.05 | 0.05 | 0.04 | 0.03 |
| spin | 0.29 | 0.12 | 0.11 | 0.07 | 0.06 | 0.05 | 0.04 | 0.03 |
| tileuse | 0.29 | 0.12 | 0.10 | 0.08 | 0.06 | 0.05 | 0.04 | 0.04 |
| fileuse | 0.60 | 0.12 | 0.12 | 0.06 | 0.06 | 0.05 | 0.05 | 0.03 |
| sweep | 0.29 | 0.12 | 0.10 | 0.07 | 0.05 | 0.04 | 0.04 | 0.04 |

Speedup vs base at 32 threads: spin 1.00x, tileuse 0.75x, fileuse 1.00x, sweep 0.75x

**stats-w1 (weak scaling)**

| variant | 32 |
|---|---|
| base | 1.78 |
| spin | 1.81 |
| tileuse | 1.35 |
| fileuse | 1.03 |
| sweep | 1.03 |

Speedup vs base at 32 threads: spin 0.98x, tileuse 1.32x, fileuse 1.73x, sweep 1.73x

**stats-w7 (weak scaling)**

| variant | 32 |
|---|---|
| base | 2.03 |
| spin | 2.03 |
| tileuse | 2.09 |
| fileuse | 1.86 |
| sweep | 1.83 |

Speedup vs base at 32 threads: spin 1.00x, tileuse 0.97x, fileuse 1.09x, sweep 1.11x

**w1 (strong scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.07 | 0.07 | 0.04 | 0.02 | 0.01 | 0.01 | 0.01 | 0.01 |
| spin | 0.08 | 0.07 | 0.03 | 0.02 | 0.01 | 0.01 | 0.01 | 0.01 |
| tileuse | 0.07 | 0.04 | 0.02 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 |
| fileuse | 0.07 | 0.03 | 0.02 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 |
| sweep | 0.08 | 0.03 | 0.02 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 |

Speedup vs base at 32 threads: spin 1.00x, tileuse 1.00x, fileuse 1.00x, sweep 1.00x

**w7 (strong scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.13 | 0.08 | 0.04 | 0.02 | 0.02 | 0.02 | 0.01 | 0.01 |
| spin | 0.13 | 0.09 | 0.04 | 0.02 | 0.02 | 0.02 | 0.01 | 0.02 |
| tileuse | 0.13 | 0.07 | 0.04 | 0.02 | 0.02 | 0.02 | 0.02 | 0.02 |
| fileuse | 0.13 | 0.08 | 0.04 | 0.02 | 0.02 | 0.02 | 0.01 | 0.02 |
| sweep | 0.13 | 0.07 | 0.04 | 0.02 | 0.02 | 0.02 | 0.02 | 0.02 |

Speedup vs base at 32 threads: spin 0.50x, tileuse 0.50x, fileuse 0.50x, sweep 0.50x

<details><summary>testtex-base-stats-w1.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 1 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 1 created, 1 current, 1 peak
    Total pixel data size of all images referenced : 10.7 MB
    File I/O time : 0.0s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 28 created, 2 current, 28 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=2.155 cpu=58.410 rc=0
```
</details>

<details><summary>testtex-base-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 7 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 10 created, 10 current, 10 peak
    Total pixel data size of all images referenced : 106.7 MB
    File I/O time : 0.4s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 810 created, 800 current, 800 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.392 cpu=65.118 rc=0
```
</details>

<details><summary>testtex-fileuse-stats-w1.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 1 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 1 created, 1 current, 1 peak
    Total pixel data size of all images referenced : 10.7 MB
    File I/O time : 0.0s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 24 created, 2 current, 24 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=1.415 cpu=35.156 rc=0
```
</details>

<details><summary>testtex-fileuse-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 7 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 10 created, 10 current, 10 peak
    Total pixel data size of all images referenced : 106.7 MB
    File I/O time : 0.3s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 814 created, 800 current, 800 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.227 cpu=58.660 rc=0
```
</details>

<details><summary>testtex-spin-stats-w1.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 1 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 1 created, 1 current, 1 peak
    Total pixel data size of all images referenced : 10.7 MB
    File I/O time : 0.0s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.0s
  Tiles: 32 created, 2 current, 31 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=2.190 cpu=58.126 rc=0
```
</details>

<details><summary>testtex-spin-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 7 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 10 created, 10 current, 10 peak
    Total pixel data size of all images referenced : 106.7 MB
    File I/O time : 0.6s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 814 created, 800 current, 800 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.398 cpu=65.302 rc=0
```
</details>

<details><summary>testtex-sweep-stats-w1.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 1 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 1 created, 1 current, 1 peak
    Total pixel data size of all images referenced : 10.7 MB
    File I/O time : 0.0s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 4 created, 2 current, 4 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=1.404 cpu=35.115 rc=0
```
</details>

<details><summary>testtex-sweep-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 7 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 10 created, 10 current, 10 peak
    Total pixel data size of all images referenced : 106.7 MB
    File I/O time : 0.3s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 819 created, 800 current, 800 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.187 cpu=58.939 rc=0
```
</details>

<details><summary>testtex-tileuse-stats-w1.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 1 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 1 created, 1 current, 1 peak
    Total pixel data size of all images referenced : 10.7 MB
    File I/O time : 0.0s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 22 created, 2 current, 22 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=1.790 cpu=43.318 rc=0
```
</details>

<details><summary>testtex-tileuse-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 7 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 10 created, 10 current, 10 peak
    Total pixel data size of all images referenced : 106.7 MB
    File I/O time : 0.3s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 816 created, 800 current, 800 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.456 cpu=63.329 rc=0
```
</details>

## spinlock_test

**program-reported time (s, best of trials)**

| variant | 1 | 4 | 16 | 32 |
|---|---|---|---|---|
| base | 0.0 | 0.2 | 0.6 | 1.0 |
| spin | 0.0 | 0.1 | 0.4 | 0.7 |

**whole process wall/cpu seconds (all trials)**

| variant | 1 | 4 | 16 | 32 |
|---|---|---|---|---|
| base | 0.603/0.586 | 1.012/2.502 | 2.316/28.968 | 3.563/93.086 |
| spin | 0.607/0.580 | 0.885/2.010 | 1.689/18.918 | 2.620/66.545 |

## parallel_test

**parallel_for task launches per second (higher is better)**

| variant | 1 | 4 | 16 | 32 |
|---|---|---|---|---|
| base | 3.06e+07 | 1.02e+08 | 2.55e+08 | 3.36e+08 |
| spin | 3.14e+07 | 1.06e+08 | 2.74e+08 | 3.68e+08 |

## spin_rw_test (strong scaling; seconds, best of 3)

**spin_rw_test --wedge**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 20 | 24 | 28 | 32 | 64 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| base | 0.0 | 0.0 | 0.1 | 0.2 | 0.2 | 0.2 | 0.2 | 0.2 | 0.3 | 0.3 | 0.2 |
| spin | 0.0 | 0.0 | 0.1 | 0.2 | 0.2 | 0.2 | 0.2 | 0.2 | 0.3 | 0.3 | 0.2 |

## spinbench (all nodes): wall/cpu ns per op

Uncontended ns: A_cur=3.10, X_bool_isb_128y64=3.09, H_stdmutex=6.00, O_oiio_spin_mutex=3.10

**spinbench17o3 tiny**

| lock | 1 | 4 | 16 | 32 |
|---|---|---|---|---|
| A_cur | 2.8/2.8 | 8.7/34.4 | 49.8/781.6 | 49.5/1463.8 |
| X_bool_isb_128y64 | 2.8/2.8 | 10.3/39.9 | 29.7/457.0 | 48.8/1500.5 |
| H_stdmutex | 5.9/5.9 | 35.4/124.6 | 33.6/477.2 | 46.5/1370.2 |
| O_oiio_spin_mutex | 2.8/2.8 | 9.1/36.0 | 27.2/418.3 | 47.2/1449.0 |

**spinbench17o3 medium**

| lock | 1 | 4 | 16 | 32 |
|---|---|---|---|---|
| A_cur | 16.9/16.8 | 32.6/126.9 | 69.7/1036.0 | 87.3/2555.0 |
| X_bool_isb_128y64 | 15.9/15.9 | 34.8/134.9 | 62.4/944.6 | 95.3/2952.1 |
| H_stdmutex | 18.4/18.3 | 108.9/384.1 | 110.7/1662.0 | 137.4/4129.4 |
| O_oiio_spin_mutex | 16.0/15.9 | 36.0/139.0 | 62.3/952.2 | 91.0/2806.1 |

**spinbench17o3 coloc**

| lock | 1 | 4 | 16 | 32 |
|---|---|---|---|---|
| A_cur | 3.2/3.2 | 6.1/22.7 | 24.0/362.0 | 42.9/1309.7 |
| X_bool_isb_128y64 | 3.2/3.2 | 8.6/31.1 | 27.3/415.5 | 47.5/1441.7 |
| H_stdmutex | 5.9/5.9 | 22.6/84.7 | 22.8/306.3 | 30.5/898.6 |
| O_oiio_spin_mutex | 3.2/3.2 | 8.7/30.5 | 27.2/411.8 | 46.6/1421.6 |

**spinbench20o3 tiny**

| lock | 1 | 4 | 16 | 32 |
|---|---|---|---|---|
| A_cur | 2.8/2.8 | 9.8/37.5 | 48.3/746.3 | 48.3/1452.4 |
| X_bool_isb_128y64 | 2.8/2.8 | 10.6/41.9 | 29.2/444.8 | 46.1/1415.0 |
| H_stdmutex | 5.9/5.9 | 34.1/125.4 | 33.6/485.1 | 46.2/1300.0 |
| O_oiio_spin_mutex | 2.7/2.8 | 9.8/38.1 | 29.7/454.9 | 48.4/1493.1 |

**spinbench20o3 medium**

| lock | 1 | 4 | 16 | 32 |
|---|---|---|---|---|
| A_cur | 16.9/16.9 | 31.9/124.4 | 72.4/1110.6 | 88.3/2564.3 |
| X_bool_isb_128y64 | 15.9/15.9 | 36.1/142.6 | 62.3/945.5 | 92.9/2872.7 |
| H_stdmutex | 18.4/18.3 | 107.2/375.1 | 117.8/1707.0 | 140.7/4152.3 |
| O_oiio_spin_mutex | 15.9/15.8 | 34.8/135.3 | 62.0/937.3 | 91.5/2822.2 |

**spinbench20o3 coloc**

| lock | 1 | 4 | 16 | 32 |
|---|---|---|---|---|
| A_cur | 3.2/3.2 | 6.5/24.0 | 26.4/399.6 | 43.6/1330.8 |
| X_bool_isb_128y64 | 3.2/3.2 | 8.2/27.5 | 27.8/425.2 | 44.6/1329.3 |
| H_stdmutex | 5.9/5.9 | 23.0/84.9 | 23.9/337.3 | 29.7/876.3 |
| O_oiio_spin_mutex | 3.2/3.2 | 9.1/31.5 | 26.6/409.0 | 45.0/1364.5 |

## Progress log tail
```
2026-10-12 16:05:48 START ctest on sweep
2026-10-12 16:06:05 END ctest rc=0 : 100% tests passed, 0 tests failed out of 86
2026-10-12 16:06:05 START testtex sweep w1
2026-10-12 16:06:05 END testtex sweep w1 wall=0.556 cpu=3.707 rc=0
2026-10-12 16:06:05 START testtex sweep w7
2026-10-12 16:06:06 END testtex sweep w7 wall=0.692 cpu=4.389 rc=0
2026-10-12 16:06:06 START testtex sweep maxfiles16
2026-10-12 16:06:08 END testtex sweep maxfiles16 wall=1.786 cpu=6.798 rc=0
2026-10-12 16:06:08 START testtex sweep cache64
2026-10-12 16:06:09 END testtex sweep cache64 wall=1.579 cpu=6.454 rc=0
2026-10-12 16:06:09 START testtex sweep stats-w1
2026-10-12 16:06:11 END testtex sweep stats-w1 wall=1.404 cpu=35.115 rc=0
2026-10-12 16:06:11 START testtex sweep stats-w7
2026-10-12 16:06:13 END testtex sweep stats-w7 wall=2.187 cpu=58.939 rc=0
2026-10-12 16:06:13 === variant sweep complete
```
