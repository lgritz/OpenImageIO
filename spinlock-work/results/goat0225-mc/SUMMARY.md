## testtex seconds (best of 3)

| workload | config | mc0 | mc16 | mc64 | mc256 |
|---|---|---|---|---|---|
| w1 | free-1 | 0.95 | 0.95 | 0.96 | 0.96 |
| w1 | s0cores-16 | 1.02 | 0.98 | 0.99 | 0.99 |
| w1 | split-16 | 0.98 | 1.02 | 1.04 | 1.01 |
| w1 | allcores-32 | 1.04 | 1.10 | 1.06 | 1.07 |
| w1 | free-64 | 1.89 | 1.89 | 1.88 | 1.90 |
| w4 | free-1 | 1.03 | 1.02 | 1.04 | 1.02 |
| w4 | s0cores-16 | 1.10 | 1.08 | 1.08 | 1.07 |
| w4 | split-16 | 1.15 | 1.06 | 1.04 | 1.08 |
| w4 | allcores-32 | 1.29 | 1.14 | 1.17 | 1.14 |
| w4 | free-64 | 2.33 | 2.09 | 2.08 | 2.08 |
| w7 | free-1 | 1.24 | 1.21 | 1.18 | 1.17 |
| w7 | s0cores-16 | 1.64 | 1.41 | 1.25 | 1.22 |
| w7 | split-16 | 2.11 | 1.59 | 1.26 | 1.17 |
| w7 | allcores-32 | 2.42 | 1.79 | 1.28 | 1.29 |
| w7 | free-64 | 4.39 | 3.03 | 2.28 | 2.22 |
| w8 | free-1 | 0.63 | 0.60 | 0.55 | 0.54 |
| w8 | s0cores-16 | 1.09 | 0.76 | 0.59 | 0.55 |
| w8 | split-16 | 1.54 | 1.03 | 0.61 | 0.54 |
| w8 | allcores-32 | 1.96 | 1.09 | 0.65 | 0.60 |
| w8 | free-64 | 3.56 | 2.03 | 1.15 | 1.03 |
| prod-c8192-w7 | free-1 | 2.00 | 2.03 | 1.99 | 1.95 |
| prod-c8192-w7 | s0cores-16 | 2.19 | 2.00 | 1.76 | 1.74 |
| prod-c8192-w7 | split-16 | 2.74 | 2.38 | 2.03 | 1.86 |
| prod-c8192-w7 | allcores-32 | 2.99 | 2.81 | 2.11 | 2.08 |
| prod-c8192-w7 | free-64 | 5.36 | 4.38 | 3.50 | 3.50 |
| prod-c8192-w8 | free-1 | 1.43 | 1.41 | 1.43 | 1.30 |
| prod-c8192-w8 | s0cores-16 | 1.58 | 1.41 | 1.08 | 1.12 |
| prod-c8192-w8 | split-16 | 2.16 | 1.80 | 1.46 | 1.37 |
| prod-c8192-w8 | allcores-32 | 2.57 | 2.01 | 1.43 | 1.40 |
| prod-c8192-w8 | free-64 | 4.28 | 2.97 | 2.18 | 2.03 |
| prod-c1024-w7 | free-1 | 2.05 | 2.04 | 1.99 | 1.96 |
| prod-c1024-w7 | s0cores-16 | 2.19 | 1.97 | 1.76 | 1.79 |
| prod-c1024-w7 | split-16 | 2.78 | 2.32 | 2.08 | 1.96 |
| prod-c1024-w7 | allcores-32 | 3.28 | 2.94 | 2.39 | 2.30 |
| prod-c1024-w7 | free-64 | 5.26 | 4.31 | 3.48 | 3.40 |
| prod-c1024-w8 | free-1 | 1.38 | 1.41 | 1.47 | 1.40 |
| prod-c1024-w8 | s0cores-16 | 1.61 | 1.42 | 1.14 | 1.12 |
| prod-c1024-w8 | split-16 | 2.05 | 1.67 | 1.42 | 1.41 |
| prod-c1024-w8 | allcores-32 | 2.68 | 2.14 | 1.50 | 1.44 |
| prod-c1024-w8 | free-64 | 3.94 | 3.00 | 2.29 | 2.19 |

## prod w7 --runstats at 64 threads

### stats-prod-c1024-w7-free-64-mc0
```
 64         5.26     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 32.8s (1.4s average per thread, for 65 threads)
  Tiles: 316650 created, 43633 current, 44658 peak
    micro-cache misses : 264410159 (97.1%)
    redundant reads: 113081 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
    1     73.9 MB ( 1.0%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,2477,676,0,0,0,0,0,0,0,0,0,0]
    2     73.8 MB ( 1.0%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,2479,671,0,0,0,0,0,0,0,0,0,0]
    3     19.0 MB ( 0.3%)  4096x4096x3.f16  prodtex200  MIP-COUNT[0,422,387,0,0,0,0,0,0,0,0,0,0]
    1    53.82 MB/s (18.66MB/0.35s)   4096x4096x3.f16  prodtex113  MIP-COUNT[0,417,379,0,0,0,0,0,0,0,0,0,0]
    2    55.24 MB/s (18.66MB/0.34s)   4096x4096x3.f16  prodtex5  MIP-COUNT[0,417,379,0,0,0,0,0,0,0,0,0,0]
    3    58.78 MB/s (18.56MB/0.32s)   4096x4096x3.f16  prodtex32  MIP-COUNT[0,416,376,0,0,0,0,0,0,0,0,0,0]
  Top files by redundant I/O:
    1     46.5 MB ( 1.8%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,2477,676,0,0,0,0,0,0,0,0,0,0]
    2     46.5 MB ( 1.8%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,2479,671,0,0,0,0,0,0,0,0,0,0]
    3      6.9 MB ( 0.3%)  4096x4096x3.f16  prodtex105  MIP-COUNT[0,420,386,0,0,0,0,0,0,0,0,0,0]
```

### stats-prod-c1024-w7-free-64-mc16
```
 64         4.39     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 21.6s (1.3s average per thread, for 65 threads)
  Tiles: 316557 created, 43633 current, 44522 peak
    micro-cache misses : 138621338 (50.9%)
    redundant reads: 112989 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
    1     73.9 MB ( 1.0%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,2475,681,0,0,0,0,0,0,0,0,0,0]
    2     73.9 MB ( 1.0%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,2478,677,0,0,0,0,0,0,0,0,0,0]
    3     19.0 MB ( 0.3%)  4096x4096x3.f16  prodtex64  MIP-COUNT[0,423,386,0,0,0,0,0,0,0,0,0,0]
    1    67.46 MB/s (18.82MB/0.28s)   4096x4096x3.f16  prodtex194  MIP-COUNT[0,419,384,0,0,0,0,0,0,0,0,0,0]
    2    67.99 MB/s (18.56MB/0.27s)   4096x4096x3.f16  prodtex2  MIP-COUNT[0,416,376,0,0,0,0,0,0,0,0,0,0]
    3    69.73 MB/s (18.49MB/0.27s)   4096x4096x3.f16  prodtex186  MIP-COUNT[0,416,373,0,0,0,0,0,0,0,0,0,0]
  Top files by redundant I/O:
    1     46.6 MB ( 1.8%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,2475,681,0,0,0,0,0,0,0,0,0,0]
    2     46.6 MB ( 1.8%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,2478,677,0,0,0,0,0,0,0,0,0,0]
    3      6.9 MB ( 0.3%)  4096x4096x3.f16  prodtex105  MIP-COUNT[0,420,386,0,0,0,0,0,0,0,0,0,0]
```

### stats-prod-c1024-w7-free-64-mc256
```
 64         3.44     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 17.0s (1.2s average per thread, for 65 threads)
  Tiles: 316728 created, 43633 current, 44331 peak
    micro-cache misses : 1231068 (0.5%)
    redundant reads: 113160 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
    1     74.3 MB ( 1.0%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,2505,665,0,0,0,0,0,0,0,0,0,0]
    2     74.3 MB ( 1.0%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,2503,668,0,0,0,0,0,0,0,0,0,0]
    3     19.0 MB ( 0.3%)  4096x4096x3.f16  prodtex200  MIP-COUNT[0,422,387,0,0,0,0,0,0,0,0,0,0]
    1    69.29 MB/s (18.80MB/0.27s)   4096x4096x3.f16  prodtex98  MIP-COUNT[0,419,383,0,0,0,0,0,0,0,0,0,0]
    2    74.68 MB/s (18.96MB/0.25s)   4096x4096x3.f16  prodtex64  MIP-COUNT[0,423,386,0,0,0,0,0,0,0,0,0,0]
    3    94.36 MB/s (74.32MB/0.79s)   4096x4096x3.f16  prodtex0  MIP-COUNT[0,2505,665,0,0,0,0,0,0,0,0,0,0]
  Top files by redundant I/O:
    1     46.9 MB ( 1.8%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,2503,668,0,0,0,0,0,0,0,0,0,0]
    2     46.9 MB ( 1.8%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,2505,665,0,0,0,0,0,0,0,0,0,0]
    3      6.9 MB ( 0.3%)  4096x4096x3.f16  prodtex200  MIP-COUNT[0,422,387,0,0,0,0,0,0,0,0,0,0]
```

### stats-prod-c1024-w7-free-64-mc64
```
 64         3.57     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 15.6s (1.2s average per thread, for 65 threads)
  Tiles: 316721 created, 43633 current, 44330 peak
    micro-cache misses : 16380858 (6.0%)
    redundant reads: 113153 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
    1     74.3 MB ( 1.0%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,2502,667,0,0,0,0,0,0,0,0,0,0]
    2     74.2 MB ( 1.0%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,2501,667,0,0,0,0,0,0,0,0,0,0]
    3     19.0 MB ( 0.3%)  4096x4096x3.f16  prodtex200  MIP-COUNT[0,422,387,0,0,0,0,0,0,0,0,0,0]
    1    70.98 MB/s (18.84MB/0.27s)   4096x4096x3.f16  prodtex109  MIP-COUNT[0,421,383,0,0,0,0,0,0,0,0,0,0]
    2    95.32 MB/s (74.23MB/0.78s)   4096x4096x3.f16  prodtex0  MIP-COUNT[0,2501,667,0,0,0,0,0,0,0,0,0,0]
    3   109.51 MB/s (74.27MB/0.68s)   4096x4096x3.f16  prodtex1  MIP-COUNT[0,2502,667,0,0,0,0,0,0,0,0,0,0]
  Top files by redundant I/O:
    1     46.9 MB ( 1.8%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,2501,667,0,0,0,0,0,0,0,0,0,0]
    2     46.9 MB ( 1.8%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,2502,667,0,0,0,0,0,0,0,0,0,0]
    3      6.9 MB ( 0.3%)  4096x4096x3.f16  prodtex200  MIP-COUNT[0,422,387,0,0,0,0,0,0,0,0,0,0]
```

### stats-prod-c256-w7-free-64-mc0
```
 64         5.08     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 23.5s (1.3s average per thread, for 65 threads)
  Tiles: 319317 created, 10908 current, 11538 peak
    micro-cache misses : 264410159 (97.1%)
    redundant reads: 115749 tiles, 2.6 GB
    Peak cache memory : 256.0 MB
    1    104.8 MB ( 1.4%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,3496,977,0,0,0,0,0,0,0,0,0,0]
    2    104.4 MB ( 1.4%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,3478,978,0,0,0,0,0,0,0,0,0,0]
    3     19.0 MB ( 0.3%)  4096x4096x3.f16  prodtex200  MIP-COUNT[0,422,387,0,0,0,0,0,0,0,0,0,0]
    1    66.25 MB/s (18.49MB/0.28s)   4096x4096x3.f16  prodtex373  MIP-COUNT[0,413,376,0,0,0,0,0,0,0,0,0,0]
    2    67.43 MB/s (18.66MB/0.28s)   4096x4096x3.f16  prodtex309  MIP-COUNT[0,419,377,0,0,0,0,0,0,0,0,0,0]
    3    68.03 MB/s (18.49MB/0.27s)   4096x4096x3.f16  prodtex186  MIP-COUNT[0,416,373,0,0,0,0,0,0,0,0,0,0]
  Top files by redundant I/O:
    1     77.5 MB ( 2.9%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,3496,977,0,0,0,0,0,0,0,0,0,0]
    2     77.1 MB ( 2.8%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,3478,978,0,0,0,0,0,0,0,0,0,0]
    3      6.9 MB ( 0.3%)  4096x4096x3.f16  prodtex105  MIP-COUNT[0,420,386,0,0,0,0,0,0,0,0,0,0]
```

### stats-prod-c256-w7-free-64-mc16
```
 64         4.41     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 24.0s (1.3s average per thread, for 65 threads)
  Tiles: 318973 created, 10908 current, 11223 peak
    micro-cache misses : 137669604 (50.5%)
    redundant reads: 115405 tiles, 2.6 GB
    Peak cache memory : 256.0 MB
    1    100.7 MB ( 1.3%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,3378,918,0,0,0,0,0,0,0,0,0,0]
    2    100.4 MB ( 1.3%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,3378,909,0,0,0,0,0,0,0,0,0,0]
    3     19.0 MB ( 0.3%)  4096x4096x3.f16  prodtex64  MIP-COUNT[0,423,386,0,0,0,0,0,0,0,0,0,0]
    1    64.19 MB/s (18.68MB/0.29s)   4096x4096x3.f16  prodtex106  MIP-COUNT[0,420,377,0,0,0,0,0,0,0,0,0,0]
    2    65.33 MB/s (18.59MB/0.28s)   4096x4096x3.f16  prodtex2  MIP-COUNT[0,417,376,0,0,0,0,0,0,0,0,0,0]
    3    67.16 MB/s (18.77MB/0.28s)   4096x4096x3.f16  prodtex241  MIP-COUNT[0,419,382,0,0,0,0,0,0,0,0,0,0]
  Top files by redundant I/O:
    1     73.4 MB ( 2.7%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,3378,918,0,0,0,0,0,0,0,0,0,0]
    2     73.1 MB ( 2.7%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,3378,909,0,0,0,0,0,0,0,0,0,0]
    3      6.9 MB ( 0.3%)  4096x4096x3.f16  prodtex105  MIP-COUNT[0,420,386,0,0,0,0,0,0,0,0,0,0]
```

### stats-prod-c256-w7-free-64-mc256
```
 64         5.66     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 39.5s (1.5s average per thread, for 65 threads)
  Tiles: 474343 created, 16384 current, 16479 peak
    micro-cache misses : 1310981 (0.5%)
    redundant reads: 270775 tiles, 6.2 GB
    Peak cache memory : 384.5 MB
    1    682.1 MB ( 6.1%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,14791,14313,0,0,0,0,0,0,0,0,0,0]
    2    644.9 MB ( 5.8%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,15194,12329,0,0,0,0,0,0,0,0,0,0]
    3     82.0 MB ( 0.7%)  4096x4096x3.f16  prodtex218  MIP-COUNT[0,573,2927,0,0,0,0,0,0,0,0,0,0]
    1    92.79 MB/s (23.20MB/0.25s)   4096x4096x3.f16  prodtex175  MIP-COUNT[0,472,518,0,0,0,0,0,0,0,0,0,0]
    2    96.61 MB/s (26.86MB/0.28s)   4096x4096x3.f16  prodtex274  MIP-COUNT[0,638,508,0,0,0,0,0,0,0,0,0,0]
    3    96.91 MB/s (26.23MB/0.27s)   4096x4096x3.f16  prodtex304  MIP-COUNT[0,736,383,0,0,0,0,0,0,0,0,0,0]
  Top files by redundant I/O:
    1    655.0 MB (10.3%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,14791,14313,0,0,0,0,0,0,0,0,0,0]
    2    617.8 MB ( 9.7%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,15194,12329,0,0,0,0,0,0,0,0,0,0]
    3     69.9 MB ( 1.1%)  4096x4096x3.f16  prodtex218  MIP-COUNT[0,573,2927,0,0,0,0,0,0,0,0,0,0]
```

### stats-prod-c256-w7-free-64-mc64
```
 64         3.53     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 14.6s (1.1s average per thread, for 65 threads)
  Tiles: 319043 created, 10908 current, 11134 peak
    micro-cache misses : 15898562 (5.8%)
    redundant reads: 115475 tiles, 2.6 GB
    Peak cache memory : 256.0 MB
    1    101.7 MB ( 1.4%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,3333,1004,0,0,0,0,0,0,0,0,0,0]
    2    101.1 MB ( 1.4%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,3315,1001,0,0,0,0,0,0,0,0,0,0]
    3     19.0 MB ( 0.3%)  4096x4096x3.f16  prodtex200  MIP-COUNT[0,422,387,0,0,0,0,0,0,0,0,0,0]
    1   101.65 MB/s (101.67MB/1.00s)   4096x4096x3.f16  prodtex0  MIP-COUNT[0,3333,1004,0,0,0,0,0,0,0,0,0,0]
    2   117.43 MB/s (101.09MB/0.86s)   4096x4096x3.f16  prodtex1  MIP-COUNT[0,3315,1001,0,0,0,0,0,0,0,0,0,0]
  Top files by redundant I/O:
    1     74.3 MB ( 2.7%)  4096x4096x3.f16  prodtex0  MIP-COUNT[0,3333,1004,0,0,0,0,0,0,0,0,0,0]
    2     73.8 MB ( 2.7%)  4096x4096x3.f16  prodtex1  MIP-COUNT[0,3315,1001,0,0,0,0,0,0,0,0,0,0]
    3      6.9 MB ( 0.3%)  4096x4096x3.f16  prodtex105  MIP-COUNT[0,420,386,0,0,0,0,0,0,0,0,0,0]
```
