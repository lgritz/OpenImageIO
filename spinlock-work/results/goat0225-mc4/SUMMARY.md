## testtex seconds (best of 3)

| workload | config | mc0 | mc64 | mc256 |
|---|---|---|---|---|
| w7 | free-1 | 1.26 | 1.19 | 1.18 |
| w7 | allcores-32 | 2.44 | 1.32 | 1.23 |
| w7 | free-64 | 4.45 | 2.30 | 2.24 |
| w8 | free-1 | 0.63 | 0.56 | 0.54 |
| w8 | allcores-32 | 1.94 | 0.64 | 0.61 |
| w8 | free-64 | 3.63 | 1.20 | 1.15 |
| prod-c8192-w7 | free-1 | 2.05 | 1.94 | 1.93 |
| prod-c8192-w7 | allcores-32 | 3.00 | 2.24 | 2.19 |
| prod-c8192-w7 | free-64 | 5.36 | 3.60 | 3.83 |
| prod-c8192-w8 | free-1 | 1.39 | 1.32 | 1.28 |
| prod-c8192-w8 | allcores-32 | 2.39 | 1.48 | 1.41 |
| prod-c8192-w8 | free-64 | 4.17 | 2.31 | 2.22 |
| prod-c256-w7 | free-1 | 2.04 | 1.94 | 1.90 |
| prod-c256-w7 | allcores-32 | 3.26 | 2.30 | 2.37 |
| prod-c256-w7 | free-64 | 5.44 | 3.86 | 3.59 |
| prod-c256-w8 | free-1 | 1.43 | 1.32 | 1.30 |
| prod-c256-w8 | allcores-32 | 2.75 | 1.67 | 1.66 |
| prod-c256-w8 | free-64 | 4.28 | 2.51 | 2.42 |

## w7 --runstats at 64 threads

### stats-prod-c1024-w7-free-64-mc0
```
 64         5.57     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 35.3s (1.5s average per thread, for 65 threads)
  Tiles: 316645 created, 43633 current, 44693 peak
    micro-cache misses : 264410159 (97.1%)
    redundant reads: 113077 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
  Top files by redundant I/O:
```

### stats-prod-c1024-w7-free-64-mc256
```
 64         3.74     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 21.9s (1.3s average per thread, for 65 threads)
  Tiles: 316708 created, 43633 current, 44582 peak
    micro-cache misses : 1341736 (0.5%)
    redundant reads: 113140 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
    Microcache victim slots : 383.7 MB peak (summed over threads), evicted but held 0 B peak, 0 flushes
  Top files by redundant I/O:
```

### stats-prod-c1024-w7-free-64-mc64
```
 64         3.87     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 18.0s (1.2s average per thread, for 65 threads)
  Tiles: 316687 created, 43633 current, 44450 peak
    micro-cache misses : 18914517 (6.9%)
    redundant reads: 113119 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
    Microcache victim slots : 95.5 MB peak (summed over threads), evicted but held 0 B peak, 0 flushes
  Top files by redundant I/O:
```

### stats-prod-c256-w7-free-64-mc0
```
 64         5.33     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 29.1s (1.4s average per thread, for 65 threads)
  Tiles: 318883 created, 10908 current, 11682 peak
    micro-cache misses : 264410159 (97.1%)
    redundant reads: 115315 tiles, 2.6 GB
    Peak cache memory : 256.0 MB
  Top files by redundant I/O:
```

### stats-prod-c256-w7-free-64-mc256
```
 64         3.69     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 17.0s (1.2s average per thread, for 65 threads)
  Tiles: 318883 created, 10908 current, 11263 peak
    micro-cache misses : 1279082 (0.5%)
    redundant reads: 115315 tiles, 2.6 GB
    Peak cache memory : 256.0 MB
    Microcache victim slots : 219.5 MB peak (summed over threads), evicted but held 0 B peak, 71 flushes
  Top files by redundant I/O:
```

### stats-prod-c256-w7-free-64-mc64
```
 64         3.80     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 12.5s (1.1s average per thread, for 65 threads)
  Tiles: 319114 created, 10908 current, 11458 peak
    micro-cache misses : 15952157 (5.9%)
    redundant reads: 115546 tiles, 2.6 GB
    Peak cache memory : 256.0 MB
    Microcache victim slots : 95.4 MB peak (summed over threads), evicted but held 0 B peak, 0 flushes
  Top files by redundant I/O:
```

### stats-untiled-c128-w7-free-64-mc0
```
 64        23.91     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 1m 39.7s (1.5s average per thread, for 65 threads)
  Tiles: 18284 created, 16 current, 53 peak
    micro-cache misses : 190628 (88.6%)
    redundant reads: 2642 tiles, 41.3 GB
    Peak cache memory : 256.0 MB
    Microcache victim slots : 0 B peak (summed over threads), evicted but held 16.0 MB peak, 0 flushes
    Sweeps unable to free enough (tiles in use) : 865
```

### stats-untiled-c128-w7-free-64-mc256
```
 64        20.80     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 1m 28.3s (1.4s average per thread, for 65 threads)
  Tiles: 14832 created, 16 current, 62 peak
    micro-cache misses : 187133 (86.9%)
    redundant reads: 1895 tiles, 29.6 GB
    Peak cache memory : 256.0 MB
    Microcache victim slots : 2.0 GB peak (summed over threads), evicted but held 16.0 MB peak, 31 flushes
    Sweeps unable to free enough (tiles in use) : 278
```

### stats-untiled-c128-w7-free-64-mc64
```
 64         2.82     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 10.0s (0.2s average per thread, for 65 threads)
  Tiles: 2442 created, 16 current, 43 peak
    micro-cache misses : 180497 (83.8%)
    redundant reads: 282 tiles, 4.4 GB
    Peak cache memory : 256.0 MB
    Microcache victim slots : 2.5 GB peak (summed over threads), evicted but held 0 B peak, 8 flushes
    Sweeps unable to free enough (tiles in use) : 127
```

### stats-untiled-c32-w7-free-64-mc0
```
 64        25.73     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 2m 9.7s (2.0s average per thread, for 65 threads)
  Tiles: 16868 created, 16 current, 53 peak
    micro-cache misses : 190628 (88.6%)
    redundant reads: 2587 tiles, 40.4 GB
    Peak cache memory : 256.0 MB
    Microcache victim slots : 0 B peak (summed over threads), evicted but held 32.0 MB peak, 0 flushes
    Sweeps unable to free enough (tiles in use) : 2527
```

### stats-untiled-c32-w7-free-64-mc256
```
 64        50.25     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 4m 28.2s (4.1s average per thread, for 65 threads)
  Tiles: 27561 created, 16 current, 60 peak
    micro-cache misses : 190628 (88.6%)
    redundant reads: 4513 tiles, 70.5 GB
    Peak cache memory : 256.0 MB
    Microcache victim slots : 1.2 GB peak (summed over threads), evicted but held 16.0 MB peak, 8 flushes
    Sweeps unable to free enough (tiles in use) : 4382
```

### stats-untiled-c32-w7-free-64-mc64
```
 64        20.12     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 1m 31.2s (1.4s average per thread, for 65 threads)
  Tiles: 15257 created, 16 current, 48 peak
    micro-cache misses : 190621 (88.6%)
    redundant reads: 2140 tiles, 33.4 GB
    Peak cache memory : 256.0 MB
    Microcache victim slots : 1.1 GB peak (summed over threads), evicted but held 16.0 MB peak, 6 flushes
    Sweeps unable to free enough (tiles in use) : 2074
```

### stats-untiled-c512-w7-free-64-mc0
```
 64         0.11     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 0.9s (0.0s average per thread, for 65 threads)
  Tiles: 17 created, 16 current, 16 peak
    micro-cache misses : 190628 (88.6%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 256.0 MB
```

### stats-untiled-c512-w7-free-64-mc256
```
 64         0.14     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 1.2s (0.0s average per thread, for 65 threads)
  Tiles: 16 created, 16 current, 16 peak
    micro-cache misses : 632 (0.3%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 256.0 MB
```

### stats-untiled-c512-w7-free-64-mc64
```
 64         0.11     64.0x   100.0%    range 0.00	(3125 iters/thread)
    File I/O time : 1.0s (0.0s average per thread, for 65 threads)
  Tiles: 16 created, 16 current, 16 peak
    micro-cache misses : 632 (0.3%)
    redundant reads: 0 tiles, 0 B
    Peak cache memory : 256.0 MB
```
