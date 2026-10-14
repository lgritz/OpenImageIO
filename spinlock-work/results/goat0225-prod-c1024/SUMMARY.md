# Results: goat0225-prod-c1024

## Machine
<details><summary>machine.txt</summary>

```
## date
Tue Oct 13 04:58:01 PM PDT 2026
## git
94fe319a629722549859ace2358d4b2b9e5592ba spinlock-work: next_run.sh for the 64-core Xeon (full + prod 8G/1G)
## uname
Linux goat0225.spimageworks.com 5.14.0-687.46.1.el9_8.x86_64 #1 SMP PREEMPT_DYNAMIC Thu Sep 10 14:23:01 UTC 2026 x86_64 x86_64 x86_64 GNU/Linux
## nproc
64
## lscpu
Architecture:                            x86_64
CPU op-mode(s):                          32-bit, 64-bit
Address sizes:                           46 bits physical, 48 bits virtual
Byte Order:                              Little Endian
CPU(s):                                  64
On-line CPU(s) list:                     0-63
Vendor ID:                               GenuineIntel
Model name:                              Intel(R) Xeon(R) Gold 6226R CPU @ 2.90GHz
CPU family:                              6
Model:                                   85
Thread(s) per core:                      2
Core(s) per socket:                      16
Socket(s):                               2
Stepping:                                7
CPU(s) scaling MHz:                      93%
CPU max MHz:                             3900.0000
CPU min MHz:                             1200.0000
BogoMIPS:                                5800.00
Virtualization:                          VT-x
L1d cache:                               1 MiB (32 instances)
L1i cache:                               1 MiB (32 instances)
L2 cache:                                32 MiB (32 instances)
L3 cache:                                44 MiB (2 instances)
NUMA node(s):                            2
NUMA node0 CPU(s):                       0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62
NUMA node1 CPU(s):                       1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63
## numa
numactl not available
## smt
1
## governor
performance
## memory
               total        used        free      shared  buff/cache   available
Mem:             187          15          40           0         132         171
Swap:            255           6         249
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

**prod-c1024-stats-w7 (weak scaling)**

| variant | 64 |
|---|---|
| base | 5.79 |
| spin | 5.78 |
| sweep | 5.25 |

Speedup vs base at 64 threads: spin 1.00x, sweep 1.10x

**prod-c1024-w1 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 | 64 |
|---|---|---|---|---|---|---|---|---|---|
| base | 0.96 | 1.22 | 1.47 | 1.93 | 3.31 | 3.54 | 4.76 | 6.09 | 10.31 |
| spin | 0.96 | 1.21 | 1.85 | 2.06 | 2.68 | 3.70 | 6.14 | 5.77 | 10.25 |
| sweep | 0.95 | 0.97 | 0.97 | 0.97 | 0.98 | 0.97 | 1.48 | 1.82 | 1.91 |

Speedup vs base at 64 threads: spin 1.01x, sweep 5.40x

**prod-c1024-w4 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 | 64 |
|---|---|---|---|---|---|---|---|---|---|
| base | 1.06 | 1.24 | 1.31 | 1.54 | 1.99 | 2.89 | 4.05 | 5.77 | 6.30 |
| spin | 1.07 | 1.23 | 1.43 | 1.55 | 2.34 | 3.10 | 4.57 | 5.45 | 6.07 |
| sweep | 1.05 | 1.08 | 1.09 | 1.09 | 1.09 | 1.14 | 1.62 | 1.73 | 2.10 |

Speedup vs base at 64 threads: spin 1.04x, sweep 3.00x

**prod-c1024-w7 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 | 64 |
|---|---|---|---|---|---|---|---|---|---|
| base | 2.08 | 2.03 | 2.15 | 2.29 | 2.38 | 2.41 | 2.97 | 3.40 | 5.48 |
| spin | 2.10 | 2.03 | 2.11 | 2.34 | 2.41 | 2.40 | 2.95 | 3.47 | 5.45 |
| sweep | 2.08 | 1.98 | 2.06 | 2.14 | 2.17 | 2.19 | 2.75 | 3.15 | 4.96 |

Speedup vs base at 64 threads: spin 1.01x, sweep 1.10x

**prod-c1024-w8 (weak scaling)**

| variant | 1 | 2 | 4 | 8 | 12 | 16 | 24 | 32 | 64 |
|---|---|---|---|---|---|---|---|---|---|
| base | 1.42 | 1.42 | 1.61 | 1.75 | 1.73 | 1.81 | 2.12 | 2.57 | 4.30 |
| spin | 1.45 | 1.39 | 1.54 | 1.62 | 1.80 | 1.77 | 2.11 | 2.49 | 4.23 |
| sweep | 1.43 | 1.35 | 1.42 | 1.57 | 1.56 | 1.66 | 1.93 | 2.34 | 3.83 |

Speedup vs base at 64 threads: spin 1.02x, sweep 1.12x

<details><summary>testtex-base-prod-c1024-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --threadtimes 7 --threads 64 --runstats --cachesize 1024 --maxfiles 1000 prodtex0 prodtex1 prodtex2 prodtex3 prodtex4 prodtex5 prodtex6 prodtex7 prodtex8 prodtex9 prodtex10 prodtex11 prodtex12 prodtex13 prodtex14 prodtex15 prodtex16 prodtex17 prodtex18 prodtex19 prodtex20 prodtex21 prodtex22 prodtex23 prodtex24 prodtex25 prodtex26 prodtex27 prodtex28 prodtex29 prodtex30 prodtex31 prodtex32 prodtex33 prodtex34 prodtex35 prodtex36 prodtex37 prodtex38 prodtex39 prodtex40 prodtex41 prodtex42 prodtex43 prodtex44 prodtex45 prodtex46 prodtex47 prodtex48 prodtex49 prodtex50 prodtex51 prodtex52 prodtex53 prodtex54 prodtex55 prodtex56 prodtex57 prodtex58 prodtex59 prodtex60 prodtex61 prodtex62 prodtex63 prodtex64 prodtex65 prodtex66 prodtex67 prodtex68 prodtex69 prodtex70 prodtex71 prodtex72 prodtex73 prodtex74 prodtex75 prodtex76 prodtex77 prodtex78 prodtex79 prodtex80 prodtex81 prodtex82 prodtex83 prodtex84 prodtex85 prodtex86 prodtex87 prodtex88 prodtex89 prodtex90 prodtex91 prodtex92 prodtex93 prodtex94 prodtex95 prodtex96 prodtex97 prodtex98 prodtex99 prodtex100 prodtex101 prodtex102 prodtex103 prodtex104 prodtex105 prodtex106 prodtex107 prodtex108 prodtex109 prodtex110 prodtex111 prodtex112 prodtex113 prodtex114 prodtex115 prodtex116 prodtex117 prodtex118 prodtex119 prodtex120 prodtex121 prodtex122 prodtex123 prodtex124 prodtex125 prodtex126 prodtex127 prodtex128 prodtex129 prodtex130 prodtex131 prodtex132 prodtex133 prodtex134 prodtex135 prodtex136 prodtex137 prodtex138 prodtex139 prodtex140 prodtex141 prodtex142 prodtex143 prodtex144 prodtex145 prodtex146 prodtex147 prodtex148 prodtex149 prodtex150 prodtex151 prodtex152 prodtex153 prodtex154 prodtex155 prodtex156 prodtex157 prodtex158 prodtex159 prodtex160 prodtex161 prodtex162 prodtex163 prodtex164 prodtex165 prodtex166 prodtex167 prodtex168 prodtex169 prodtex170 prodtex171 prodtex172 prodtex173 prodtex174 prodtex175 prodtex176 prodtex177 prodtex178 prodtex179 prodtex180 prodtex181 prodtex182 prodtex183 prodtex184 prodtex185 prodtex186 prodtex187 prodtex188 prodtex189 prodtex190 prodtex191 prodtex192 prodtex193 prodtex194 prodtex195 prodtex196 prodtex197 prodtex198 prodtex199 prodtex200 prodtex201 prodtex202 prodtex203 prodtex204 prodtex205 prodtex206 prodtex207 prodtex208 prodtex209 prodtex210 prodtex211 prodtex212 prodtex213 prodtex214 prodtex215 prodtex216 prodtex217 prodtex218 prodtex219 prodtex220 prodtex221 prodtex222 prodtex223 prodtex224 prodtex225 prodtex226 prodtex227 prodtex228 prodtex229 prodtex230 prodtex231 prodtex232 prodtex233 prodtex234 prodtex235 prodtex236 prodtex237 prodtex238 prodtex239 prodtex240 prodtex241 prodtex242 prodtex243 prodtex244 prodtex245 prodtex246 prodtex247 prodtex248 prodtex249 prodtex250 prodtex251 prodtex252 prodtex253 prodtex254 prodtex255 prodtex256 prodtex257 prodtex258 prodtex259 prodtex260 prodtex261 prodtex262 prodtex263 prodtex264 prodtex265 prodtex266 prodtex267 prodtex268 prodtex269 prodtex270 prodtex271 prodtex272 prodtex273 prodtex274 prodtex275 prodtex276 prodtex277 prodtex278 prodtex279 prodtex280 prodtex281 prodtex282 prodtex283 prodtex284 prodtex285 prodtex286 prodtex287 prodtex288 prodtex289 prodtex290 prodtex291 prodtex292 prodtex293 prodtex294 prodtex295 prodtex296 prodtex297 prodtex298 prodtex299 prodtex300 prodtex301 prodtex302 prodtex303 prodtex304 prodtex305 prodtex306 prodtex307 prodtex308 prodtex309 prodtex310 prodtex311 prodtex312 prodtex313 prodtex314 prodtex315 prodtex316 prodtex317 prodtex318 prodtex319 prodtex320 prodtex321 prodtex322 prodtex323 prodtex324 prodtex325 prodtex326 prodtex327 prodtex328 prodtex329 prodtex330 prodtex331 prodtex332 prodtex333 prodtex334 prodtex335 prodtex336 prodtex337 prodtex338 prodtex339 prodtex340 prodtex341 prodtex342 prodtex343 prodtex344 prodtex345 prodtex346 prodtex347 prodtex348 prodtex349 prodtex350 prodtex351 prodtex352 prodtex353 prodtex354 prodtex355 prodtex356 prodtex357 prodtex358 prodtex359 prodtex360 prodtex361 prodtex362 prodtex363 prodtex364 prodtex365 prodtex366 prodtex367 prodtex368 prodtex369 prodtex370 prodtex371 prodtex372 prodtex373 prodtex374 prodtex375 prodtex376 prodtex377 prodtex378 prodtex379 prodtex380 prodtex381 prodtex382 prodtex383 prodtex384 prodtex385 prodtex386 prodtex387 prodtex388 prodtex389 prodtex390 prodtex391
hw threads = 64
threads  time (s)   speedup efficiency
    ImageInputs : 392 created, 392 current, 392 peak
    Total pixel data size of all images referenced : 49.0 GB
    File I/O time : 1m 40.7s (1.5s average per thread, for 65 threads)
    ImageInput mutex locking time : 0.5s
  Tiles: 316716 created, 43633 current, 44620 peak
    micro-cache misses : 264410159 (97.1%)
    redundant reads: 113147 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
  Top files by redundant I/O:
wall=6.024 cpu=308.134 rc=0
```
</details>

<details><summary>testtex-spin-prod-c1024-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --threadtimes 7 --threads 64 --runstats --cachesize 1024 --maxfiles 1000 prodtex0 prodtex1 prodtex2 prodtex3 prodtex4 prodtex5 prodtex6 prodtex7 prodtex8 prodtex9 prodtex10 prodtex11 prodtex12 prodtex13 prodtex14 prodtex15 prodtex16 prodtex17 prodtex18 prodtex19 prodtex20 prodtex21 prodtex22 prodtex23 prodtex24 prodtex25 prodtex26 prodtex27 prodtex28 prodtex29 prodtex30 prodtex31 prodtex32 prodtex33 prodtex34 prodtex35 prodtex36 prodtex37 prodtex38 prodtex39 prodtex40 prodtex41 prodtex42 prodtex43 prodtex44 prodtex45 prodtex46 prodtex47 prodtex48 prodtex49 prodtex50 prodtex51 prodtex52 prodtex53 prodtex54 prodtex55 prodtex56 prodtex57 prodtex58 prodtex59 prodtex60 prodtex61 prodtex62 prodtex63 prodtex64 prodtex65 prodtex66 prodtex67 prodtex68 prodtex69 prodtex70 prodtex71 prodtex72 prodtex73 prodtex74 prodtex75 prodtex76 prodtex77 prodtex78 prodtex79 prodtex80 prodtex81 prodtex82 prodtex83 prodtex84 prodtex85 prodtex86 prodtex87 prodtex88 prodtex89 prodtex90 prodtex91 prodtex92 prodtex93 prodtex94 prodtex95 prodtex96 prodtex97 prodtex98 prodtex99 prodtex100 prodtex101 prodtex102 prodtex103 prodtex104 prodtex105 prodtex106 prodtex107 prodtex108 prodtex109 prodtex110 prodtex111 prodtex112 prodtex113 prodtex114 prodtex115 prodtex116 prodtex117 prodtex118 prodtex119 prodtex120 prodtex121 prodtex122 prodtex123 prodtex124 prodtex125 prodtex126 prodtex127 prodtex128 prodtex129 prodtex130 prodtex131 prodtex132 prodtex133 prodtex134 prodtex135 prodtex136 prodtex137 prodtex138 prodtex139 prodtex140 prodtex141 prodtex142 prodtex143 prodtex144 prodtex145 prodtex146 prodtex147 prodtex148 prodtex149 prodtex150 prodtex151 prodtex152 prodtex153 prodtex154 prodtex155 prodtex156 prodtex157 prodtex158 prodtex159 prodtex160 prodtex161 prodtex162 prodtex163 prodtex164 prodtex165 prodtex166 prodtex167 prodtex168 prodtex169 prodtex170 prodtex171 prodtex172 prodtex173 prodtex174 prodtex175 prodtex176 prodtex177 prodtex178 prodtex179 prodtex180 prodtex181 prodtex182 prodtex183 prodtex184 prodtex185 prodtex186 prodtex187 prodtex188 prodtex189 prodtex190 prodtex191 prodtex192 prodtex193 prodtex194 prodtex195 prodtex196 prodtex197 prodtex198 prodtex199 prodtex200 prodtex201 prodtex202 prodtex203 prodtex204 prodtex205 prodtex206 prodtex207 prodtex208 prodtex209 prodtex210 prodtex211 prodtex212 prodtex213 prodtex214 prodtex215 prodtex216 prodtex217 prodtex218 prodtex219 prodtex220 prodtex221 prodtex222 prodtex223 prodtex224 prodtex225 prodtex226 prodtex227 prodtex228 prodtex229 prodtex230 prodtex231 prodtex232 prodtex233 prodtex234 prodtex235 prodtex236 prodtex237 prodtex238 prodtex239 prodtex240 prodtex241 prodtex242 prodtex243 prodtex244 prodtex245 prodtex246 prodtex247 prodtex248 prodtex249 prodtex250 prodtex251 prodtex252 prodtex253 prodtex254 prodtex255 prodtex256 prodtex257 prodtex258 prodtex259 prodtex260 prodtex261 prodtex262 prodtex263 prodtex264 prodtex265 prodtex266 prodtex267 prodtex268 prodtex269 prodtex270 prodtex271 prodtex272 prodtex273 prodtex274 prodtex275 prodtex276 prodtex277 prodtex278 prodtex279 prodtex280 prodtex281 prodtex282 prodtex283 prodtex284 prodtex285 prodtex286 prodtex287 prodtex288 prodtex289 prodtex290 prodtex291 prodtex292 prodtex293 prodtex294 prodtex295 prodtex296 prodtex297 prodtex298 prodtex299 prodtex300 prodtex301 prodtex302 prodtex303 prodtex304 prodtex305 prodtex306 prodtex307 prodtex308 prodtex309 prodtex310 prodtex311 prodtex312 prodtex313 prodtex314 prodtex315 prodtex316 prodtex317 prodtex318 prodtex319 prodtex320 prodtex321 prodtex322 prodtex323 prodtex324 prodtex325 prodtex326 prodtex327 prodtex328 prodtex329 prodtex330 prodtex331 prodtex332 prodtex333 prodtex334 prodtex335 prodtex336 prodtex337 prodtex338 prodtex339 prodtex340 prodtex341 prodtex342 prodtex343 prodtex344 prodtex345 prodtex346 prodtex347 prodtex348 prodtex349 prodtex350 prodtex351 prodtex352 prodtex353 prodtex354 prodtex355 prodtex356 prodtex357 prodtex358 prodtex359 prodtex360 prodtex361 prodtex362 prodtex363 prodtex364 prodtex365 prodtex366 prodtex367 prodtex368 prodtex369 prodtex370 prodtex371 prodtex372 prodtex373 prodtex374 prodtex375 prodtex376 prodtex377 prodtex378 prodtex379 prodtex380 prodtex381 prodtex382 prodtex383 prodtex384 prodtex385 prodtex386 prodtex387 prodtex388 prodtex389 prodtex390 prodtex391
hw threads = 64
threads  time (s)   speedup efficiency
    ImageInputs : 392 created, 392 current, 392 peak
    Total pixel data size of all images referenced : 49.0 GB
    File I/O time : 1m 37.2s (1.5s average per thread, for 65 threads)
    ImageInput mutex locking time : 0.3s
  Tiles: 316685 created, 43633 current, 44399 peak
    micro-cache misses : 264410159 (97.1%)
    redundant reads: 113117 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
  Top files by redundant I/O:
wall=5.998 cpu=308.288 rc=0
```
</details>

<details><summary>testtex-sweep-prod-c1024-stats-w7.txt</summary>

```
$ /net/soft_scratch/users/lg/code/openimageio-feedstock/build/bin/testtex --threadtimes 7 --threads 64 --runstats --cachesize 1024 --maxfiles 1000 prodtex0 prodtex1 prodtex2 prodtex3 prodtex4 prodtex5 prodtex6 prodtex7 prodtex8 prodtex9 prodtex10 prodtex11 prodtex12 prodtex13 prodtex14 prodtex15 prodtex16 prodtex17 prodtex18 prodtex19 prodtex20 prodtex21 prodtex22 prodtex23 prodtex24 prodtex25 prodtex26 prodtex27 prodtex28 prodtex29 prodtex30 prodtex31 prodtex32 prodtex33 prodtex34 prodtex35 prodtex36 prodtex37 prodtex38 prodtex39 prodtex40 prodtex41 prodtex42 prodtex43 prodtex44 prodtex45 prodtex46 prodtex47 prodtex48 prodtex49 prodtex50 prodtex51 prodtex52 prodtex53 prodtex54 prodtex55 prodtex56 prodtex57 prodtex58 prodtex59 prodtex60 prodtex61 prodtex62 prodtex63 prodtex64 prodtex65 prodtex66 prodtex67 prodtex68 prodtex69 prodtex70 prodtex71 prodtex72 prodtex73 prodtex74 prodtex75 prodtex76 prodtex77 prodtex78 prodtex79 prodtex80 prodtex81 prodtex82 prodtex83 prodtex84 prodtex85 prodtex86 prodtex87 prodtex88 prodtex89 prodtex90 prodtex91 prodtex92 prodtex93 prodtex94 prodtex95 prodtex96 prodtex97 prodtex98 prodtex99 prodtex100 prodtex101 prodtex102 prodtex103 prodtex104 prodtex105 prodtex106 prodtex107 prodtex108 prodtex109 prodtex110 prodtex111 prodtex112 prodtex113 prodtex114 prodtex115 prodtex116 prodtex117 prodtex118 prodtex119 prodtex120 prodtex121 prodtex122 prodtex123 prodtex124 prodtex125 prodtex126 prodtex127 prodtex128 prodtex129 prodtex130 prodtex131 prodtex132 prodtex133 prodtex134 prodtex135 prodtex136 prodtex137 prodtex138 prodtex139 prodtex140 prodtex141 prodtex142 prodtex143 prodtex144 prodtex145 prodtex146 prodtex147 prodtex148 prodtex149 prodtex150 prodtex151 prodtex152 prodtex153 prodtex154 prodtex155 prodtex156 prodtex157 prodtex158 prodtex159 prodtex160 prodtex161 prodtex162 prodtex163 prodtex164 prodtex165 prodtex166 prodtex167 prodtex168 prodtex169 prodtex170 prodtex171 prodtex172 prodtex173 prodtex174 prodtex175 prodtex176 prodtex177 prodtex178 prodtex179 prodtex180 prodtex181 prodtex182 prodtex183 prodtex184 prodtex185 prodtex186 prodtex187 prodtex188 prodtex189 prodtex190 prodtex191 prodtex192 prodtex193 prodtex194 prodtex195 prodtex196 prodtex197 prodtex198 prodtex199 prodtex200 prodtex201 prodtex202 prodtex203 prodtex204 prodtex205 prodtex206 prodtex207 prodtex208 prodtex209 prodtex210 prodtex211 prodtex212 prodtex213 prodtex214 prodtex215 prodtex216 prodtex217 prodtex218 prodtex219 prodtex220 prodtex221 prodtex222 prodtex223 prodtex224 prodtex225 prodtex226 prodtex227 prodtex228 prodtex229 prodtex230 prodtex231 prodtex232 prodtex233 prodtex234 prodtex235 prodtex236 prodtex237 prodtex238 prodtex239 prodtex240 prodtex241 prodtex242 prodtex243 prodtex244 prodtex245 prodtex246 prodtex247 prodtex248 prodtex249 prodtex250 prodtex251 prodtex252 prodtex253 prodtex254 prodtex255 prodtex256 prodtex257 prodtex258 prodtex259 prodtex260 prodtex261 prodtex262 prodtex263 prodtex264 prodtex265 prodtex266 prodtex267 prodtex268 prodtex269 prodtex270 prodtex271 prodtex272 prodtex273 prodtex274 prodtex275 prodtex276 prodtex277 prodtex278 prodtex279 prodtex280 prodtex281 prodtex282 prodtex283 prodtex284 prodtex285 prodtex286 prodtex287 prodtex288 prodtex289 prodtex290 prodtex291 prodtex292 prodtex293 prodtex294 prodtex295 prodtex296 prodtex297 prodtex298 prodtex299 prodtex300 prodtex301 prodtex302 prodtex303 prodtex304 prodtex305 prodtex306 prodtex307 prodtex308 prodtex309 prodtex310 prodtex311 prodtex312 prodtex313 prodtex314 prodtex315 prodtex316 prodtex317 prodtex318 prodtex319 prodtex320 prodtex321 prodtex322 prodtex323 prodtex324 prodtex325 prodtex326 prodtex327 prodtex328 prodtex329 prodtex330 prodtex331 prodtex332 prodtex333 prodtex334 prodtex335 prodtex336 prodtex337 prodtex338 prodtex339 prodtex340 prodtex341 prodtex342 prodtex343 prodtex344 prodtex345 prodtex346 prodtex347 prodtex348 prodtex349 prodtex350 prodtex351 prodtex352 prodtex353 prodtex354 prodtex355 prodtex356 prodtex357 prodtex358 prodtex359 prodtex360 prodtex361 prodtex362 prodtex363 prodtex364 prodtex365 prodtex366 prodtex367 prodtex368 prodtex369 prodtex370 prodtex371 prodtex372 prodtex373 prodtex374 prodtex375 prodtex376 prodtex377 prodtex378 prodtex379 prodtex380 prodtex381 prodtex382 prodtex383 prodtex384 prodtex385 prodtex386 prodtex387 prodtex388 prodtex389 prodtex390 prodtex391
hw threads = 64
threads  time (s)   speedup efficiency
    ImageInputs : 392 created, 392 current, 392 peak
    Total pixel data size of all images referenced : 49.0 GB
    File I/O time : 1m 30.2s (1.4s average per thread, for 65 threads)
    ImageInput mutex locking time : 0.4s
  Tiles: 316661 created, 43633 current, 44211 peak
    micro-cache misses : 264410159 (97.1%)
    redundant reads: 113092 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
  Top files by redundant I/O:
wall=5.493 cpu=267.478 rc=0
```
</details>

## Progress log tail
```
2026-10-13 17:09:40 === variant spin complete
2026-10-13 17:09:40 START checkout sweep (1a9ee4ea5)
2026-10-13 17:09:40 START build.sh sweep
2026-10-13 17:09:45 END build.sh sweep OK
2026-10-13 17:09:45 START testtex sweep prod-c1024-w1
2026-10-13 17:10:19 END testtex sweep prod-c1024-w1 wall=34.014 cpu=742.103 rc=0
2026-10-13 17:10:19 START testtex sweep prod-c1024-w4
2026-10-13 17:10:56 END testtex sweep prod-c1024-w4 wall=36.600 cpu=775.130 rc=0
2026-10-13 17:10:56 START testtex sweep prod-c1024-w7
2026-10-13 17:12:08 END testtex sweep prod-c1024-w7 wall=72.254 cpu=1480.348 rc=0
2026-10-13 17:12:08 START testtex sweep prod-c1024-w8
2026-10-13 17:13:01 END testtex sweep prod-c1024-w8 wall=52.641 cpu=1031.590 rc=0
2026-10-13 17:13:01 START testtex sweep prod-c1024-stats-w7
2026-10-13 17:13:06 END testtex sweep prod-c1024-stats-w7 wall=5.493 cpu=267.478 rc=0
2026-10-13 17:13:07 === variant sweep complete
```
