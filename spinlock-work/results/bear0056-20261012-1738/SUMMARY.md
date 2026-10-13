# Results: bear0056-20261012-1738

## Machine
<details><summary>machine.txt</summary>

```
## date
Mon Oct 12 05:38:23 PM PDT 2026
## git
8f6862f026757c4db4fdc6295378545d075b4002 spinlock-work: results from bear0056-20261012-1629
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
CPU(s) scaling MHz:                      95%
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
Mem:             125          12          53           0          60         112
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
isb 14.550 ns
yield-insn 14.553 ns
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
| base | 0.96 | 1.28 | 1.26 | 1.34 | 1.53 | 1.63 | 1.92 | 2.16 |
| spin | 0.97 | 1.23 | 1.22 | 1.41 | 1.52 | 1.61 | 1.96 | 2.14 |
| tileuse | 0.95 | 1.19 | 1.26 | 1.32 | 1.49 | 1.58 | 1.90 | 2.08 |
| fileuse | 0.95 | 1.20 | 1.14 | 1.25 | 1.40 | 1.45 | 1.81 | 1.94 |
| sweep | 0.94 | 1.05 | 1.14 | 1.27 | 1.36 | 1.45 | 1.79 | 1.95 |

Speedup vs base at 32 threads: spin 1.01x, tileuse 1.04x, fileuse 1.11x, sweep 1.11x

**maxfiles16 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 1.16 | 1.30 | 1.46 | 1.50 | 1.53 | 1.70 | 1.98 | 2.15 |
| spin | 1.15 | 1.25 | 1.41 | 1.51 | 1.60 | 1.67 | 2.03 | 2.14 |
| tileuse | 1.14 | 1.36 | 1.45 | 1.39 | 1.56 | 1.64 | 1.94 | 2.08 |
| fileuse | 1.14 | 1.26 | 1.33 | 1.33 | 1.45 | 1.51 | 1.81 | 1.95 |
| sweep | 1.14 | 1.23 | 1.32 | 1.34 | 1.39 | 1.53 | 1.83 | 1.96 |

Speedup vs base at 32 threads: spin 1.00x, tileuse 1.03x, fileuse 1.10x, sweep 1.10x

**prod-stats-w7 (weak scaling)**

| variant | 32 |
|---|---|
| base | 5.00 |
| spin | 4.83 |
| tileuse | 4.84 |
| fileuse | 4.84 |
| sweep | 4.72 |

Speedup vs base at 32 threads: spin 1.04x, tileuse 1.03x, fileuse 1.03x, sweep 1.06x

**prod-w1 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.67 | 1.10 | 1.33 | 1.76 | 1.48 | 1.42 | 1.60 | 1.87 |
| spin | 0.67 | 0.80 | 1.45 | 1.45 | 1.50 | 1.49 | 1.71 | 1.79 |
| tileuse | 0.66 | 0.92 | 0.93 | 0.92 | 0.93 | 0.96 | 1.18 | 1.28 |
| fileuse | 0.66 | 0.66 | 0.66 | 0.69 | 0.75 | 0.79 | 0.95 | 1.01 |
| sweep | 0.67 | 0.67 | 0.67 | 0.68 | 0.72 | 0.83 | 0.94 | 1.05 |

Speedup vs base at 32 threads: spin 1.04x, tileuse 1.46x, fileuse 1.85x, sweep 1.78x

**prod-w4 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.78 | 1.05 | 1.04 | 1.02 | 1.04 | 1.06 | 1.33 | 1.46 |
| spin | 0.79 | 0.94 | 1.06 | 1.02 | 1.02 | 1.17 | 1.39 | 1.47 |
| tileuse | 0.77 | 1.02 | 1.02 | 1.01 | 1.01 | 1.07 | 1.30 | 1.43 |
| fileuse | 0.77 | 0.76 | 0.77 | 0.76 | 0.77 | 0.89 | 1.02 | 1.13 |
| sweep | 0.79 | 0.77 | 0.78 | 0.77 | 0.78 | 0.81 | 1.04 | 1.14 |

Speedup vs base at 32 threads: spin 0.99x, tileuse 1.02x, fileuse 1.29x, sweep 1.28x

**prod-w7 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 3.47 | 3.57 | 3.39 | 2.93 | 3.34 | 3.33 | 3.99 | 4.87 |
| spin | 3.42 | 3.49 | 3.42 | 2.94 | 3.33 | 3.06 | 3.96 | 4.79 |
| tileuse | 3.41 | 3.49 | 3.36 | 2.88 | 3.19 | 3.29 | 3.88 | 4.76 |
| fileuse | 3.43 | 3.35 | 3.25 | 2.77 | 3.10 | 3.25 | 3.64 | 4.54 |
| sweep | 3.43 | 3.38 | 3.22 | 2.72 | 2.74 | 3.16 | 3.81 | 4.70 |

Speedup vs base at 32 threads: spin 1.02x, tileuse 1.02x, fileuse 1.07x, sweep 1.04x

**prod-w8 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 3.02 | 3.08 | 3.00 | 2.50 | 2.39 | 2.89 | 3.33 | 4.09 |
| spin | 2.97 | 3.03 | 2.93 | 2.45 | 2.41 | 2.77 | 3.22 | 4.03 |
| tileuse | 2.97 | 3.00 | 2.87 | 2.41 | 2.78 | 2.77 | 3.23 | 3.99 |
| fileuse | 2.99 | 2.89 | 2.82 | 2.34 | 2.23 | 2.78 | 3.02 | 3.94 |
| sweep | 2.97 | 2.95 | 2.79 | 2.28 | 2.66 | 2.69 | 3.11 | 4.05 |

Speedup vs base at 32 threads: spin 1.01x, tileuse 1.03x, fileuse 1.04x, sweep 1.01x

**stats-w1 (weak scaling)**

| variant | 32 |
|---|---|
| base | 1.80 |
| spin | 1.77 |
| tileuse | 1.34 |
| fileuse | 1.07 |
| sweep | 1.06 |

Speedup vs base at 32 threads: spin 1.02x, tileuse 1.34x, fileuse 1.68x, sweep 1.70x

**stats-w7 (weak scaling)**

| variant | 32 |
|---|---|
| base | 2.03 |
| spin | 2.05 |
| tileuse | 1.98 |
| fileuse | 1.88 |
| sweep | 1.85 |

Speedup vs base at 32 threads: spin 0.99x, tileuse 1.03x, fileuse 1.08x, sweep 1.10x

**w1 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.66 | 1.20 | 1.41 | 1.45 | 1.46 | 1.48 | 1.51 | 1.80 |
| spin | 0.67 | 1.41 | 1.45 | 1.40 | 1.43 | 1.46 | 1.58 | 1.75 |
| tileuse | 0.66 | 0.92 | 0.92 | 0.92 | 0.93 | 1.02 | 1.19 | 1.30 |
| fileuse | 0.68 | 0.68 | 0.68 | 0.69 | 0.71 | 0.87 | 0.95 | 1.02 |
| sweep | 0.66 | 0.66 | 0.67 | 0.68 | 0.70 | 0.70 | 0.94 | 1.00 |

Speedup vs base at 32 threads: spin 1.03x, tileuse 1.38x, fileuse 1.76x, sweep 1.80x

**w2 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.72 | 1.44 | 1.43 | 1.46 | 1.51 | 1.46 | 1.53 | 1.79 |
| spin | 0.74 | 1.40 | 1.48 | 1.50 | 1.49 | 1.41 | 1.55 | 2.06 |
| tileuse | 0.73 | 0.97 | 0.95 | 0.95 | 0.96 | 1.04 | 1.23 | 1.31 |
| fileuse | 0.73 | 0.72 | 0.73 | 0.73 | 0.74 | 0.79 | 0.98 | 1.06 |
| sweep | 0.73 | 0.73 | 0.74 | 0.74 | 0.74 | 0.88 | 1.02 | 1.08 |

Speedup vs base at 32 threads: spin 0.87x, tileuse 1.37x, fileuse 1.69x, sweep 1.66x

**w4 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.75 | 1.01 | 1.01 | 1.01 | 1.12 | 1.24 | 1.54 | 1.64 |
| spin | 0.76 | 1.00 | 0.99 | 1.03 | 1.11 | 1.26 | 1.48 | 1.67 |
| tileuse | 0.75 | 1.00 | 1.00 | 0.99 | 1.02 | 1.06 | 1.33 | 1.48 |
| fileuse | 0.75 | 0.75 | 0.75 | 0.76 | 0.79 | 0.91 | 1.12 | 1.22 |
| sweep | 0.75 | 0.75 | 0.76 | 0.77 | 0.80 | 0.86 | 1.10 | 1.23 |

Speedup vs base at 32 threads: spin 0.98x, tileuse 1.11x, fileuse 1.34x, sweep 1.33x

**w7 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.90 | 1.13 | 1.20 | 1.34 | 1.49 | 1.55 | 1.91 | 2.07 |
| spin | 0.91 | 1.12 | 1.20 | 1.33 | 1.43 | 1.59 | 1.91 | 2.06 |
| tileuse | 0.89 | 1.10 | 1.18 | 1.29 | 1.41 | 1.54 | 1.83 | 2.02 |
| fileuse | 0.90 | 0.97 | 1.01 | 1.15 | 1.30 | 1.35 | 1.73 | 1.85 |
| sweep | 0.89 | 0.98 | 1.01 | 1.15 | 1.27 | 1.37 | 1.71 | 1.87 |

Speedup vs base at 32 threads: spin 1.00x, tileuse 1.02x, fileuse 1.12x, sweep 1.11x

**w8 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.45 | 0.69 | 0.72 | 0.86 | 1.00 | 1.10 | 1.31 | 1.47 |
| spin | 0.45 | 0.67 | 0.71 | 0.92 | 1.00 | 1.10 | 1.33 | 1.47 |
| tileuse | 0.45 | 0.66 | 0.72 | 0.85 | 0.93 | 1.04 | 1.27 | 1.42 |
| fileuse | 0.44 | 0.50 | 0.55 | 0.72 | 0.82 | 0.92 | 1.11 | 1.27 |
| sweep | 0.45 | 0.50 | 0.55 | 0.68 | 0.82 | 0.90 | 1.13 | 1.29 |

Speedup vs base at 32 threads: spin 1.00x, tileuse 1.04x, fileuse 1.16x, sweep 1.14x

<details><summary>testtex-base-prod-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --threadtimes 7 --threads 32 --runstats prodtex0 prodtex1 prodtex2 prodtex3 prodtex4 prodtex5 prodtex6 prodtex7 prodtex8 prodtex9 prodtex10 prodtex11 prodtex12 prodtex13 prodtex14 prodtex15 prodtex16 prodtex17 prodtex18 prodtex19 prodtex20 prodtex21 prodtex22 prodtex23 prodtex24 prodtex25 prodtex26 prodtex27 prodtex28 prodtex29 prodtex30 prodtex31 prodtex32 prodtex33 prodtex34 prodtex35 prodtex36 prodtex37 prodtex38 prodtex39 prodtex40 prodtex41 prodtex42 prodtex43 prodtex44 prodtex45 prodtex46 prodtex47 prodtex48 prodtex49 prodtex50 prodtex51 prodtex52 prodtex53 prodtex54 prodtex55 prodtex56 prodtex57 prodtex58 prodtex59 prodtex60 prodtex61 prodtex62 prodtex63 prodtex64 prodtex65 prodtex66 prodtex67 prodtex68 prodtex69 prodtex70 prodtex71 prodtex72 prodtex73 prodtex74 prodtex75 prodtex76 prodtex77 prodtex78 prodtex79 prodtex80 prodtex81 prodtex82 prodtex83 prodtex84 prodtex85 prodtex86 prodtex87 prodtex88 prodtex89 prodtex90 prodtex91 prodtex92 prodtex93 prodtex94 prodtex95 prodtex96 prodtex97 prodtex98 prodtex99 prodtex100 prodtex101 prodtex102 prodtex103 prodtex104 prodtex105 prodtex106 prodtex107 prodtex108 prodtex109 prodtex110 prodtex111 prodtex112 prodtex113 prodtex114 prodtex115 prodtex116 prodtex117 prodtex118 prodtex119 prodtex120 prodtex121 prodtex122 prodtex123 prodtex124 prodtex125 prodtex126 prodtex127 prodtex128 prodtex129 prodtex130 prodtex131 prodtex132 prodtex133 prodtex134 prodtex135 prodtex136 prodtex137 prodtex138 prodtex139 prodtex140 prodtex141 prodtex142 prodtex143 prodtex144 prodtex145 prodtex146 prodtex147 prodtex148 prodtex149 prodtex150 prodtex151 prodtex152 prodtex153 prodtex154 prodtex155 prodtex156 prodtex157 prodtex158 prodtex159 prodtex160 prodtex161 prodtex162 prodtex163 prodtex164 prodtex165 prodtex166 prodtex167 prodtex168 prodtex169 prodtex170 prodtex171 prodtex172 prodtex173 prodtex174 prodtex175 prodtex176 prodtex177 prodtex178 prodtex179 prodtex180 prodtex181 prodtex182 prodtex183 prodtex184 prodtex185 prodtex186 prodtex187 prodtex188 prodtex189 prodtex190 prodtex191 prodtex192 prodtex193 prodtex194 prodtex195 prodtex196 prodtex197 prodtex198 prodtex199 prodtex200 prodtex201 prodtex202 prodtex203 prodtex204 prodtex205 prodtex206 prodtex207 prodtex208 prodtex209 prodtex210 prodtex211 prodtex212 prodtex213 prodtex214 prodtex215 prodtex216 prodtex217 prodtex218 prodtex219 prodtex220 prodtex221 prodtex222 prodtex223 prodtex224 prodtex225 prodtex226 prodtex227 prodtex228 prodtex229 prodtex230 prodtex231 prodtex232 prodtex233 prodtex234 prodtex235 prodtex236 prodtex237 prodtex238 prodtex239 prodtex240 prodtex241 prodtex242 prodtex243 prodtex244 prodtex245 prodtex246 prodtex247 prodtex248 prodtex249 prodtex250 prodtex251 prodtex252 prodtex253 prodtex254 prodtex255 prodtex256 prodtex257 prodtex258 prodtex259 prodtex260 prodtex261 prodtex262 prodtex263 prodtex264 prodtex265 prodtex266 prodtex267 prodtex268 prodtex269 prodtex270 prodtex271 prodtex272 prodtex273 prodtex274 prodtex275 prodtex276 prodtex277 prodtex278 prodtex279 prodtex280 prodtex281 prodtex282 prodtex283 prodtex284 prodtex285 prodtex286 prodtex287 prodtex288 prodtex289 prodtex290 prodtex291 prodtex292 prodtex293 prodtex294 prodtex295 prodtex296 prodtex297 prodtex298 prodtex299 prodtex300 prodtex301 prodtex302 prodtex303 prodtex304 prodtex305 prodtex306 prodtex307 prodtex308 prodtex309 prodtex310 prodtex311 prodtex312 prodtex313 prodtex314 prodtex315 prodtex316 prodtex317 prodtex318 prodtex319 prodtex320 prodtex321 prodtex322 prodtex323 prodtex324 prodtex325 prodtex326 prodtex327 prodtex328 prodtex329 prodtex330 prodtex331 prodtex332 prodtex333 prodtex334 prodtex335 prodtex336 prodtex337 prodtex338 prodtex339 prodtex340 prodtex341 prodtex342 prodtex343 prodtex344 prodtex345 prodtex346 prodtex347 prodtex348 prodtex349 prodtex350 prodtex351 prodtex352 prodtex353 prodtex354 prodtex355 prodtex356 prodtex357 prodtex358 prodtex359 prodtex360 prodtex361 prodtex362 prodtex363 prodtex364 prodtex365 prodtex366 prodtex367 prodtex368 prodtex369 prodtex370 prodtex371 prodtex372 prodtex373 prodtex374 prodtex375 prodtex376 prodtex377 prodtex378 prodtex379 prodtex380 prodtex381 prodtex382 prodtex383 prodtex384 prodtex385 prodtex386 prodtex387 prodtex388 prodtex389 prodtex390 prodtex391
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 17825 created, 100 current, 336 peak
    Total pixel data size of all images referenced : 49.0 GB
    File I/O time : 1m 43.7s (3.1s average per thread, for 33 threads)
    ImageInput mutex locking time : 1m 2.7s
  Tiles: 153369 created, 43633 current, 43781 peak
    micro-cache misses : 132721844 (97.1%)
    redundant reads: 50906 tiles, 1.2 GB
    Peak cache memory : 1024.0 MB
  Top files by redundant I/O:
wall=5.127 cpu=82.833 rc=0
```
</details>

<details><summary>testtex-base-stats-w1.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 1 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 1 created, 1 current, 1 peak
    Total pixel data size of all images referenced : 10.7 MB
    File I/O time : 0.0s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 29 created, 2 current, 29 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=2.183 cpu=58.391 rc=0
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
  Tiles: 826 created, 800 current, 800 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.391 cpu=65.807 rc=0
```
</details>

<details><summary>testtex-fileuse-prod-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --threadtimes 7 --threads 32 --runstats prodtex0 prodtex1 prodtex2 prodtex3 prodtex4 prodtex5 prodtex6 prodtex7 prodtex8 prodtex9 prodtex10 prodtex11 prodtex12 prodtex13 prodtex14 prodtex15 prodtex16 prodtex17 prodtex18 prodtex19 prodtex20 prodtex21 prodtex22 prodtex23 prodtex24 prodtex25 prodtex26 prodtex27 prodtex28 prodtex29 prodtex30 prodtex31 prodtex32 prodtex33 prodtex34 prodtex35 prodtex36 prodtex37 prodtex38 prodtex39 prodtex40 prodtex41 prodtex42 prodtex43 prodtex44 prodtex45 prodtex46 prodtex47 prodtex48 prodtex49 prodtex50 prodtex51 prodtex52 prodtex53 prodtex54 prodtex55 prodtex56 prodtex57 prodtex58 prodtex59 prodtex60 prodtex61 prodtex62 prodtex63 prodtex64 prodtex65 prodtex66 prodtex67 prodtex68 prodtex69 prodtex70 prodtex71 prodtex72 prodtex73 prodtex74 prodtex75 prodtex76 prodtex77 prodtex78 prodtex79 prodtex80 prodtex81 prodtex82 prodtex83 prodtex84 prodtex85 prodtex86 prodtex87 prodtex88 prodtex89 prodtex90 prodtex91 prodtex92 prodtex93 prodtex94 prodtex95 prodtex96 prodtex97 prodtex98 prodtex99 prodtex100 prodtex101 prodtex102 prodtex103 prodtex104 prodtex105 prodtex106 prodtex107 prodtex108 prodtex109 prodtex110 prodtex111 prodtex112 prodtex113 prodtex114 prodtex115 prodtex116 prodtex117 prodtex118 prodtex119 prodtex120 prodtex121 prodtex122 prodtex123 prodtex124 prodtex125 prodtex126 prodtex127 prodtex128 prodtex129 prodtex130 prodtex131 prodtex132 prodtex133 prodtex134 prodtex135 prodtex136 prodtex137 prodtex138 prodtex139 prodtex140 prodtex141 prodtex142 prodtex143 prodtex144 prodtex145 prodtex146 prodtex147 prodtex148 prodtex149 prodtex150 prodtex151 prodtex152 prodtex153 prodtex154 prodtex155 prodtex156 prodtex157 prodtex158 prodtex159 prodtex160 prodtex161 prodtex162 prodtex163 prodtex164 prodtex165 prodtex166 prodtex167 prodtex168 prodtex169 prodtex170 prodtex171 prodtex172 prodtex173 prodtex174 prodtex175 prodtex176 prodtex177 prodtex178 prodtex179 prodtex180 prodtex181 prodtex182 prodtex183 prodtex184 prodtex185 prodtex186 prodtex187 prodtex188 prodtex189 prodtex190 prodtex191 prodtex192 prodtex193 prodtex194 prodtex195 prodtex196 prodtex197 prodtex198 prodtex199 prodtex200 prodtex201 prodtex202 prodtex203 prodtex204 prodtex205 prodtex206 prodtex207 prodtex208 prodtex209 prodtex210 prodtex211 prodtex212 prodtex213 prodtex214 prodtex215 prodtex216 prodtex217 prodtex218 prodtex219 prodtex220 prodtex221 prodtex222 prodtex223 prodtex224 prodtex225 prodtex226 prodtex227 prodtex228 prodtex229 prodtex230 prodtex231 prodtex232 prodtex233 prodtex234 prodtex235 prodtex236 prodtex237 prodtex238 prodtex239 prodtex240 prodtex241 prodtex242 prodtex243 prodtex244 prodtex245 prodtex246 prodtex247 prodtex248 prodtex249 prodtex250 prodtex251 prodtex252 prodtex253 prodtex254 prodtex255 prodtex256 prodtex257 prodtex258 prodtex259 prodtex260 prodtex261 prodtex262 prodtex263 prodtex264 prodtex265 prodtex266 prodtex267 prodtex268 prodtex269 prodtex270 prodtex271 prodtex272 prodtex273 prodtex274 prodtex275 prodtex276 prodtex277 prodtex278 prodtex279 prodtex280 prodtex281 prodtex282 prodtex283 prodtex284 prodtex285 prodtex286 prodtex287 prodtex288 prodtex289 prodtex290 prodtex291 prodtex292 prodtex293 prodtex294 prodtex295 prodtex296 prodtex297 prodtex298 prodtex299 prodtex300 prodtex301 prodtex302 prodtex303 prodtex304 prodtex305 prodtex306 prodtex307 prodtex308 prodtex309 prodtex310 prodtex311 prodtex312 prodtex313 prodtex314 prodtex315 prodtex316 prodtex317 prodtex318 prodtex319 prodtex320 prodtex321 prodtex322 prodtex323 prodtex324 prodtex325 prodtex326 prodtex327 prodtex328 prodtex329 prodtex330 prodtex331 prodtex332 prodtex333 prodtex334 prodtex335 prodtex336 prodtex337 prodtex338 prodtex339 prodtex340 prodtex341 prodtex342 prodtex343 prodtex344 prodtex345 prodtex346 prodtex347 prodtex348 prodtex349 prodtex350 prodtex351 prodtex352 prodtex353 prodtex354 prodtex355 prodtex356 prodtex357 prodtex358 prodtex359 prodtex360 prodtex361 prodtex362 prodtex363 prodtex364 prodtex365 prodtex366 prodtex367 prodtex368 prodtex369 prodtex370 prodtex371 prodtex372 prodtex373 prodtex374 prodtex375 prodtex376 prodtex377 prodtex378 prodtex379 prodtex380 prodtex381 prodtex382 prodtex383 prodtex384 prodtex385 prodtex386 prodtex387 prodtex388 prodtex389 prodtex390 prodtex391
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 19262 created, 100 current, 328 peak
    Total pixel data size of all images referenced : 49.0 GB
    File I/O time : 1m 43.0s (3.1s average per thread, for 33 threads)
    ImageInput mutex locking time : 60.0s
  Tiles: 152928 created, 43633 current, 43786 peak
    micro-cache misses : 132721844 (97.1%)
    redundant reads: 50465 tiles, 1.2 GB
    Peak cache memory : 1024.0 MB
  Top files by redundant I/O:
wall=4.959 cpu=80.963 rc=0
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
  Tiles: 23 created, 2 current, 23 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=1.458 cpu=35.274 rc=0
```
</details>

<details><summary>testtex-fileuse-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 7 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 10 created, 10 current, 10 peak
    Total pixel data size of all images referenced : 106.7 MB
    File I/O time : 0.7s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 820 created, 800 current, 800 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.244 cpu=60.410 rc=0
```
</details>

<details><summary>testtex-spin-prod-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --threadtimes 7 --threads 32 --runstats prodtex0 prodtex1 prodtex2 prodtex3 prodtex4 prodtex5 prodtex6 prodtex7 prodtex8 prodtex9 prodtex10 prodtex11 prodtex12 prodtex13 prodtex14 prodtex15 prodtex16 prodtex17 prodtex18 prodtex19 prodtex20 prodtex21 prodtex22 prodtex23 prodtex24 prodtex25 prodtex26 prodtex27 prodtex28 prodtex29 prodtex30 prodtex31 prodtex32 prodtex33 prodtex34 prodtex35 prodtex36 prodtex37 prodtex38 prodtex39 prodtex40 prodtex41 prodtex42 prodtex43 prodtex44 prodtex45 prodtex46 prodtex47 prodtex48 prodtex49 prodtex50 prodtex51 prodtex52 prodtex53 prodtex54 prodtex55 prodtex56 prodtex57 prodtex58 prodtex59 prodtex60 prodtex61 prodtex62 prodtex63 prodtex64 prodtex65 prodtex66 prodtex67 prodtex68 prodtex69 prodtex70 prodtex71 prodtex72 prodtex73 prodtex74 prodtex75 prodtex76 prodtex77 prodtex78 prodtex79 prodtex80 prodtex81 prodtex82 prodtex83 prodtex84 prodtex85 prodtex86 prodtex87 prodtex88 prodtex89 prodtex90 prodtex91 prodtex92 prodtex93 prodtex94 prodtex95 prodtex96 prodtex97 prodtex98 prodtex99 prodtex100 prodtex101 prodtex102 prodtex103 prodtex104 prodtex105 prodtex106 prodtex107 prodtex108 prodtex109 prodtex110 prodtex111 prodtex112 prodtex113 prodtex114 prodtex115 prodtex116 prodtex117 prodtex118 prodtex119 prodtex120 prodtex121 prodtex122 prodtex123 prodtex124 prodtex125 prodtex126 prodtex127 prodtex128 prodtex129 prodtex130 prodtex131 prodtex132 prodtex133 prodtex134 prodtex135 prodtex136 prodtex137 prodtex138 prodtex139 prodtex140 prodtex141 prodtex142 prodtex143 prodtex144 prodtex145 prodtex146 prodtex147 prodtex148 prodtex149 prodtex150 prodtex151 prodtex152 prodtex153 prodtex154 prodtex155 prodtex156 prodtex157 prodtex158 prodtex159 prodtex160 prodtex161 prodtex162 prodtex163 prodtex164 prodtex165 prodtex166 prodtex167 prodtex168 prodtex169 prodtex170 prodtex171 prodtex172 prodtex173 prodtex174 prodtex175 prodtex176 prodtex177 prodtex178 prodtex179 prodtex180 prodtex181 prodtex182 prodtex183 prodtex184 prodtex185 prodtex186 prodtex187 prodtex188 prodtex189 prodtex190 prodtex191 prodtex192 prodtex193 prodtex194 prodtex195 prodtex196 prodtex197 prodtex198 prodtex199 prodtex200 prodtex201 prodtex202 prodtex203 prodtex204 prodtex205 prodtex206 prodtex207 prodtex208 prodtex209 prodtex210 prodtex211 prodtex212 prodtex213 prodtex214 prodtex215 prodtex216 prodtex217 prodtex218 prodtex219 prodtex220 prodtex221 prodtex222 prodtex223 prodtex224 prodtex225 prodtex226 prodtex227 prodtex228 prodtex229 prodtex230 prodtex231 prodtex232 prodtex233 prodtex234 prodtex235 prodtex236 prodtex237 prodtex238 prodtex239 prodtex240 prodtex241 prodtex242 prodtex243 prodtex244 prodtex245 prodtex246 prodtex247 prodtex248 prodtex249 prodtex250 prodtex251 prodtex252 prodtex253 prodtex254 prodtex255 prodtex256 prodtex257 prodtex258 prodtex259 prodtex260 prodtex261 prodtex262 prodtex263 prodtex264 prodtex265 prodtex266 prodtex267 prodtex268 prodtex269 prodtex270 prodtex271 prodtex272 prodtex273 prodtex274 prodtex275 prodtex276 prodtex277 prodtex278 prodtex279 prodtex280 prodtex281 prodtex282 prodtex283 prodtex284 prodtex285 prodtex286 prodtex287 prodtex288 prodtex289 prodtex290 prodtex291 prodtex292 prodtex293 prodtex294 prodtex295 prodtex296 prodtex297 prodtex298 prodtex299 prodtex300 prodtex301 prodtex302 prodtex303 prodtex304 prodtex305 prodtex306 prodtex307 prodtex308 prodtex309 prodtex310 prodtex311 prodtex312 prodtex313 prodtex314 prodtex315 prodtex316 prodtex317 prodtex318 prodtex319 prodtex320 prodtex321 prodtex322 prodtex323 prodtex324 prodtex325 prodtex326 prodtex327 prodtex328 prodtex329 prodtex330 prodtex331 prodtex332 prodtex333 prodtex334 prodtex335 prodtex336 prodtex337 prodtex338 prodtex339 prodtex340 prodtex341 prodtex342 prodtex343 prodtex344 prodtex345 prodtex346 prodtex347 prodtex348 prodtex349 prodtex350 prodtex351 prodtex352 prodtex353 prodtex354 prodtex355 prodtex356 prodtex357 prodtex358 prodtex359 prodtex360 prodtex361 prodtex362 prodtex363 prodtex364 prodtex365 prodtex366 prodtex367 prodtex368 prodtex369 prodtex370 prodtex371 prodtex372 prodtex373 prodtex374 prodtex375 prodtex376 prodtex377 prodtex378 prodtex379 prodtex380 prodtex381 prodtex382 prodtex383 prodtex384 prodtex385 prodtex386 prodtex387 prodtex388 prodtex389 prodtex390 prodtex391
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 15075 created, 100 current, 313 peak
    Total pixel data size of all images referenced : 49.0 GB
    File I/O time : 1m 41.5s (3.1s average per thread, for 33 threads)
    ImageInput mutex locking time : 1m 4.9s
  Tiles: 153401 created, 43633 current, 43809 peak
    micro-cache misses : 132721844 (97.1%)
    redundant reads: 50938 tiles, 1.2 GB
    Peak cache memory : 1024.0 MB
  Top files by redundant I/O:
wall=4.963 cpu=80.124 rc=0
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
    ImageInput mutex locking time : 0.1s
  Tiles: 25 created, 2 current, 25 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=2.150 cpu=58.292 rc=0
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
  Tiles: 812 created, 800 current, 800 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.416 cpu=65.970 rc=0
```
</details>

<details><summary>testtex-sweep-prod-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --threadtimes 7 --threads 32 --runstats prodtex0 prodtex1 prodtex2 prodtex3 prodtex4 prodtex5 prodtex6 prodtex7 prodtex8 prodtex9 prodtex10 prodtex11 prodtex12 prodtex13 prodtex14 prodtex15 prodtex16 prodtex17 prodtex18 prodtex19 prodtex20 prodtex21 prodtex22 prodtex23 prodtex24 prodtex25 prodtex26 prodtex27 prodtex28 prodtex29 prodtex30 prodtex31 prodtex32 prodtex33 prodtex34 prodtex35 prodtex36 prodtex37 prodtex38 prodtex39 prodtex40 prodtex41 prodtex42 prodtex43 prodtex44 prodtex45 prodtex46 prodtex47 prodtex48 prodtex49 prodtex50 prodtex51 prodtex52 prodtex53 prodtex54 prodtex55 prodtex56 prodtex57 prodtex58 prodtex59 prodtex60 prodtex61 prodtex62 prodtex63 prodtex64 prodtex65 prodtex66 prodtex67 prodtex68 prodtex69 prodtex70 prodtex71 prodtex72 prodtex73 prodtex74 prodtex75 prodtex76 prodtex77 prodtex78 prodtex79 prodtex80 prodtex81 prodtex82 prodtex83 prodtex84 prodtex85 prodtex86 prodtex87 prodtex88 prodtex89 prodtex90 prodtex91 prodtex92 prodtex93 prodtex94 prodtex95 prodtex96 prodtex97 prodtex98 prodtex99 prodtex100 prodtex101 prodtex102 prodtex103 prodtex104 prodtex105 prodtex106 prodtex107 prodtex108 prodtex109 prodtex110 prodtex111 prodtex112 prodtex113 prodtex114 prodtex115 prodtex116 prodtex117 prodtex118 prodtex119 prodtex120 prodtex121 prodtex122 prodtex123 prodtex124 prodtex125 prodtex126 prodtex127 prodtex128 prodtex129 prodtex130 prodtex131 prodtex132 prodtex133 prodtex134 prodtex135 prodtex136 prodtex137 prodtex138 prodtex139 prodtex140 prodtex141 prodtex142 prodtex143 prodtex144 prodtex145 prodtex146 prodtex147 prodtex148 prodtex149 prodtex150 prodtex151 prodtex152 prodtex153 prodtex154 prodtex155 prodtex156 prodtex157 prodtex158 prodtex159 prodtex160 prodtex161 prodtex162 prodtex163 prodtex164 prodtex165 prodtex166 prodtex167 prodtex168 prodtex169 prodtex170 prodtex171 prodtex172 prodtex173 prodtex174 prodtex175 prodtex176 prodtex177 prodtex178 prodtex179 prodtex180 prodtex181 prodtex182 prodtex183 prodtex184 prodtex185 prodtex186 prodtex187 prodtex188 prodtex189 prodtex190 prodtex191 prodtex192 prodtex193 prodtex194 prodtex195 prodtex196 prodtex197 prodtex198 prodtex199 prodtex200 prodtex201 prodtex202 prodtex203 prodtex204 prodtex205 prodtex206 prodtex207 prodtex208 prodtex209 prodtex210 prodtex211 prodtex212 prodtex213 prodtex214 prodtex215 prodtex216 prodtex217 prodtex218 prodtex219 prodtex220 prodtex221 prodtex222 prodtex223 prodtex224 prodtex225 prodtex226 prodtex227 prodtex228 prodtex229 prodtex230 prodtex231 prodtex232 prodtex233 prodtex234 prodtex235 prodtex236 prodtex237 prodtex238 prodtex239 prodtex240 prodtex241 prodtex242 prodtex243 prodtex244 prodtex245 prodtex246 prodtex247 prodtex248 prodtex249 prodtex250 prodtex251 prodtex252 prodtex253 prodtex254 prodtex255 prodtex256 prodtex257 prodtex258 prodtex259 prodtex260 prodtex261 prodtex262 prodtex263 prodtex264 prodtex265 prodtex266 prodtex267 prodtex268 prodtex269 prodtex270 prodtex271 prodtex272 prodtex273 prodtex274 prodtex275 prodtex276 prodtex277 prodtex278 prodtex279 prodtex280 prodtex281 prodtex282 prodtex283 prodtex284 prodtex285 prodtex286 prodtex287 prodtex288 prodtex289 prodtex290 prodtex291 prodtex292 prodtex293 prodtex294 prodtex295 prodtex296 prodtex297 prodtex298 prodtex299 prodtex300 prodtex301 prodtex302 prodtex303 prodtex304 prodtex305 prodtex306 prodtex307 prodtex308 prodtex309 prodtex310 prodtex311 prodtex312 prodtex313 prodtex314 prodtex315 prodtex316 prodtex317 prodtex318 prodtex319 prodtex320 prodtex321 prodtex322 prodtex323 prodtex324 prodtex325 prodtex326 prodtex327 prodtex328 prodtex329 prodtex330 prodtex331 prodtex332 prodtex333 prodtex334 prodtex335 prodtex336 prodtex337 prodtex338 prodtex339 prodtex340 prodtex341 prodtex342 prodtex343 prodtex344 prodtex345 prodtex346 prodtex347 prodtex348 prodtex349 prodtex350 prodtex351 prodtex352 prodtex353 prodtex354 prodtex355 prodtex356 prodtex357 prodtex358 prodtex359 prodtex360 prodtex361 prodtex362 prodtex363 prodtex364 prodtex365 prodtex366 prodtex367 prodtex368 prodtex369 prodtex370 prodtex371 prodtex372 prodtex373 prodtex374 prodtex375 prodtex376 prodtex377 prodtex378 prodtex379 prodtex380 prodtex381 prodtex382 prodtex383 prodtex384 prodtex385 prodtex386 prodtex387 prodtex388 prodtex389 prodtex390 prodtex391
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 14305 created, 100 current, 332 peak
    Total pixel data size of all images referenced : 49.0 GB
    File I/O time : 1m 41.7s (3.1s average per thread, for 33 threads)
    ImageInput mutex locking time : 1m 6.6s
  Tiles: 153282 created, 43633 current, 43818 peak
    micro-cache misses : 132721844 (97.1%)
    redundant reads: 50819 tiles, 1.2 GB
    Peak cache memory : 1024.0 MB
  Top files by redundant I/O:
wall=4.848 cpu=74.525 rc=0
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
    ImageInput mutex locking time : 0.0s
  Tiles: 9 created, 2 current, 9 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=1.440 cpu=35.899 rc=0
```
</details>

<details><summary>testtex-sweep-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --maketests 10 test{:04}.exr --maketest-res 1024 --threadtimes 7 --threads 32 --runstats
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 10 created, 10 current, 10 peak
    Total pixel data size of all images referenced : 106.7 MB
    File I/O time : 0.2s (0.0s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.1s
  Tiles: 816 created, 800 current, 801 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.213 cpu=58.997 rc=0
```
</details>

<details><summary>testtex-tileuse-prod-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --threadtimes 7 --threads 32 --runstats prodtex0 prodtex1 prodtex2 prodtex3 prodtex4 prodtex5 prodtex6 prodtex7 prodtex8 prodtex9 prodtex10 prodtex11 prodtex12 prodtex13 prodtex14 prodtex15 prodtex16 prodtex17 prodtex18 prodtex19 prodtex20 prodtex21 prodtex22 prodtex23 prodtex24 prodtex25 prodtex26 prodtex27 prodtex28 prodtex29 prodtex30 prodtex31 prodtex32 prodtex33 prodtex34 prodtex35 prodtex36 prodtex37 prodtex38 prodtex39 prodtex40 prodtex41 prodtex42 prodtex43 prodtex44 prodtex45 prodtex46 prodtex47 prodtex48 prodtex49 prodtex50 prodtex51 prodtex52 prodtex53 prodtex54 prodtex55 prodtex56 prodtex57 prodtex58 prodtex59 prodtex60 prodtex61 prodtex62 prodtex63 prodtex64 prodtex65 prodtex66 prodtex67 prodtex68 prodtex69 prodtex70 prodtex71 prodtex72 prodtex73 prodtex74 prodtex75 prodtex76 prodtex77 prodtex78 prodtex79 prodtex80 prodtex81 prodtex82 prodtex83 prodtex84 prodtex85 prodtex86 prodtex87 prodtex88 prodtex89 prodtex90 prodtex91 prodtex92 prodtex93 prodtex94 prodtex95 prodtex96 prodtex97 prodtex98 prodtex99 prodtex100 prodtex101 prodtex102 prodtex103 prodtex104 prodtex105 prodtex106 prodtex107 prodtex108 prodtex109 prodtex110 prodtex111 prodtex112 prodtex113 prodtex114 prodtex115 prodtex116 prodtex117 prodtex118 prodtex119 prodtex120 prodtex121 prodtex122 prodtex123 prodtex124 prodtex125 prodtex126 prodtex127 prodtex128 prodtex129 prodtex130 prodtex131 prodtex132 prodtex133 prodtex134 prodtex135 prodtex136 prodtex137 prodtex138 prodtex139 prodtex140 prodtex141 prodtex142 prodtex143 prodtex144 prodtex145 prodtex146 prodtex147 prodtex148 prodtex149 prodtex150 prodtex151 prodtex152 prodtex153 prodtex154 prodtex155 prodtex156 prodtex157 prodtex158 prodtex159 prodtex160 prodtex161 prodtex162 prodtex163 prodtex164 prodtex165 prodtex166 prodtex167 prodtex168 prodtex169 prodtex170 prodtex171 prodtex172 prodtex173 prodtex174 prodtex175 prodtex176 prodtex177 prodtex178 prodtex179 prodtex180 prodtex181 prodtex182 prodtex183 prodtex184 prodtex185 prodtex186 prodtex187 prodtex188 prodtex189 prodtex190 prodtex191 prodtex192 prodtex193 prodtex194 prodtex195 prodtex196 prodtex197 prodtex198 prodtex199 prodtex200 prodtex201 prodtex202 prodtex203 prodtex204 prodtex205 prodtex206 prodtex207 prodtex208 prodtex209 prodtex210 prodtex211 prodtex212 prodtex213 prodtex214 prodtex215 prodtex216 prodtex217 prodtex218 prodtex219 prodtex220 prodtex221 prodtex222 prodtex223 prodtex224 prodtex225 prodtex226 prodtex227 prodtex228 prodtex229 prodtex230 prodtex231 prodtex232 prodtex233 prodtex234 prodtex235 prodtex236 prodtex237 prodtex238 prodtex239 prodtex240 prodtex241 prodtex242 prodtex243 prodtex244 prodtex245 prodtex246 prodtex247 prodtex248 prodtex249 prodtex250 prodtex251 prodtex252 prodtex253 prodtex254 prodtex255 prodtex256 prodtex257 prodtex258 prodtex259 prodtex260 prodtex261 prodtex262 prodtex263 prodtex264 prodtex265 prodtex266 prodtex267 prodtex268 prodtex269 prodtex270 prodtex271 prodtex272 prodtex273 prodtex274 prodtex275 prodtex276 prodtex277 prodtex278 prodtex279 prodtex280 prodtex281 prodtex282 prodtex283 prodtex284 prodtex285 prodtex286 prodtex287 prodtex288 prodtex289 prodtex290 prodtex291 prodtex292 prodtex293 prodtex294 prodtex295 prodtex296 prodtex297 prodtex298 prodtex299 prodtex300 prodtex301 prodtex302 prodtex303 prodtex304 prodtex305 prodtex306 prodtex307 prodtex308 prodtex309 prodtex310 prodtex311 prodtex312 prodtex313 prodtex314 prodtex315 prodtex316 prodtex317 prodtex318 prodtex319 prodtex320 prodtex321 prodtex322 prodtex323 prodtex324 prodtex325 prodtex326 prodtex327 prodtex328 prodtex329 prodtex330 prodtex331 prodtex332 prodtex333 prodtex334 prodtex335 prodtex336 prodtex337 prodtex338 prodtex339 prodtex340 prodtex341 prodtex342 prodtex343 prodtex344 prodtex345 prodtex346 prodtex347 prodtex348 prodtex349 prodtex350 prodtex351 prodtex352 prodtex353 prodtex354 prodtex355 prodtex356 prodtex357 prodtex358 prodtex359 prodtex360 prodtex361 prodtex362 prodtex363 prodtex364 prodtex365 prodtex366 prodtex367 prodtex368 prodtex369 prodtex370 prodtex371 prodtex372 prodtex373 prodtex374 prodtex375 prodtex376 prodtex377 prodtex378 prodtex379 prodtex380 prodtex381 prodtex382 prodtex383 prodtex384 prodtex385 prodtex386 prodtex387 prodtex388 prodtex389 prodtex390 prodtex391
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 17534 created, 100 current, 341 peak
    Total pixel data size of all images referenced : 49.0 GB
    File I/O time : 1m 41.2s (3.1s average per thread, for 33 threads)
    ImageInput mutex locking time : 1m 1.6s
  Tiles: 153276 created, 43633 current, 43782 peak
    micro-cache misses : 132721844 (97.1%)
    redundant reads: 50813 tiles, 1.2 GB
    Peak cache memory : 1024.0 MB
  Top files by redundant I/O:
wall=4.966 cpu=82.333 rc=0
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
  Tiles: 21 created, 2 current, 21 peak
    micro-cache misses : 64 (0.0%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 64 KB
wall=1.722 cpu=44.255 rc=0
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
  Tiles: 814 created, 800 current, 800 peak
    micro-cache misses : 128972283 (94.9%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 25.0 MB
wall=2.339 cpu=63.934 rc=0
```
</details>

## spinlock_test

**program-reported time (s, best of trials)**

| variant | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| base | 0.5 | 0.9 | 1.7 | 3.6 | 6.3 | 10.8 | 9.9 | 9.3 |
| spin | 0.5 | 0.9 | 1.4 | 2.3 | 4.2 | 7.0 | 7.9 | 9.1 |

**whole process wall/cpu seconds (all trials)**

| variant | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| base | 1.927/1.920 | 3.186/5.527 | 5.970/20.986 | 11.499/83.952 | 19.488/294.935 | 33.059/969.169 | 31.421/931.344 | 28.896/868.890 |
| spin | 1.930/1.913 | 3.101/5.411 | 4.879/16.301 | 7.734/53.490 | 13.142/194.012 | 21.753/634.700 | 24.967/754.642 | 27.947/862.605 |

## parallel_test

**parallel_for task launches per second (higher is better)**

| variant | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| base | 3.08e+07 | 5.78e+07 | 1.02e+08 | 1.71e+08 | 2.51e+08 | 3.33e+08 | 3.79e+08 | 3.95e+08 |
| spin | 3.12e+07 | 5.59e+07 | 1.02e+08 | 1.79e+08 | 2.73e+08 | 3.64e+08 | 4.13e+08 | 4.44e+08 |

## spin_rw_test (strong scaling; seconds, best of 3)

**spin_rw_test --wedge**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 20 | 24 | 28 | 32 | 64 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| base | 0.2 | 0.4 | 1.3 | 1.6 | 1.6 | 1.9 | 2.2 | 2.4 | 2.7 | 3.0 | 2.6 |
| spin | 0.2 | 0.4 | 1.2 | 1.5 | 1.7 | 1.9 | 2.2 | 2.5 | 2.7 | 3.0 | 2.5 |

## spinbench (all nodes): wall/cpu ns per op

Uncontended ns: A_cur=3.10, A2_cur_nodclp=3.09, B_flag_isb_16y1=3.09, C_bool_isb_16y1=3.09, D_article=3.12, E_bool_isb_64y64=3.10, X_bool_isb_64y64=3.09, X_bool_isb_64y256=3.09, X_bool_isb_64y1k=3.09, X_bool_isb_128y16=3.11, X_bool_isb_128y64=3.10, X_bool_isb_128y256=3.10, X_bool_isb_32y256=3.09, H_stdmutex=5.97, O_oiio_spin_mutex=3.09

**spinbench17o3 tiny**

| lock | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| A_cur | 2.7/2.6 | 4.9/9.8 | 8.2/31.8 | 20.3/159.5 | 50.9/794.7 | 48.1/1433.9 | 38.1/1129.6 | 19.5/529.0 |
| A2_cur_nodclp | 2.7/2.6 | 4.8/9.6 | 12.0/47.2 | 27.4/214.6 | 56.2/878.8 | 93.6/2835.9 | 67.5/2016.1 | 44.5/1133.0 |
| B_flag_isb_16y1 | 2.7/2.7 | 7.5/14.6 | 14.9/57.6 | 30.9/235.4 | 63.5/974.6 | 71.0/2123.3 | 56.4/1575.0 | 35.4/975.6 |
| C_bool_isb_16y1 | 2.7/2.7 | 7.3/14.3 | 15.1/57.7 | 29.8/229.8 | 65.4/1007.8 | 72.2/2192.0 | 57.9/1686.2 | 34.9/953.0 |
| D_article | 2.7/2.6 | 6.7/13.2 | 11.4/45.2 | 23.4/184.4 | 49.5/774.4 | 76.2/2354.2 |  |  |
| E_bool_isb_64y64 | 2.7/2.7 | 5.5/10.8 | 13.2/51.8 | 23.6/185.5 | 51.7/811.4 | 87.8/2676.9 | 86.8/2612.3 | 92.0/2811.0 |
| X_bool_isb_64y64 | 2.7/2.6 | 6.5/12.8 | 12.6/49.6 | 24.3/190.5 | 50.1/787.4 | 83.1/2592.1 | 83.9/2536.7 | 88.3/2705.8 |
| X_bool_isb_64y256 | 2.7/2.6 | 7.0/14.0 | 13.3/52.3 | 24.1/188.9 | 49.5/776.7 | 80.2/2505.6 | 91.1/2750.0 | 112.7/3476.2 |
| X_bool_isb_64y1k | 2.7/2.6 | 7.1/13.9 | 11.6/44.8 | 25.7/202.3 | 51.3/800.5 | 82.0/2540.0 | 98.2/3029.3 | 119.5/3744.5 |
| X_bool_isb_128y16 | 2.7/2.7 | 5.7/11.4 | 10.3/39.9 | 15.5/119.9 | 35.6/561.3 | 75.1/2353.9 | 54.8/1654.0 | 44.0/1241.9 |
| X_bool_isb_128y64 | 2.7/2.7 | 5.9/11.7 | 9.1/35.9 | 15.7/122.8 | 28.8/448.2 | 46.7/1445.5 | 50.0/1525.1 | 52.6/1590.3 |
| X_bool_isb_128y256 | 2.7/2.6 | 5.8/11.4 | 8.9/34.8 | 15.5/121.3 | 27.9/435.7 | 45.8/1425.8 | 50.6/1543.5 | 58.3/1816.6 |
| X_bool_isb_32y256 | 2.7/2.6 | 8.5/16.9 | 19.9/78.4 | 50.5/396.5 | 114.5/1768.7 | 179.4/5556.1 | 198.1/6028.8 | 234.4/7366.5 |
| H_stdmutex | 5.6/5.5 | 19.5/38.7 | 33.8/120.4 | 31.4/228.6 | 33.0/485.6 | 45.5/1340.1 | 44.9/1356.4 | 43.6/1356.5 |
| O_oiio_spin_mutex | 2.7/2.6 | 5.6/10.9 | 8.9/35.0 | 16.5/129.2 | 28.8/450.1 | 46.6/1438.0 | 50.7/1540.5 | 53.6/1635.0 |

**spinbench17o3 medium**

| lock | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| A_cur | 16.8/16.7 | 24.8/48.8 | 32.5/127.3 | 46.8/364.4 | 74.9/1134.1 | 88.2/2576.1 | 76.4/2087.4 | 50.8/1314.3 |
| A2_cur_nodclp | 16.7/16.7 | 24.1/47.3 | 35.4/140.2 | 50.7/391.4 | 75.0/1150.7 | 118.4/3577.2 | 89.1/2440.2 | 59.3/1471.5 |
| B_flag_isb_16y1 | 16.9/16.8 | 31.7/61.8 | 43.8/169.9 | 62.2/478.0 | 86.0/1289.1 | 105.8/3089.9 | 91.7/2467.5 | 60.2/1540.9 |
| C_bool_isb_16y1 | 16.8/16.8 | 33.3/65.6 | 43.1/166.0 | 57.6/434.5 | 86.4/1306.8 | 104.5/2954.6 | 91.0/2498.3 | 59.9/1520.9 |
| D_article | 16.8/16.7 | 31.0/61.2 | 40.2/151.5 | 56.6/434.2 | 82.9/1265.9 | 129.8/4032.4 |  |  |
| E_bool_isb_64y64 | 16.8/16.8 | 30.4/60.2 | 39.7/150.4 | 53.2/409.8 | 80.0/1210.7 | 132.9/4148.4 | 132.8/4033.0 | 137.3/4218.0 |
| X_bool_isb_64y64 | 15.8/15.8 | 31.5/61.3 | 39.7/155.5 | 55.4/426.5 | 79.1/1199.0 | 127.4/3940.4 | 125.9/3803.3 | 130.9/4049.5 |
| X_bool_isb_64y256 | 15.8/15.8 | 32.1/63.6 | 39.4/155.7 | 54.6/429.6 | 78.9/1201.2 | 123.5/3781.7 | 134.5/4086.2 | 151.0/4684.5 |
| X_bool_isb_64y1k | 15.8/15.8 | 32.1/63.3 | 41.5/163.4 | 56.7/438.9 | 78.7/1188.9 | 123.2/3717.3 | 138.3/4216.5 | 156.2/4864.9 |
| X_bool_isb_128y16 | 15.8/15.7 | 28.5/55.7 | 36.0/142.2 | 45.3/355.4 | 66.6/1029.8 | 116.6/3637.5 | 91.5/2705.9 | 79.7/2171.3 |
| X_bool_isb_128y64 | 15.8/15.7 | 28.3/56.4 | 35.8/140.0 | 44.8/346.2 | 58.1/886.0 | 93.3/2862.6 | 91.7/2746.8 | 99.6/3041.6 |
| X_bool_isb_128y256 | 15.8/15.8 | 28.2/56.2 | 35.5/138.9 | 45.0/347.1 | 59.3/901.7 | 92.0/2780.4 | 100.2/3000.7 | 112.7/3501.0 |
| X_bool_isb_32y256 | 15.8/15.8 | 38.8/76.7 | 51.5/202.6 | 72.3/560.2 | 131.9/2009.0 | 203.4/6143.2 | 223.4/6926.6 | 262.9/8179.6 |
| H_stdmutex | 18.1/18.1 | 58.9/110.5 | 106.8/377.9 | 104.2/751.0 | 108.0/1608.6 | 135.0/4053.7 | 130.8/3913.0 | 130.3/4068.7 |
| O_oiio_spin_mutex | 15.8/15.8 | 28.0/55.4 | 35.5/141.3 | 45.6/352.2 | 57.7/878.8 | 90.9/2830.6 | 94.1/2812.3 | 98.5/3058.7 |

**spinbench17o3 coloc**

| lock | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| A_cur | 3.1/3.1 | 4.5/8.8 | 6.2/23.7 | 11.1/84.9 | 25.6/395.1 | 42.8/1322.6 | 33.8/1008.2 | 19.9/521.6 |
| A2_cur_nodclp | 3.1/3.1 | 4.5/8.4 | 18.7/73.2 | 27.3/213.7 | 41.8/654.9 | 79.0/2392.8 | 60.5/1687.3 | 44.8/1076.9 |
| B_flag_isb_16y1 | 3.1/3.1 | 8.1/15.5 | 16.9/65.2 | 24.8/192.4 | 41.0/613.2 | 62.1/1788.3 | 50.5/1407.6 | 34.0/889.2 |
| C_bool_isb_16y1 | 3.1/3.1 | 8.2/15.7 | 16.3/62.6 | 27.0/199.5 | 43.1/648.3 | 64.1/1970.9 | 52.5/1461.2 | 36.2/965.3 |
| D_article | 3.1/3.1 | 7.8/14.9 | 14.4/55.3 | 24.0/183.0 | 45.7/696.3 | 73.5/2285.9 |  |  |
| E_bool_isb_64y64 | 3.1/3.1 | 7.8/15.0 | 14.7/56.1 | 24.1/186.4 | 45.0/683.3 | 72.8/2209.6 | 76.4/2334.8 | 79.9/2474.1 |
| X_bool_isb_64y64 | 3.1/3.1 | 5.2/9.3 | 11.4/43.6 | 22.4/172.0 | 43.3/658.4 | 73.4/2230.2 | 73.5/2183.2 | 80.1/2446.8 |
| X_bool_isb_64y256 | 3.1/3.1 | 5.1/9.3 | 10.3/37.0 | 21.0/154.4 | 45.1/691.1 | 70.8/2177.3 | 83.5/2559.4 | 101.6/3155.8 |
| X_bool_isb_64y1k | 3.1/3.1 | 5.5/10.2 | 11.4/43.1 | 22.2/162.1 | 44.7/685.0 | 70.0/2152.4 | 83.5/2572.2 | 91.9/2815.0 |
| X_bool_isb_128y16 | 3.1/3.1 | 4.4/7.5 | 9.4/31.8 | 15.7/118.3 | 29.5/450.4 | 56.8/1748.1 | 44.3/1242.1 | 42.1/1124.7 |
| X_bool_isb_128y64 | 3.1/3.1 | 4.6/7.9 | 9.0/29.9 | 15.1/105.0 | 26.5/394.8 | 45.3/1357.1 | 51.5/1591.4 | 54.0/1676.8 |
| X_bool_isb_128y256 | 3.1/3.1 | 6.0/11.7 | 8.3/28.0 | 15.1/108.6 | 27.4/411.7 | 44.1/1322.6 | 52.5/1598.7 | 66.3/2065.5 |
| X_bool_isb_32y256 | 3.1/3.1 | 6.0/11.2 | 14.3/52.7 | 35.9/271.8 | 83.1/1262.4 | 166.5/5135.4 | 183.5/5682.8 | 206.6/6470.1 |
| H_stdmutex | 5.7/5.7 | 17.5/34.5 | 21.8/81.9 | 21.5/163.1 | 20.7/313.8 | 29.6/908.6 | 29.0/865.3 | 29.4/914.5 |
| O_oiio_spin_mutex | 3.1/3.1 | 5.0/9.0 | 9.9/37.0 | 16.5/124.7 | 26.5/393.8 | 47.3/1449.6 | 51.8/1601.7 | 55.4/1722.5 |

**spinbench20o3 tiny**

| lock | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| A_cur | 2.7/2.6 | 5.0/9.9 | 9.3/36.0 | 19.6/150.2 | 50.7/792.2 | 47.5/1429.1 | 37.3/1087.3 | 19.2/523.5 |
| A2_cur_nodclp | 2.7/2.6 | 4.7/9.4 | 11.5/45.1 | 28.2/222.2 | 56.5/885.7 | 95.2/2892.5 | 75.4/2211.5 | 44.0/1173.7 |
| B_flag_isb_16y1 | 2.7/2.6 | 7.3/14.3 | 14.8/57.6 | 30.1/232.9 | 62.9/956.4 | 71.0/2127.2 | 56.0/1606.0 | 34.9/938.9 |
| C_bool_isb_16y1 | 2.7/2.6 | 7.2/14.1 | 15.6/61.6 | 29.5/230.6 | 64.7/1003.3 | 72.5/2182.0 | 55.0/1561.9 | 34.8/958.8 |
| D_article | 2.7/2.6 | 6.8/13.5 | 13.5/53.1 | 23.4/183.9 | 49.3/764.8 | 77.8/2431.6 |  |  |
| E_bool_isb_64y64 | 2.7/2.6 | 6.8/13.4 | 13.0/51.1 | 24.0/188.3 | 51.4/799.4 | 84.3/2647.9 | 83.2/2508.3 | 85.6/2608.1 |
| X_bool_isb_64y64 | 2.7/2.6 | 6.5/12.6 | 12.7/48.9 | 22.5/176.8 | 48.9/769.0 | 84.1/2601.4 | 85.0/2527.1 | 87.6/2678.3 |
| X_bool_isb_64y256 | 2.7/2.6 | 7.1/13.9 | 11.7/45.9 | 25.7/202.1 | 50.3/784.5 | 83.9/2551.1 | 96.0/2944.3 | 107.3/3341.5 |
| X_bool_isb_64y1k | 2.7/2.6 | 6.7/13.4 | 12.6/49.7 | 25.1/195.7 | 52.4/821.7 | 76.0/2356.0 | 88.2/2676.3 | 112.3/3516.0 |
| X_bool_isb_128y16 | 2.7/2.6 | 5.8/11.5 | 9.0/34.9 | 16.3/127.2 | 34.0/533.5 | 77.1/2427.8 | 59.2/1770.9 | 48.9/1359.2 |
| X_bool_isb_128y64 | 2.7/2.6 | 6.1/12.2 | 9.6/37.8 | 16.8/131.1 | 27.2/428.2 | 46.6/1453.6 | 46.0/1379.4 | 49.0/1520.5 |
| X_bool_isb_128y256 | 2.7/2.6 | 6.0/11.9 | 10.3/40.6 | 17.2/134.5 | 28.2/443.1 | 45.5/1412.4 | 51.6/1536.9 | 58.4/1807.8 |
| X_bool_isb_32y256 | 2.7/2.6 | 9.3/18.3 | 21.5/85.1 | 49.1/388.7 | 111.3/1722.6 | 182.5/5674.7 | 212.5/6574.6 | 252.7/7940.7 |
| H_stdmutex | 5.4/5.3 | 19.8/38.3 | 33.7/120.0 | 31.1/225.1 | 32.6/482.3 | 45.4/1389.2 | 44.1/1306.4 | 44.3/1370.8 |
| F_flagtest_isb_64y16 | 2.7/2.6 | 7.2/14.2 | 12.9/51.0 | 27.6/217.6 | 68.7/1077.3 | 115.1/3569.6 | 84.5/2529.3 | 71.8/2027.8 |
| O_oiio_spin_mutex | 2.7/2.6 | 5.5/10.9 | 8.5/32.7 | 16.8/132.0 | 29.1/454.0 | 47.7/1495.4 | 48.1/1459.8 | 52.4/1621.7 |

**spinbench20o3 medium**

| lock | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| A_cur | 16.8/16.7 | 24.6/49.0 | 32.5/127.2 | 45.4/343.0 | 72.6/1105.5 | 87.7/2542.2 | 79.8/2197.4 | 51.1/1271.2 |
| A2_cur_nodclp | 16.7/16.7 | 24.3/47.8 | 34.8/134.2 | 52.2/405.3 | 73.7/1108.5 | 117.6/3549.3 | 84.8/2371.8 | 60.0/1503.3 |
| B_flag_isb_16y1 | 16.9/16.8 | 31.7/61.4 | 44.6/174.0 | 60.2/455.4 | 84.7/1256.4 | 106.1/3112.1 | 91.3/2541.8 | 60.0/1483.0 |
| C_bool_isb_16y1 | 16.7/16.7 | 32.5/63.2 | 45.1/176.6 | 61.1/463.6 | 84.3/1237.9 | 105.5/3009.0 | 90.5/2496.4 | 60.1/1544.0 |
| D_article | 16.8/16.7 | 31.0/60.8 | 39.1/152.8 | 56.4/436.3 | 82.2/1245.4 | 128.6/3926.3 |  |  |
| E_bool_isb_64y64 | 16.9/16.8 | 30.6/60.1 | 39.6/156.0 | 54.8/429.9 | 80.4/1237.3 | 134.4/4205.3 | 131.1/3863.1 | 136.0/4191.9 |
| X_bool_isb_64y64 | 15.8/15.8 | 31.8/62.4 | 40.7/158.7 | 55.0/420.5 | 80.6/1248.9 | 128.3/3956.2 | 126.1/3813.4 | 132.2/4040.1 |
| X_bool_isb_64y256 | 15.9/15.8 | 32.0/64.0 | 38.4/150.8 | 54.5/421.2 | 77.6/1179.7 | 123.5/3832.6 | 136.2/4054.3 | 151.7/4712.4 |
| X_bool_isb_64y1k | 15.9/15.8 | 31.8/61.8 | 40.5/160.8 | 56.0/430.7 | 78.9/1193.0 | 124.7/3869.5 | 142.2/4362.4 | 153.6/4736.7 |
| X_bool_isb_128y16 | 15.7/15.7 | 28.3/56.0 | 35.7/141.5 | 45.7/357.1 | 66.1/1037.6 | 118.4/3662.9 | 91.2/2671.1 | 78.4/2161.4 |
| X_bool_isb_128y64 | 15.8/15.7 | 28.2/55.3 | 34.9/136.8 | 43.4/333.7 | 57.1/865.1 | 91.8/2851.4 | 93.8/2787.8 | 99.0/3042.0 |
| X_bool_isb_128y256 | 15.8/15.8 | 27.7/53.8 | 35.2/136.9 | 44.0/335.1 | 57.3/888.5 | 90.2/2727.2 | 96.1/2979.2 | 118.8/3686.4 |
| X_bool_isb_32y256 | 15.7/15.7 | 37.4/73.5 | 51.5/201.9 | 75.6/578.0 | 134.0/2041.9 | 206.5/6319.1 | 225.6/6937.9 | 267.1/8353.4 |
| H_stdmutex | 18.1/18.0 | 58.1/109.2 | 106.2/372.8 | 105.8/763.9 | 107.6/1612.7 | 134.2/4007.6 | 130.0/3919.9 | 129.2/4038.2 |
| F_flagtest_isb_64y16 | 16.8/16.8 | 31.2/62.2 | 41.4/161.5 | 62.6/485.6 | 99.4/1521.1 | 151.7/4703.8 | 114.2/3369.1 | 98.8/2675.4 |
| O_oiio_spin_mutex | 15.7/15.7 | 28.1/54.7 | 35.0/135.7 | 45.4/349.5 | 60.7/923.6 | 92.7/2817.2 | 93.1/2771.6 | 98.4/3032.6 |

**spinbench20o3 coloc**

| lock | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 |
|---|---|---|---|---|---|---|---|---|
| A_cur | 3.1/3.1 | 4.6/9.2 | 6.3/23.4 | 10.8/82.6 | 25.8/398.1 | 43.3/1346.2 | 34.6/1013.8 | 20.8/548.6 |
| A2_cur_nodclp | 3.1/3.1 | 4.7/9.0 | 18.7/71.4 | 27.9/217.5 | 42.5/668.2 | 80.3/2418.4 | 58.0/1595.3 | 42.8/1049.2 |
| B_flag_isb_16y1 | 3.1/3.1 | 8.6/16.7 | 17.4/68.6 | 26.0/197.7 | 40.7/612.7 | 63.1/1932.7 | 47.7/1346.1 | 37.2/980.3 |
| C_bool_isb_16y1 | 3.1/3.1 | 7.5/14.3 | 15.4/60.2 | 24.9/186.7 | 43.0/659.0 | 64.4/1988.5 | 51.3/1447.0 | 36.9/987.6 |
| D_article | 3.1/3.1 | 8.1/15.6 | 13.8/50.8 | 24.5/188.9 | 45.3/694.1 | 72.8/2232.9 |  |  |
| E_bool_isb_64y64 | 3.1/3.1 | 7.9/15.2 | 14.2/54.8 | 24.8/190.7 | 45.0/685.1 | 74.5/2298.7 | 76.9/2304.0 | 84.0/2602.5 |
| X_bool_isb_64y64 | 3.1/3.1 | 5.2/9.6 | 10.6/39.6 | 22.5/166.7 | 42.6/644.5 | 72.6/2218.4 | 75.9/2315.8 | 77.8/2391.7 |
| X_bool_isb_64y256 | 3.1/3.1 | 5.2/9.5 | 10.7/39.7 | 22.0/167.7 | 44.0/674.0 | 70.9/2122.1 | 79.0/2406.1 | 98.7/3050.6 |
| X_bool_isb_64y1k | 3.1/3.1 | 5.2/9.5 | 11.9/46.0 | 22.2/169.4 | 43.9/669.3 | 69.9/2137.3 | 84.4/2520.5 | 92.3/2860.1 |
| X_bool_isb_128y16 | 3.1/3.1 | 4.7/8.3 | 9.8/36.7 | 16.0/125.1 | 28.5/435.8 | 56.7/1734.5 | 44.2/1255.0 | 40.6/1106.0 |
| X_bool_isb_128y64 | 3.1/3.1 | 4.7/8.4 | 10.0/38.7 | 16.1/117.3 | 27.5/420.5 | 43.7/1297.9 | 48.0/1458.1 | 60.3/1850.0 |
| X_bool_isb_128y256 | 3.1/3.1 | 4.6/8.1 | 8.6/27.0 | 15.4/111.3 | 26.5/396.3 | 44.0/1316.1 | 53.2/1618.0 | 58.6/1809.3 |
| X_bool_isb_32y256 | 3.1/3.1 | 6.0/11.2 | 14.7/56.7 | 38.2/295.8 | 86.9/1342.7 | 168.2/5159.5 | 186.2/5722.6 | 219.5/6829.7 |
| H_stdmutex | 5.8/5.7 | 18.3/36.4 | 21.8/82.0 | 20.8/156.1 | 21.3/316.2 | 29.6/911.6 | 29.1/839.4 | 29.2/902.8 |
| F_flagtest_isb_64y16 | 3.1/3.1 | 8.0/15.5 | 13.7/52.4 | 24.2/179.2 | 47.9/727.3 | 92.4/2833.4 | 73.0/2145.5 | 60.2/1648.8 |
| O_oiio_spin_mutex | 3.1/3.1 | 4.8/8.4 | 9.8/37.7 | 15.8/117.7 | 26.5/396.7 | 44.3/1345.8 | 47.5/1454.6 | 54.1/1681.7 |

## Progress log tail
```
2026-10-12 18:30:50 START testtex sweep stats-w1
2026-10-12 18:30:52 END testtex sweep stats-w1 wall=1.440 cpu=35.899 rc=0
2026-10-12 18:30:52 START testtex sweep stats-w7
2026-10-12 18:30:54 END testtex sweep stats-w7 wall=2.213 cpu=58.997 rc=0
2026-10-12 18:30:54 START testtex sweep prod-w1
2026-10-12 18:31:14 END testtex sweep prod-w1 wall=19.736 cpu=263.181 rc=0
2026-10-12 18:31:14 START testtex sweep prod-w4
2026-10-12 18:31:35 END testtex sweep prod-w4 wall=21.282 cpu=275.871 rc=0
2026-10-12 18:31:35 START testtex sweep prod-w7
2026-10-12 18:32:59 END testtex sweep prod-w7 wall=84.204 cpu=608.337 rc=0
2026-10-12 18:32:59 START testtex sweep prod-w8
2026-10-12 18:34:11 END testtex sweep prod-w8 wall=71.981 cpu=434.546 rc=0
2026-10-12 18:34:11 START testtex sweep prod-stats-w7
2026-10-12 18:34:16 END testtex sweep prod-stats-w7 wall=4.848 cpu=74.525 rc=0
2026-10-12 18:34:17 === variant sweep complete
```
