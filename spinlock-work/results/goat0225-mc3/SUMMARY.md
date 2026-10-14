## testtex seconds (best of 3)

| workload | config | mc0 | mc64 | mc256 |
|---|---|---|---|---|
| w7 | free-1 | 1.24 | 1.17 | 1.16 |
| w7 | allcores-32 | 2.43 | 1.31 | 1.22 |
| w7 | free-64 | 4.41 | 2.38 | 2.20 |
| w8 | free-1 | 0.63 | 0.55 | 0.55 |
| w8 | allcores-32 | 1.91 | 0.70 | 0.60 |
| w8 | free-64 | 3.62 | 1.13 | 1.05 |
| prod-c8192-w7 | free-1 | 2.06 | 1.92 | 1.89 |
| prod-c8192-w7 | allcores-32 | 3.01 | 2.16 | 2.15 |
| prod-c8192-w7 | free-64 | 5.44 | 3.51 | 3.48 |
| prod-c8192-w8 | free-1 | 1.39 | 1.31 | 1.28 |
| prod-c8192-w8 | allcores-32 | 2.47 | 1.45 | 1.44 |
| prod-c8192-w8 | free-64 | 4.46 | 2.22 | 2.13 |
| prod-c256-w7 | free-1 | 2.03 | 1.96 | 1.92 |
| prod-c256-w7 | allcores-32 | 3.28 | 2.37 | 2.38 |
| prod-c256-w7 | free-64 | 5.46 | 3.67 | 3.54 |
| prod-c256-w8 | free-1 | 1.43 | 1.31 | 1.27 |
| prod-c256-w8 | allcores-32 | 2.65 | 1.69 | 1.64 |
| prod-c256-w8 | free-64 | 4.23 | 2.39 | 2.46 |

## w7 --runstats at 64 threads

### stats-prod-c1024-w7-free-64-mc0
```
 64         5.63     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 42.0s (1.6s average per thread, for 65 threads)
  Tiles: 316692 created, 43633 current, 44818 peak
    micro-cache misses : 264410159 (97.1%)
    redundant reads: 113124 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
  Top files by redundant I/O:
```

### stats-prod-c1024-w7-free-64-mc256
```
 64         3.67     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 19.0s (1.2s average per thread, for 65 threads)
  Tiles: 316641 created, 43633 current, 44366 peak
    micro-cache misses : 1314334 (0.5%)
    redundant reads: 113073 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
  Top files by redundant I/O:
```

### stats-prod-c1024-w7-free-64-mc64
```
 64         3.72     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 18.8s (1.2s average per thread, for 65 threads)
  Tiles: 316765 created, 43633 current, 44412 peak
    micro-cache misses : 15535339 (5.7%)
    redundant reads: 113197 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
  Top files by redundant I/O:
```

### stats-prod-c256-w7-free-64-mc0
```
 64         5.37     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 30.7s (1.4s average per thread, for 65 threads)
  Tiles: 319176 created, 10908 current, 11690 peak
    micro-cache misses : 264410159 (97.1%)
    redundant reads: 115608 tiles, 2.6 GB
    Peak cache memory : 256.0 MB
  Top files by redundant I/O:
```

### stats-prod-c256-w7-free-64-mc256
```
 64         3.64     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 13.9s (1.1s average per thread, for 65 threads)
  Tiles: 319756 created, 15499 current, 16290 peak
    micro-cache misses : 1586894 (0.6%)
    redundant reads: 116188 tiles, 2.7 GB
    Peak cache memory : 363.7 MB
    Sweeps unable to free enough (tiles in use) : 16
  Top files by redundant I/O:
```

### stats-prod-c256-w7-free-64-mc64
```
 64         3.70     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 13.9s (1.1s average per thread, for 65 threads)
  Tiles: 318972 created, 10908 current, 11331 peak
    micro-cache misses : 16185572 (5.9%)
    redundant reads: 115404 tiles, 2.6 GB
    Peak cache memory : 256.0 MB
  Top files by redundant I/O:
```

### stats-untiled-c128-w7-free-64-mc0
```
 64         7.92     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 32.0s (0.5s average per thread, for 65 threads)
  Tiles: 6064 created, 16 current, 52 peak
    micro-cache misses : 190628 (88.6%)
    redundant reads: 834 tiles, 13.0 GB
    Peak cache memory : 256.0 MB
    Evicted tiles held by microcaches : 16.0 MB peak, 0 flushes
    Sweeps unable to free enough (tiles in use) : 310
```

### stats-untiled-c128-w7-free-64-mc256
```
 64         0.19     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 1.4s (0.0s average per thread, for 65 threads)
  Tiles: 42 created, 16 current, 31 peak
    micro-cache misses : 188067 (87.4%)
    redundant reads: 6 tiles, 96.0 MB
    Peak cache memory : 256.0 MB
    Sweeps unable to free enough (tiles in use) : 12
```

### stats-untiled-c128-w7-free-64-mc64
```
 64         0.59     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 2.2s (0.0s average per thread, for 65 threads)
  Tiles: 648 created, 16 current, 42 peak
    micro-cache misses : 190628 (88.6%)
    redundant reads: 81 tiles, 1.3 GB
    Peak cache memory : 256.0 MB
    Sweeps unable to free enough (tiles in use) : 85
```

### stats-untiled-c32-w7-free-64-mc0
```
 64        19.16     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 1m 39.8s (1.5s average per thread, for 65 threads)
  Tiles: 11529 created, 16 current, 58 peak
    micro-cache misses : 190628 (88.6%)
    redundant reads: 1914 tiles, 29.9 GB
    Peak cache memory : 256.0 MB
    Evicted tiles held by microcaches : 16.0 MB peak, 0 flushes
    Sweeps unable to free enough (tiles in use) : 1832
```

### stats-untiled-c32-w7-free-64-mc256
```
 64        13.95     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 56.4s (0.9s average per thread, for 65 threads)
  Tiles: 14322 created, 16 current, 58 peak
    micro-cache misses : 190628 (88.6%)
    redundant reads: 2177 tiles, 34.0 GB
    Peak cache memory : 256.0 MB
    Evicted tiles held by microcaches : 16.0 MB peak, 8 flushes
    Sweeps unable to free enough (tiles in use) : 2066
```

### stats-untiled-c32-w7-free-64-mc64
```
 64        46.91     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 3m 56.4s (3.6s average per thread, for 65 threads)
  Tiles: 28133 created, 16 current, 57 peak
    micro-cache misses : 190628 (88.6%)
    redundant reads: 4313 tiles, 67.4 GB
    Peak cache memory : 256.0 MB
    Evicted tiles held by microcaches : 16.0 MB peak, 6 flushes
    Sweeps unable to free enough (tiles in use) : 4177
```

### stats-untiled-c512-w7-free-64-mc0
```
 64         0.13     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 1.1s (0.0s average per thread, for 65 threads)
  Tiles: 16 created, 16 current, 16 peak
    micro-cache misses : 190628 (88.6%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 256.0 MB
```

### stats-untiled-c512-w7-free-64-mc256
```
 64         0.13     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 1.0s (0.0s average per thread, for 65 threads)
  Tiles: 16 created, 16 current, 16 peak
    micro-cache misses : 632 (0.3%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 256.0 MB
```

### stats-untiled-c512-w7-free-64-mc64
```
 64         0.12     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 1.2s (0.0s average per thread, for 65 threads)
  Tiles: 16 created, 16 current, 16 peak
    micro-cache misses : 13088 (6.1%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 256.0 MB
```
