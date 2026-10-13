# Results: bear0056-prod-c8192

## Machine
<details><summary>machine.txt</summary>

```
## date
Mon Oct 12 09:28:28 PM PDT 2026
## git
1d226e2a4330560a81bf8791a2cfc42320106958 spinlock-work: next_run.sh pushes results when done
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
Mem:             125          14          51           0          60         110
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

## testtex --threadtimes (seconds)
Weak scaling (fixed iters/thread): flat = perfect. Strong scaling (quick runs with --iters): halving = perfect.

**prod-c8192-stats-w7 (weak scaling)**

| variant | 32 |
|---|---|
| base | 2.77 |
| sweep | 2.56 |

Speedup vs base at 32 threads: sweep 1.08x

**prod-c8192-w1 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.67 | 1.45 | 1.47 | 1.47 | 1.47 | 1.43 | 1.62 | 1.90 |
| sweep | 0.67 | 0.67 | 0.67 | 0.70 | 0.70 | 0.72 | 0.94 | 1.02 |

Speedup vs base at 32 threads: sweep 1.86x

**prod-c8192-w4 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 0.78 | 1.06 | 1.06 | 1.02 | 1.03 | 1.13 | 1.33 | 1.45 |
| sweep | 0.78 | 0.77 | 0.77 | 0.77 | 0.80 | 0.84 | 1.05 | 1.15 |

Speedup vs base at 32 threads: sweep 1.26x

**prod-c8192-w7 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 1.66 | 1.74 | 1.80 | 1.78 | 1.75 | 1.78 | 2.00 | 2.45 |
| sweep | 1.68 | 1.63 | 1.69 | 1.64 | 1.66 | 1.66 | 1.89 | 2.35 |

Speedup vs base at 32 threads: sweep 1.04x

**prod-c8192-w8 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 |
|---|---|---|---|---|---|---|---|---|
| base | 1.22 | 1.25 | 1.26 | 1.29 | 1.29 | 1.34 | 1.39 | 1.79 |
| sweep | 1.23 | 1.20 | 1.24 | 1.22 | 1.24 | 1.23 | 1.27 | 1.68 |

Speedup vs base at 32 threads: sweep 1.07x

<details><summary>testtex-base-prod-c8192-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --threadtimes 7 --threads 32 --runstats --cachesize 8192 --maxfiles 1000 prodtex0 prodtex1 prodtex2 prodtex3 prodtex4 prodtex5 prodtex6 prodtex7 prodtex8 prodtex9 prodtex10 prodtex11 prodtex12 prodtex13 prodtex14 prodtex15 prodtex16 prodtex17 prodtex18 prodtex19 prodtex20 prodtex21 prodtex22 prodtex23 prodtex24 prodtex25 prodtex26 prodtex27 prodtex28 prodtex29 prodtex30 prodtex31 prodtex32 prodtex33 prodtex34 prodtex35 prodtex36 prodtex37 prodtex38 prodtex39 prodtex40 prodtex41 prodtex42 prodtex43 prodtex44 prodtex45 prodtex46 prodtex47 prodtex48 prodtex49 prodtex50 prodtex51 prodtex52 prodtex53 prodtex54 prodtex55 prodtex56 prodtex57 prodtex58 prodtex59 prodtex60 prodtex61 prodtex62 prodtex63 prodtex64 prodtex65 prodtex66 prodtex67 prodtex68 prodtex69 prodtex70 prodtex71 prodtex72 prodtex73 prodtex74 prodtex75 prodtex76 prodtex77 prodtex78 prodtex79 prodtex80 prodtex81 prodtex82 prodtex83 prodtex84 prodtex85 prodtex86 prodtex87 prodtex88 prodtex89 prodtex90 prodtex91 prodtex92 prodtex93 prodtex94 prodtex95 prodtex96 prodtex97 prodtex98 prodtex99 prodtex100 prodtex101 prodtex102 prodtex103 prodtex104 prodtex105 prodtex106 prodtex107 prodtex108 prodtex109 prodtex110 prodtex111 prodtex112 prodtex113 prodtex114 prodtex115 prodtex116 prodtex117 prodtex118 prodtex119 prodtex120 prodtex121 prodtex122 prodtex123 prodtex124 prodtex125 prodtex126 prodtex127 prodtex128 prodtex129 prodtex130 prodtex131 prodtex132 prodtex133 prodtex134 prodtex135 prodtex136 prodtex137 prodtex138 prodtex139 prodtex140 prodtex141 prodtex142 prodtex143 prodtex144 prodtex145 prodtex146 prodtex147 prodtex148 prodtex149 prodtex150 prodtex151 prodtex152 prodtex153 prodtex154 prodtex155 prodtex156 prodtex157 prodtex158 prodtex159 prodtex160 prodtex161 prodtex162 prodtex163 prodtex164 prodtex165 prodtex166 prodtex167 prodtex168 prodtex169 prodtex170 prodtex171 prodtex172 prodtex173 prodtex174 prodtex175 prodtex176 prodtex177 prodtex178 prodtex179 prodtex180 prodtex181 prodtex182 prodtex183 prodtex184 prodtex185 prodtex186 prodtex187 prodtex188 prodtex189 prodtex190 prodtex191 prodtex192 prodtex193 prodtex194 prodtex195 prodtex196 prodtex197 prodtex198 prodtex199 prodtex200 prodtex201 prodtex202 prodtex203 prodtex204 prodtex205 prodtex206 prodtex207 prodtex208 prodtex209 prodtex210 prodtex211 prodtex212 prodtex213 prodtex214 prodtex215 prodtex216 prodtex217 prodtex218 prodtex219 prodtex220 prodtex221 prodtex222 prodtex223 prodtex224 prodtex225 prodtex226 prodtex227 prodtex228 prodtex229 prodtex230 prodtex231 prodtex232 prodtex233 prodtex234 prodtex235 prodtex236 prodtex237 prodtex238 prodtex239 prodtex240 prodtex241 prodtex242 prodtex243 prodtex244 prodtex245 prodtex246 prodtex247 prodtex248 prodtex249 prodtex250 prodtex251 prodtex252 prodtex253 prodtex254 prodtex255 prodtex256 prodtex257 prodtex258 prodtex259 prodtex260 prodtex261 prodtex262 prodtex263 prodtex264 prodtex265 prodtex266 prodtex267 prodtex268 prodtex269 prodtex270 prodtex271 prodtex272 prodtex273 prodtex274 prodtex275 prodtex276 prodtex277 prodtex278 prodtex279 prodtex280 prodtex281 prodtex282 prodtex283 prodtex284 prodtex285 prodtex286 prodtex287 prodtex288 prodtex289 prodtex290 prodtex291 prodtex292 prodtex293 prodtex294 prodtex295 prodtex296 prodtex297 prodtex298 prodtex299 prodtex300 prodtex301 prodtex302 prodtex303 prodtex304 prodtex305 prodtex306 prodtex307 prodtex308 prodtex309 prodtex310 prodtex311 prodtex312 prodtex313 prodtex314 prodtex315 prodtex316 prodtex317 prodtex318 prodtex319 prodtex320 prodtex321 prodtex322 prodtex323 prodtex324 prodtex325 prodtex326 prodtex327 prodtex328 prodtex329 prodtex330 prodtex331 prodtex332 prodtex333 prodtex334 prodtex335 prodtex336 prodtex337 prodtex338 prodtex339 prodtex340 prodtex341 prodtex342 prodtex343 prodtex344 prodtex345 prodtex346 prodtex347 prodtex348 prodtex349 prodtex350 prodtex351 prodtex352 prodtex353 prodtex354 prodtex355 prodtex356 prodtex357 prodtex358 prodtex359 prodtex360 prodtex361 prodtex362 prodtex363 prodtex364 prodtex365 prodtex366 prodtex367 prodtex368 prodtex369 prodtex370 prodtex371 prodtex372 prodtex373 prodtex374 prodtex375 prodtex376 prodtex377 prodtex378 prodtex379 prodtex380 prodtex381 prodtex382 prodtex383 prodtex384 prodtex385 prodtex386 prodtex387 prodtex388 prodtex389 prodtex390 prodtex391
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 392 created, 392 current, 392 peak
    Total pixel data size of all images referenced : 49.0 GB
    File I/O time : 28.0s (0.8s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.2s
  Tiles: 102463 created, 102463 current, 102463 peak
    micro-cache misses : 132721844 (97.1%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 2.3 GB
wall=2.995 cpu=76.678 rc=0
```
</details>

<details><summary>testtex-sweep-prod-c8192-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --threadtimes 7 --threads 32 --runstats --cachesize 8192 --maxfiles 1000 prodtex0 prodtex1 prodtex2 prodtex3 prodtex4 prodtex5 prodtex6 prodtex7 prodtex8 prodtex9 prodtex10 prodtex11 prodtex12 prodtex13 prodtex14 prodtex15 prodtex16 prodtex17 prodtex18 prodtex19 prodtex20 prodtex21 prodtex22 prodtex23 prodtex24 prodtex25 prodtex26 prodtex27 prodtex28 prodtex29 prodtex30 prodtex31 prodtex32 prodtex33 prodtex34 prodtex35 prodtex36 prodtex37 prodtex38 prodtex39 prodtex40 prodtex41 prodtex42 prodtex43 prodtex44 prodtex45 prodtex46 prodtex47 prodtex48 prodtex49 prodtex50 prodtex51 prodtex52 prodtex53 prodtex54 prodtex55 prodtex56 prodtex57 prodtex58 prodtex59 prodtex60 prodtex61 prodtex62 prodtex63 prodtex64 prodtex65 prodtex66 prodtex67 prodtex68 prodtex69 prodtex70 prodtex71 prodtex72 prodtex73 prodtex74 prodtex75 prodtex76 prodtex77 prodtex78 prodtex79 prodtex80 prodtex81 prodtex82 prodtex83 prodtex84 prodtex85 prodtex86 prodtex87 prodtex88 prodtex89 prodtex90 prodtex91 prodtex92 prodtex93 prodtex94 prodtex95 prodtex96 prodtex97 prodtex98 prodtex99 prodtex100 prodtex101 prodtex102 prodtex103 prodtex104 prodtex105 prodtex106 prodtex107 prodtex108 prodtex109 prodtex110 prodtex111 prodtex112 prodtex113 prodtex114 prodtex115 prodtex116 prodtex117 prodtex118 prodtex119 prodtex120 prodtex121 prodtex122 prodtex123 prodtex124 prodtex125 prodtex126 prodtex127 prodtex128 prodtex129 prodtex130 prodtex131 prodtex132 prodtex133 prodtex134 prodtex135 prodtex136 prodtex137 prodtex138 prodtex139 prodtex140 prodtex141 prodtex142 prodtex143 prodtex144 prodtex145 prodtex146 prodtex147 prodtex148 prodtex149 prodtex150 prodtex151 prodtex152 prodtex153 prodtex154 prodtex155 prodtex156 prodtex157 prodtex158 prodtex159 prodtex160 prodtex161 prodtex162 prodtex163 prodtex164 prodtex165 prodtex166 prodtex167 prodtex168 prodtex169 prodtex170 prodtex171 prodtex172 prodtex173 prodtex174 prodtex175 prodtex176 prodtex177 prodtex178 prodtex179 prodtex180 prodtex181 prodtex182 prodtex183 prodtex184 prodtex185 prodtex186 prodtex187 prodtex188 prodtex189 prodtex190 prodtex191 prodtex192 prodtex193 prodtex194 prodtex195 prodtex196 prodtex197 prodtex198 prodtex199 prodtex200 prodtex201 prodtex202 prodtex203 prodtex204 prodtex205 prodtex206 prodtex207 prodtex208 prodtex209 prodtex210 prodtex211 prodtex212 prodtex213 prodtex214 prodtex215 prodtex216 prodtex217 prodtex218 prodtex219 prodtex220 prodtex221 prodtex222 prodtex223 prodtex224 prodtex225 prodtex226 prodtex227 prodtex228 prodtex229 prodtex230 prodtex231 prodtex232 prodtex233 prodtex234 prodtex235 prodtex236 prodtex237 prodtex238 prodtex239 prodtex240 prodtex241 prodtex242 prodtex243 prodtex244 prodtex245 prodtex246 prodtex247 prodtex248 prodtex249 prodtex250 prodtex251 prodtex252 prodtex253 prodtex254 prodtex255 prodtex256 prodtex257 prodtex258 prodtex259 prodtex260 prodtex261 prodtex262 prodtex263 prodtex264 prodtex265 prodtex266 prodtex267 prodtex268 prodtex269 prodtex270 prodtex271 prodtex272 prodtex273 prodtex274 prodtex275 prodtex276 prodtex277 prodtex278 prodtex279 prodtex280 prodtex281 prodtex282 prodtex283 prodtex284 prodtex285 prodtex286 prodtex287 prodtex288 prodtex289 prodtex290 prodtex291 prodtex292 prodtex293 prodtex294 prodtex295 prodtex296 prodtex297 prodtex298 prodtex299 prodtex300 prodtex301 prodtex302 prodtex303 prodtex304 prodtex305 prodtex306 prodtex307 prodtex308 prodtex309 prodtex310 prodtex311 prodtex312 prodtex313 prodtex314 prodtex315 prodtex316 prodtex317 prodtex318 prodtex319 prodtex320 prodtex321 prodtex322 prodtex323 prodtex324 prodtex325 prodtex326 prodtex327 prodtex328 prodtex329 prodtex330 prodtex331 prodtex332 prodtex333 prodtex334 prodtex335 prodtex336 prodtex337 prodtex338 prodtex339 prodtex340 prodtex341 prodtex342 prodtex343 prodtex344 prodtex345 prodtex346 prodtex347 prodtex348 prodtex349 prodtex350 prodtex351 prodtex352 prodtex353 prodtex354 prodtex355 prodtex356 prodtex357 prodtex358 prodtex359 prodtex360 prodtex361 prodtex362 prodtex363 prodtex364 prodtex365 prodtex366 prodtex367 prodtex368 prodtex369 prodtex370 prodtex371 prodtex372 prodtex373 prodtex374 prodtex375 prodtex376 prodtex377 prodtex378 prodtex379 prodtex380 prodtex381 prodtex382 prodtex383 prodtex384 prodtex385 prodtex386 prodtex387 prodtex388 prodtex389 prodtex390 prodtex391
hw threads = 32
threads  time (s)   speedup efficiency
    ImageInputs : 392 created, 392 current, 392 peak
    Total pixel data size of all images referenced : 49.0 GB
    File I/O time : 24.2s (0.7s average per thread, for 33 threads)
    ImageInput mutex locking time : 0.2s
  Tiles: 102463 created, 102463 current, 102463 peak
    micro-cache misses : 132721844 (97.1%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 2.3 GB
wall=2.782 cpu=68.982 rc=0
```
</details>

## Progress log tail
```
2026-10-12 21:31:01 === variant base complete
2026-10-12 21:31:01 START checkout sweep (1a9ee4ea5)
2026-10-12 21:31:01 START build.sh sweep
2026-10-12 21:31:06 END build.sh sweep OK
2026-10-12 21:31:06 START testtex sweep prod-c8192-w1
2026-10-12 21:31:25 END testtex sweep prod-c8192-w1 wall=18.744 cpu=245.995 rc=0
2026-10-12 21:31:25 START testtex sweep prod-c8192-w4
2026-10-12 21:31:46 END testtex sweep prod-c8192-w4 wall=21.504 cpu=276.033 rc=0
2026-10-12 21:31:46 START testtex sweep prod-c8192-w7
2026-10-12 21:32:30 END testtex sweep prod-c8192-w7 wall=43.453 cpu=490.323 rc=0
2026-10-12 21:32:30 START testtex sweep prod-c8192-w8
2026-10-12 21:33:02 END testtex sweep prod-c8192-w8 wall=31.777 cpu=325.322 rc=0
2026-10-12 21:33:02 START testtex sweep prod-c8192-stats-w7
2026-10-12 21:33:05 END testtex sweep prod-c8192-stats-w7 wall=2.782 cpu=68.982 rc=0
2026-10-12 21:33:05 === variant sweep complete
```
