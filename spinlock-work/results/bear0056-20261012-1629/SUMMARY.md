# Results: bear0056-20261012-1629

## Machine
<details><summary>machine.txt</summary>

```
## date
Mon Oct 12 04:29:53 PM PDT 2026
## git
48080a88a5c83b55857737b28d33b3971da60fa9 spinlock-work: results from bear0056-20261012-1602-quick
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
CPU(s) scaling MHz:                      93%
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
Mem:             125          12          53           0          60         113
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
isb 14.556 ns
yield-insn 14.556 ns
```

## ctest (newest variant)
```
100% tests passed, 0 tests failed out of 1
100% tests passed, 0 tests failed out of 86
```

## testtex --threadtimes (seconds)
Weak scaling (fixed iters/thread): flat = perfect. Strong scaling (quick runs with --iters): halving = perfect.

**cache64 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.96 | 1.13 | 1.24 | 1.42 | 1.49 | 1.64 | 1.97 | 2.11 |
| spin | 0.96 | 1.15 | 1.34 | 1.37 | 1.53 | 1.60 | 1.96 | 2.11 |
| tileuse | 0.95 | 0.98 | 1.22 | 1.31 | 1.51 | 1.57 | 1.90 | 2.08 |
| fileuse | 0.95 | 1.09 | 1.17 | 1.30 | 1.40 | 1.45 | 1.74 | 1.92 |
| sweep | 0.95 | 1.08 | 1.14 | 1.25 | 1.37 | 1.48 | 1.78 | 1.95 |

Speedup vs base at 32 threads: spin 1.00x, tileuse 1.01x, fileuse 1.10x, sweep 1.08x

**maxfiles16 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 1.15 | 1.29 | 1.43 | 1.49 | 1.62 | 1.70 | 1.99 | 2.14 |
| spin | 1.16 | 1.32 | 1.46 | 1.46 | 1.52 | 1.70 | 2.00 | 2.14 |
| tileuse | 1.13 | 1.31 | 1.42 | 1.44 | 1.57 | 1.63 | 1.97 | 2.08 |
| fileuse | 1.14 | 1.24 | 1.31 | 1.33 | 1.39 | 1.55 | 1.82 | 1.95 |
| sweep | 1.13 | 1.26 | 1.31 | 1.34 | 1.39 | 1.49 | 1.80 | 1.95 |

Speedup vs base at 32 threads: spin 1.00x, tileuse 1.03x, fileuse 1.10x, sweep 1.10x

**stats-w1 (weak scaling)**

| variant | 32 |
|---|---|
| base | 1.80 |
| spin | 1.74 |
| tileuse | 1.32 |
| fileuse | 1.07 |
| sweep | 1.05 |

Speedup vs base at 32 threads: spin 1.03x, tileuse 1.36x, fileuse 1.68x, sweep 1.71x

**stats-w7 (weak scaling)**

| variant | 32 |
|---|---|
| base | 2.05 |
| spin | 2.05 |
| tileuse | 1.99 |
| fileuse | 1.87 |
| sweep | 1.86 |

Speedup vs base at 32 threads: spin 1.00x, tileuse 1.03x, fileuse 1.10x, sweep 1.10x

**w1 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.66 | 1.33 | 1.41 | 1.39 | 1.48 | 1.49 | 1.59 | 1.82 |
| spin | 0.66 | 1.43 | 1.46 | 1.45 | 1.49 | 1.62 | 1.61 | 3.24 |
| tileuse | 0.66 | 0.88 | 0.92 | 0.92 | 0.93 | 0.97 | 1.18 | 1.27 |
| fileuse | 0.66 | 0.66 | 0.66 | 0.67 | 0.67 | 0.84 | 0.96 | 1.01 |
| sweep | 0.67 | 0.67 | 0.67 | 0.68 | 0.71 | 0.84 | 0.95 | 1.01 |

Speedup vs base at 32 threads: spin 0.56x, tileuse 1.43x, fileuse 1.80x, sweep 1.80x

**w2 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.72 | 1.47 | 1.48 | 1.49 | 1.48 | 1.48 | 1.56 | 1.93 |
| spin | 0.74 | 1.49 | 1.49 | 1.49 | 1.51 | 1.50 | 1.60 | 1.86 |
| tileuse | 0.72 | 0.93 | 0.99 | 0.96 | 0.97 | 1.03 | 1.24 | 1.32 |
| fileuse | 0.73 | 0.73 | 0.73 | 0.73 | 0.74 | 0.78 | 1.01 | 1.08 |
| sweep | 0.73 | 0.73 | 0.74 | 0.74 | 0.74 | 0.78 | 1.00 | 1.08 |

Speedup vs base at 32 threads: spin 1.04x, tileuse 1.46x, fileuse 1.79x, sweep 1.79x

**w4 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.75 | 1.03 | 1.01 | 1.02 | 1.09 | 1.25 | 1.52 | 1.71 |
| spin | 0.76 | 1.01 | 1.00 | 1.08 | 1.13 | 1.20 | 1.52 | 1.65 |
| tileuse | 0.74 | 1.00 | 0.98 | 0.97 | 1.00 | 1.13 | 1.31 | 1.42 |
| fileuse | 0.74 | 0.74 | 0.75 | 0.77 | 0.81 | 0.82 | 1.09 | 1.20 |
| sweep | 0.75 | 0.76 | 0.76 | 0.77 | 0.80 | 0.87 | 1.08 | 1.22 |

Speedup vs base at 32 threads: spin 1.04x, tileuse 1.20x, fileuse 1.43x, sweep 1.40x

**w7 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.90 | 1.13 | 1.18 | 1.33 | 1.49 | 1.57 | 1.92 | 2.08 |
| spin | 0.92 | 1.07 | 1.21 | 1.35 | 1.45 | 1.57 | 1.92 | 2.09 |
| tileuse | 0.89 | 1.12 | 1.16 | 1.30 | 1.39 | 1.51 | 1.87 | 2.02 |
| fileuse | 0.89 | 0.94 | 0.99 | 1.16 | 1.27 | 1.39 | 1.72 | 1.86 |
| sweep | 0.90 | 0.96 | 1.02 | 1.16 | 1.31 | 1.38 | 1.71 | 1.87 |

Speedup vs base at 32 threads: spin 1.00x, tileuse 1.03x, fileuse 1.12x, sweep 1.11x

**w8 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.45 | 0.68 | 0.73 | 0.89 | 1.00 | 1.10 | 1.32 | 1.47 |
| spin | 0.46 | 0.66 | 0.73 | 0.87 | 1.00 | 1.09 | 1.35 | 1.47 |
| tileuse | 0.44 | 0.66 | 0.65 | 0.87 | 0.98 | 1.09 | 1.30 | 1.44 |
| fileuse | 0.45 | 0.52 | 0.55 | 0.68 | 0.81 | 0.93 | 1.11 | 1.26 |
| sweep | 0.45 | 0.51 | 0.56 | 0.68 | 0.82 | 0.92 | 1.11 | 1.26 |

Speedup vs base at 32 threads: spin 1.00x, tileuse 1.02x, fileuse 1.17x, sweep 1.17x

<details><summary>testtex-base-stats-w1.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 1 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 1 created, 1 current, 1 peak
    Total pixel data size of all images referenced : 10.7 MB
    File I/O time : 0.0s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 23 created, 2 current, 23 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=2.188 cpu=58.790 rc=0
```
</details>

<details><summary>testtex-base-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 7 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 10 created, 10 current, 10 peak
    Total pixel data size of all images referenced : 106.7 MB
    File I/O time : 0.3s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 815 created, 800 current, 801 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.412 cpu=65.978 rc=0
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
    ImageInput mutex locking time : 0.0s
  Tiles: 30 created, 2 current, 30 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=1.447 cpu=35.536 rc=0
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
  Tiles: 811 created, 800 current, 800 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.234 cpu=59.798 rc=0
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
  Tiles: 26 created, 2 current, 26 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=2.126 cpu=58.139 rc=0
```
</details>

<details><summary>testtex-spin-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 7 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 10 created, 10 current, 10 peak
    Total pixel data size of all images referenced : 106.7 MB
    File I/O time : 0.3s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 806 created, 800 current, 800 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.425 cpu=65.654 rc=0
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
  Tiles: 5 created, 2 current, 5 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=1.424 cpu=35.425 rc=0
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
  Tiles: 825 created, 800 current, 801 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.225 cpu=58.985 rc=0
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
  Tiles: 23 created, 2 current, 23 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=1.693 cpu=43.375 rc=0
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
  Tiles: 812 created, 800 current, 800 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.349 cpu=64.562 rc=0
```
</details>

## spinlock_test

**program-reported time (s, best of trials)**

| variant | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| base | 0.5 | 1.0 | 1.7 | 3.7 | 6.3 | 10.9 | 10.2 | 9.2 |
| spin | 0.5 | 0.8 | 1.4 | 2.3 | 4.3 | 7.5 | 8.1 | 9.1 |

**whole process wall/cpu seconds (all trials)**

| variant | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| base | 1.929/1.918 | 3.468/6.180 | 6.210/21.864 | 11.777/88.539 | 19.571/296.566 | 33.189/970.053 | 31.708/946.737 | 28.788/868.884 |
| spin | 1.936/1.911 | 3.283/5.780 | 4.807/16.315 | 7.595/52.298 | 13.332/194.804 | 22.905/678.549 | 25.142/768.667 | 28.388/877.347 |

## parallel_test

**parallel_for task launches per second (higher is better)**

| variant | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| base | 3.07e+07 | 5.83e+07 | 1.02e+08 | 1.68e+08 | 2.57e+08 | 3.32e+08 | 3.79e+08 | 3.97e+08 |
| spin | 3.05e+07 | 5.92e+07 | 1.07e+08 | 1.82e+08 | 2.67e+08 | 3.65e+08 | 4.14e+08 | 4.45e+08 |

## spin_rw_test (strong scaling; seconds, best of 3)

**spin_rw_test --wedge**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 20 | 24 | 28 | 32 | 64 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| base | 0.2 | 0.5 | 1.3 | 1.6 | 1.6 | 1.9 | 2.3 | 2.4 | 2.7 | 3.0 | 2.7 |
| spin | 0.2 | 0.4 | 1.3 | 1.5 | 1.8 | 1.9 | 2.2 | 2.4 | 2.7 | 3.0 | 2.6 |

## spinbench (all nodes): wall/cpu ns per op

Uncontended ns: A_cur=3.09, A2_cur_nodclp=3.12, B_flag_isb_16y1=3.09, C_bool_isb_16y1=3.09, D_article=3.09, E_bool_isb_64y64=3.09, X_bool_isb_64y64=3.10, X_bool_isb_64y256=3.09, X_bool_isb_64y1k=3.11, X_bool_isb_128y16=3.10, X_bool_isb_128y64=3.12, X_bool_isb_128y256=3.09, X_bool_isb_32y256=3.09, H_stdmutex=5.96, O_oiio_spin_mutex=3.09

**spinbench17o3 tiny**

| lock | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| A_cur | 2.7/2.6 | 5.0/9.9 | 9.4/35.9 | 19.8/153.8 | 52.1/814.0 | 47.8/1446.9 | 38.9/1148.3 | 19.2/517.4 |
| A2_cur_nodclp | 2.7/2.6 | 4.7/9.3 | 12.2/48.4 | 27.7/218.1 | 56.4/893.1 | 94.8/2875.6 | 69.5/2007.8 | 43.6/1120.0 |
| B_flag_isb_16y1 | 2.7/2.6 | 7.8/15.4 | 15.7/61.3 | 31.1/238.7 | 60.5/918.0 | 70.6/2116.0 | 54.4/1507.4 | 36.6/993.8 |
| C_bool_isb_16y1 | 2.7/2.7 | 7.5/14.6 | 15.3/59.1 | 31.4/244.3 | 63.6/979.6 | 73.5/2235.6 | 57.2/1655.5 | 35.8/969.6 |
| D_article | 2.7/2.6 | 6.9/13.7 | 13.5/52.8 | 25.1/197.0 | 49.6/776.4 | 76.2/2303.1 |  |  |
| E_bool_isb_64y64 | 2.7/2.6 | 7.2/14.3 | 11.2/44.6 | 22.5/176.8 | 51.7/811.9 | 88.0/2761.7 | 87.7/2610.9 | 90.8/2783.4 |
| X_bool_isb_64y64 | 2.7/2.6 | 6.6/13.0 | 13.5/53.1 | 26.0/205.6 | 48.9/768.0 | 83.1/2602.7 | 83.0/2467.6 | 84.3/2512.1 |
| X_bool_isb_64y256 | 2.7/2.6 | 5.9/11.3 | 12.3/47.8 | 26.2/207.7 | 51.6/810.9 | 82.5/2575.0 | 90.2/2767.3 | 104.5/3258.9 |
| X_bool_isb_64y1k | 2.7/2.6 | 6.7/13.1 | 13.7/54.0 | 22.6/175.9 | 50.1/784.2 | 81.9/2544.8 | 92.9/2782.7 | 99.9/3110.4 |
| X_bool_isb_128y16 | 2.7/2.6 | 5.7/11.2 | 10.7/41.7 | 15.7/124.7 | 33.8/534.3 | 73.2/2304.5 | 52.6/1578.5 | 44.8/1265.8 |
| X_bool_isb_128y64 | 2.7/2.6 | 5.2/9.9 | 10.1/39.7 | 16.7/131.4 | 29.2/457.0 | 45.2/1372.7 | 47.5/1434.2 | 48.2/1474.7 |
| X_bool_isb_128y256 | 2.7/2.6 | 5.9/11.7 | 10.1/39.5 | 17.0/133.0 | 30.4/478.8 | 46.2/1438.6 | 49.6/1494.3 | 60.5/1883.6 |
| X_bool_isb_32y256 | 2.7/2.6 | 9.3/18.2 | 21.5/83.5 | 51.8/406.0 | 110.4/1717.4 | 178.7/5536.4 | 206.7/6370.4 | 224.8/7071.0 |
| H_stdmutex | 5.7/5.7 | 21.0/41.1 | 33.7/119.5 | 31.3/228.3 | 33.2/490.3 | 45.6/1392.7 | 44.2/1290.8 | 44.0/1375.9 |
| O_oiio_spin_mutex | 2.7/2.6 | 4.9/9.1 | 9.5/37.0 | 14.9/115.6 | 29.1/453.7 | 47.6/1488.5 | 48.0/1478.3 | 52.8/1645.2 |

**spinbench17o3 medium**

| lock | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| A_cur | 16.8/16.7 | 24.9/48.9 | 31.8/122.2 | 47.2/358.2 | 72.5/1110.6 | 87.2/2541.0 | 75.4/2119.5 | 49.0/1246.3 |
| A2_cur_nodclp | 16.9/16.8 | 24.2/47.5 | 35.3/139.5 | 51.7/394.9 | 75.1/1149.9 | 119.0/3582.7 | 84.6/2380.5 | 58.8/1444.1 |
| B_flag_isb_16y1 | 16.8/16.8 | 33.0/65.0 | 44.4/170.9 | 58.6/449.8 | 85.2/1268.4 | 106.2/3124.2 | 91.1/2526.6 | 64.5/1659.6 |
| C_bool_isb_16y1 | 16.8/16.8 | 32.3/62.9 | 42.5/166.8 | 59.2/455.0 | 84.0/1258.8 | 104.6/2981.1 | 87.3/2424.0 | 60.0/1498.6 |
| D_article | 16.8/16.7 | 32.2/63.8 | 41.3/161.6 | 59.4/459.2 | 83.6/1280.9 | 128.7/3951.6 |  |  |
| E_bool_isb_64y64 | 16.8/16.7 | 29.9/59.0 | 40.0/155.1 | 55.9/425.7 | 81.0/1223.6 | 134.1/4205.5 | 134.9/4107.2 | 136.1/4125.6 |
| X_bool_isb_64y64 | 15.8/15.7 | 18.7/37.0 | 39.2/153.2 | 53.7/409.2 | 80.4/1246.1 | 129.0/4031.2 | 125.9/3828.1 | 131.0/4065.2 |
| X_bool_isb_64y256 | 15.8/15.8 | 30.7/60.3 | 41.5/163.5 | 55.5/432.3 | 80.0/1250.0 | 124.2/3862.6 | 136.7/4145.8 | 157.3/4851.0 |
| X_bool_isb_64y1k | 15.8/15.8 | 32.8/64.6 | 41.7/162.1 | 54.9/421.3 | 80.6/1246.2 | 122.8/3741.6 | 135.9/4159.1 | 160.3/5004.4 |
| X_bool_isb_128y16 | 15.7/15.7 | 28.6/56.6 | 35.6/139.5 | 47.3/370.7 | 66.8/1030.9 | 120.4/3758.8 | 92.1/2686.7 | 78.7/2203.9 |
| X_bool_isb_128y64 | 15.8/15.8 | 28.9/57.2 | 35.8/137.8 | 44.4/343.8 | 60.6/919.3 | 92.3/2815.6 | 92.2/2745.0 | 103.6/3224.0 |
| X_bool_isb_128y256 | 15.8/15.8 | 28.3/55.2 | 35.5/140.3 | 44.1/343.6 | 57.3/875.6 | 90.2/2800.8 | 102.0/3053.5 | 119.4/3672.6 |
| X_bool_isb_32y256 | 15.7/15.7 | 36.4/72.4 | 53.7/211.3 | 79.0/606.7 | 134.6/2057.3 | 206.6/6199.5 | 235.0/7263.3 | 257.7/8011.2 |
| H_stdmutex | 18.1/18.1 | 58.5/110.8 | 107.0/378.3 | 103.2/747.4 | 109.0/1611.7 | 133.3/3946.1 | 128.0/3687.6 | 128.7/4026.3 |
| O_oiio_spin_mutex | 15.8/15.7 | 28.4/55.8 | 35.4/138.7 | 45.2/349.0 | 60.2/930.6 | 90.7/2823.1 | 94.0/2858.0 | 95.8/2970.1 |

**spinbench17o3 coloc**

| lock | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| A_cur | 3.1/3.1 | 4.5/8.7 | 6.0/23.4 | 10.7/81.9 | 24.9/383.1 | 41.8/1285.5 | 31.7/921.9 | 19.7/514.8 |
| A2_cur_nodclp | 3.1/3.1 | 4.5/8.4 | 17.9/69.6 | 26.9/210.0 | 42.4/656.6 | 80.4/2431.9 | 58.0/1581.3 | 44.1/1084.5 |
| B_flag_isb_16y1 | 3.1/3.1 | 8.7/16.9 | 16.4/62.6 | 26.1/196.2 | 40.6/606.7 | 60.1/1826.8 | 47.6/1332.6 | 35.4/872.8 |
| C_bool_isb_16y1 | 3.1/3.1 | 8.5/16.7 | 16.7/65.4 | 25.1/185.1 | 42.8/654.7 | 64.5/1939.4 | 51.4/1430.8 | 36.3/925.4 |
| D_article | 3.1/3.1 | 7.6/14.5 | 14.8/55.5 | 23.6/178.8 | 45.9/702.5 | 70.9/2142.2 |  |  |
| E_bool_isb_64y64 | 3.1/3.1 | 8.4/16.2 | 14.7/55.5 | 24.3/187.0 | 44.3/671.1 | 73.2/2240.8 | 76.8/2343.7 | 79.5/2485.6 |
| X_bool_isb_64y64 | 3.1/3.1 | 5.3/9.6 | 12.2/40.7 | 21.9/169.7 | 43.1/656.0 | 72.2/2206.8 | 76.9/2303.0 | 76.9/2387.6 |
| X_bool_isb_64y256 | 3.1/3.1 | 5.2/9.3 | 10.7/40.5 | 22.2/166.9 | 44.7/685.6 | 70.0/2069.1 | 82.3/2437.4 | 91.2/2845.0 |
| X_bool_isb_64y1k | 3.1/3.1 | 5.2/9.4 | 11.2/39.5 | 21.9/168.6 | 44.7/684.8 | 71.3/2193.9 | 82.4/2556.2 | 96.9/3012.0 |
| X_bool_isb_128y16 | 3.1/3.1 | 4.7/8.4 | 9.3/32.3 | 15.2/111.6 | 28.2/422.6 | 59.0/1826.0 | 47.1/1339.9 | 40.5/1051.6 |
| X_bool_isb_128y64 | 3.1/3.1 | 6.9/13.6 | 9.4/35.2 | 14.9/111.5 | 26.3/390.2 | 43.6/1307.6 | 49.0/1497.9 | 58.0/1790.8 |
| X_bool_isb_128y256 | 3.1/3.1 | 4.6/8.0 | 8.3/28.2 | 15.8/110.3 | 27.1/411.9 | 44.4/1334.0 | 56.3/1693.9 | 56.6/1774.3 |
| X_bool_isb_32y256 | 3.1/3.1 | 6.3/11.9 | 14.7/55.9 | 35.6/272.8 | 84.9/1297.2 | 167.3/5153.9 | 186.6/5770.6 | 213.3/6678.5 |
| H_stdmutex | 5.5/5.5 | 16.5/32.1 | 21.2/79.6 | 21.2/161.2 | 21.1/317.3 | 29.4/906.1 | 29.3/887.1 | 29.4/912.4 |
| O_oiio_spin_mutex | 3.1/3.1 | 5.1/9.2 | 9.0/33.7 | 16.2/117.5 | 27.5/420.5 | 45.4/1367.2 | 48.8/1495.1 | 54.2/1663.7 |

**spinbench20o3 tiny**

| lock | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| A_cur | 2.7/2.6 | 5.1/10.0 | 9.1/35.0 | 20.0/154.1 | 51.5/797.4 | 47.7/1454.0 | 36.8/1076.3 | 19.2/516.7 |
| A2_cur_nodclp | 2.7/2.6 | 4.8/9.4 | 12.3/47.8 | 27.3/213.1 | 56.2/881.0 | 95.1/2880.2 | 68.6/2019.6 | 43.7/1166.4 |
| B_flag_isb_16y1 | 2.7/2.6 | 7.6/14.8 | 15.7/59.9 | 30.1/231.4 | 63.1/958.2 | 71.7/2132.2 | 61.9/1789.2 | 34.9/949.2 |
| C_bool_isb_16y1 | 2.7/2.7 | 7.3/14.1 | 14.6/56.9 | 31.3/245.7 | 65.9/1001.6 | 73.0/2219.3 | 56.8/1612.4 | 36.2/978.2 |
| D_article | 2.7/2.7 | 6.2/12.4 | 13.6/53.4 | 25.4/199.7 | 51.8/810.6 | 76.9/2366.9 |  |  |
| E_bool_isb_64y64 | 2.7/2.6 | 6.8/13.4 | 11.5/45.1 | 24.2/187.5 | 50.4/791.4 | 81.6/2546.4 | 81.5/2389.4 | 85.2/2575.1 |
| X_bool_isb_64y64 | 2.7/2.7 | 6.7/13.4 | 12.4/48.8 | 25.3/198.1 | 50.8/799.5 | 85.5/2666.7 | 85.7/2604.9 | 86.3/2610.0 |
| X_bool_isb_64y256 | 2.7/2.6 | 6.8/13.4 | 13.6/53.6 | 24.5/189.3 | 50.8/792.5 | 83.3/2573.1 | 96.0/2941.6 | 106.0/3319.7 |
| X_bool_isb_64y1k | 2.7/2.6 | 7.2/14.3 | 13.4/52.6 | 25.6/200.5 | 49.4/776.4 | 77.2/2362.6 | 88.3/2720.5 | 99.8/3067.6 |
| X_bool_isb_128y16 | 2.7/2.6 | 6.1/12.1 | 9.5/37.1 | 17.4/136.6 | 36.3/572.0 | 74.1/2324.6 | 58.8/1746.3 | 45.5/1263.7 |
| X_bool_isb_128y64 | 2.7/2.6 | 5.8/11.4 | 9.6/37.4 | 16.9/130.8 | 30.1/476.9 | 46.6/1460.3 | 45.7/1350.5 | 49.2/1537.3 |
| X_bool_isb_128y256 | 2.7/2.6 | 5.7/11.3 | 9.9/39.3 | 15.8/122.1 | 30.6/479.7 | 46.8/1458.7 | 52.5/1601.3 | 54.8/1686.5 |
| X_bool_isb_32y256 | 2.7/2.6 | 8.5/16.6 | 22.2/87.7 | 48.9/382.8 | 113.9/1767.5 | 184.8/5721.5 | 203.7/6284.1 | 244.4/7654.4 |
| H_stdmutex | 5.3/5.3 | 19.0/36.4 | 34.1/121.4 | 31.3/228.7 | 32.1/476.7 | 45.7/1393.2 | 44.5/1340.6 | 44.4/1387.4 |
| F_flagtest_isb_64y16 | 2.7/2.6 | 7.4/14.5 | 13.2/51.7 | 28.2/221.5 | 67.9/1059.8 | 114.4/3548.3 | 85.0/2535.4 | 70.6/1972.1 |
| O_oiio_spin_mutex | 2.7/2.6 | 3.9/7.7 | 9.2/36.5 | 16.4/126.2 | 28.9/454.3 | 48.3/1514.2 | 48.9/1526.4 | 56.4/1761.4 |

**spinbench20o3 medium**

| lock | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| A_cur | 16.7/16.7 | 25.0/49.5 | 32.6/128.8 | 46.1/358.5 | 72.4/1083.3 | 87.4/2528.4 | 77.7/2114.8 | 49.6/1226.5 |
| A2_cur_nodclp | 16.8/16.7 | 24.3/47.9 | 34.0/130.5 | 51.7/402.2 | 75.4/1143.2 | 117.5/3557.0 | 85.9/2394.0 | 59.1/1477.5 |
| B_flag_isb_16y1 | 16.9/16.8 | 31.7/61.2 | 43.9/170.0 | 60.4/460.6 | 85.3/1262.2 | 106.4/3110.6 | 89.4/2464.0 | 59.7/1516.8 |
| C_bool_isb_16y1 | 16.7/16.7 | 33.3/65.8 | 45.0/176.2 | 60.3/459.6 | 83.3/1259.4 | 103.9/2965.3 | 88.6/2504.6 | 59.5/1530.8 |
| D_article | 16.7/16.7 | 31.2/61.5 | 40.5/154.6 | 56.9/442.0 | 81.8/1253.1 | 129.2/3972.2 |  |  |
| E_bool_isb_64y64 | 16.8/16.8 | 30.6/60.5 | 39.5/154.8 | 54.5/423.2 | 82.5/1250.7 | 134.3/4164.1 | 132.4/4027.8 | 138.9/4258.3 |
| X_bool_isb_64y64 | 15.8/15.7 | 32.4/64.4 | 42.1/167.4 | 55.4/427.2 | 78.8/1189.9 | 128.6/3963.7 | 126.9/3801.6 | 132.6/4081.4 |
| X_bool_isb_64y256 | 15.8/15.8 | 29.9/57.7 | 41.7/164.8 | 56.0/429.8 | 78.9/1193.3 | 124.6/3804.0 | 131.5/3941.0 | 160.3/4998.1 |
| X_bool_isb_64y1k | 15.8/15.7 | 32.7/65.1 | 40.7/160.6 | 54.0/415.6 | 79.3/1199.3 | 123.8/3832.4 | 138.8/4200.8 | 164.4/5119.4 |
| X_bool_isb_128y16 | 15.7/15.7 | 28.2/55.8 | 35.5/140.1 | 46.2/356.7 | 64.9/999.1 | 116.9/3662.3 | 88.6/2587.6 | 77.7/2128.5 |
| X_bool_isb_128y64 | 15.8/15.8 | 27.7/54.3 | 34.8/136.6 | 43.8/342.2 | 58.1/877.7 | 92.0/2851.2 | 93.0/2755.7 | 98.4/3061.9 |
| X_bool_isb_128y256 | 15.9/15.8 | 26.4/50.2 | 34.6/136.1 | 44.8/347.5 | 59.4/903.6 | 89.6/2790.1 | 98.8/2938.3 | 108.9/3362.2 |
| X_bool_isb_32y256 | 15.8/15.7 | 37.8/74.6 | 48.8/191.6 | 74.6/568.6 | 128.5/1960.8 | 208.5/6368.1 | 229.9/7128.1 | 266.9/8345.4 |
| H_stdmutex | 18.1/18.1 | 57.0/107.1 | 107.0/379.3 | 103.3/750.0 | 109.1/1604.7 | 135.6/3968.5 | 127.8/3736.4 | 128.6/3973.1 |
| F_flagtest_isb_64y16 | 16.8/16.8 | 31.0/61.2 | 40.1/154.3 | 57.2/440.4 | 92.5/1398.3 | 150.6/4647.0 | 116.1/3404.0 | 99.7/2801.1 |
| O_oiio_spin_mutex | 15.7/15.7 | 28.7/56.5 | 34.8/135.0 | 43.2/332.8 | 60.5/921.7 | 92.0/2875.8 | 91.8/2797.8 | 100.7/3124.3 |

**spinbench20o3 coloc**

| lock | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| A_cur | 3.1/3.1 | 4.5/8.5 | 6.4/24.9 | 10.7/83.3 | 26.3/410.3 | 42.0/1290.6 | 31.4/909.9 | 21.9/576.5 |
| A2_cur_nodclp | 3.1/3.1 | 4.9/9.7 | 17.1/61.6 | 27.6/211.8 | 42.9/667.5 | 79.4/2350.5 | 60.9/1742.3 | 44.0/1058.0 |
| B_flag_isb_16y1 | 3.1/3.1 | 8.0/15.4 | 15.9/61.1 | 25.3/194.1 | 40.2/597.4 | 63.0/1867.6 | 50.3/1448.4 | 37.0/988.3 |
| C_bool_isb_16y1 | 3.1/3.1 | 7.5/14.3 | 14.5/56.8 | 24.8/187.6 | 42.7/650.7 | 65.0/2014.5 | 52.1/1477.4 | 37.8/1000.1 |
| D_article | 3.1/3.1 | 7.9/15.3 | 15.1/58.0 | 24.2/183.5 | 45.0/689.3 | 72.8/2241.1 |  |  |
| E_bool_isb_64y64 | 3.1/3.1 | 7.8/15.1 | 15.0/56.9 | 24.2/184.1 | 45.5/694.5 | 73.0/2249.1 | 78.6/2388.9 | 79.6/2481.6 |
| X_bool_isb_64y64 | 3.1/3.1 | 6.7/13.2 | 11.0/39.2 | 22.3/169.7 | 43.4/659.3 | 71.5/2182.2 | 75.8/2328.6 | 77.2/2404.9 |
| X_bool_isb_64y256 | 3.1/3.1 | 5.1/9.1 | 12.3/39.6 | 21.4/158.5 | 44.1/676.5 | 70.8/2120.8 | 80.4/2424.3 | 96.3/3019.7 |
| X_bool_isb_64y1k | 3.1/3.1 | 5.6/10.6 | 10.4/39.6 | 22.0/167.8 | 44.4/679.0 | 70.7/2170.8 | 82.5/2549.6 | 89.9/2789.2 |
| X_bool_isb_128y16 | 3.1/3.1 | 4.7/8.3 | 9.5/37.0 | 15.3/111.9 | 28.1/421.7 | 57.0/1711.8 | 45.6/1288.7 | 42.6/1156.2 |
| X_bool_isb_128y64 | 3.1/3.1 | 4.7/8.3 | 8.6/29.7 | 15.2/116.2 | 27.1/414.3 | 44.3/1330.3 | 49.7/1506.3 | 54.6/1681.6 |
| X_bool_isb_128y256 | 3.1/3.1 | 4.7/8.4 | 8.4/28.8 | 14.5/103.5 | 26.1/388.4 | 44.1/1317.3 | 50.6/1498.9 | 61.7/1909.2 |
| X_bool_isb_32y256 | 3.1/3.1 | 6.0/11.2 | 15.2/55.5 | 34.2/260.2 | 84.8/1306.5 | 168.6/5136.3 | 187.1/5747.5 | 219.7/6929.8 |
| H_stdmutex | 5.7/5.7 | 17.8/35.3 | 21.3/80.2 | 20.9/157.2 | 21.2/319.9 | 29.5/915.2 | 29.0/875.2 | 29.3/914.4 |
| F_flagtest_isb_64y16 | 3.1/3.1 | 7.7/14.8 | 14.8/56.2 | 24.4/180.8 | 48.8/746.7 | 90.6/2801.8 | 71.3/2079.8 | 62.1/1717.5 |
| O_oiio_spin_mutex | 3.1/3.1 | 5.1/9.3 | 9.3/33.6 | 15.6/108.4 | 27.0/412.6 | 42.3/1270.4 | 46.8/1387.9 | 56.5/1747.5 |

## Progress log tail
```
2026-10-12 17:05:06 START testtex sweep w4
2026-10-12 17:05:28 END testtex sweep w4 wall=21.949 cpu=288.587 rc=0
2026-10-12 17:05:28 START testtex sweep w7
2026-10-12 17:06:00 END testtex sweep w7 wall=31.688 cpu=439.401 rc=0
2026-10-12 17:06:00 START testtex sweep w8
2026-10-12 17:06:20 END testtex sweep w8 wall=19.997 cpu=288.025 rc=0
2026-10-12 17:06:20 START testtex sweep maxfiles16
2026-10-12 17:06:57 END testtex sweep maxfiles16 wall=37.004 cpu=476.915 rc=0
2026-10-12 17:06:57 START testtex sweep cache64
2026-10-12 17:07:32 END testtex sweep cache64 wall=34.723 cpu=467.143 rc=0
2026-10-12 17:07:32 START testtex sweep stats-w1
2026-10-12 17:07:33 END testtex sweep stats-w1 wall=1.424 cpu=35.425 rc=0
2026-10-12 17:07:33 START testtex sweep stats-w7
2026-10-12 17:07:36 END testtex sweep stats-w7 wall=2.225 cpu=58.985 rc=0
2026-10-12 17:07:36 === variant sweep complete
```
