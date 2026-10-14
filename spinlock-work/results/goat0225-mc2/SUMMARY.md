## testtex seconds (best of 3)

| workload | config | mc0 | mc64 | mc256 |
|---|---|---|---|---|
| w7 | free-1 | 1.26 | 1.17 | 1.18 |
| w7 | allcores-32 | 2.43 | 1.32 | 1.27 |
| w7 | free-64 | 4.39 | 2.40 | 2.27 |
| w8 | free-1 | 0.63 | 0.56 | 0.54 |
| w8 | allcores-32 | 1.94 | 0.64 | 0.63 |
| w8 | free-64 | 3.61 | 1.18 | 1.05 |
| prod-c8192-w7 | free-1 | 2.04 | 1.94 | 1.91 |
| prod-c8192-w7 | allcores-32 | 3.07 | 2.32 | 2.13 |
| prod-c8192-w7 | free-64 | 5.21 | 3.67 | 3.55 |
| prod-c8192-w8 | free-1 | 1.40 | 1.30 | 1.25 |
| prod-c8192-w8 | allcores-32 | 2.44 | 1.46 | 1.46 |
| prod-c8192-w8 | free-64 | 4.23 | 2.27 | 2.08 |
| prod-c256-w7 | free-1 | 2.01 | 1.90 | 1.90 |
| prod-c256-w7 | allcores-32 | 3.37 | 2.32 | 2.37 |
| prod-c256-w7 | free-64 | 5.32 | 3.60 | 3.53 |
| prod-c256-w8 | free-1 | 1.40 | 1.34 | 1.25 |
| prod-c256-w8 | allcores-32 | 2.69 | 1.64 | 1.60 |
| prod-c256-w8 | free-64 | 4.19 | 2.41 | 2.37 |

## prod w7 --runstats at 64 threads

### stats-prod-c1024-w7-free-64-mc0
```
 64         5.68     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 43.2s (1.6s average per thread, for 65 threads)
  Tiles: 316673 created, 43633 current, 44300 peak
    micro-cache misses : 264410159 (97.1%)
    redundant reads: 113105 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
  Top files by redundant I/O:
```

### stats-prod-c1024-w7-free-64-mc256
```
 64         3.59     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 14.7s (1.1s average per thread, for 65 threads)
  Tiles: 316694 created, 43633 current, 44647 peak
    micro-cache misses : 1397819 (0.5%)
    redundant reads: 113126 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
    Evicted tiles held by microcaches : 44.5 MB peak, 0 flushes
  Top files by redundant I/O:
```

### stats-prod-c1024-w7-free-64-mc64
```
 64         3.70     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 16.0s (1.2s average per thread, for 65 threads)
  Tiles: 316716 created, 43633 current, 44418 peak
    micro-cache misses : 17296767 (6.3%)
    redundant reads: 113148 tiles, 2.6 GB
    Peak cache memory : 1024.0 MB
    Evicted tiles held by microcaches : 2.0 MB peak, 0 flushes
  Top files by redundant I/O:
```

### stats-prod-c256-w7-free-64-mc0
```
 64         5.31     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 28.3s (1.4s average per thread, for 65 threads)
  Tiles: 319220 created, 10908 current, 12068 peak
    micro-cache misses : 264410159 (97.1%)
    redundant reads: 115652 tiles, 2.6 GB
    Peak cache memory : 256.0 MB
    Evicted tiles held by microcaches : 144 KB peak, 0 flushes
  Top files by redundant I/O:
```

### stats-prod-c256-w7-free-64-mc256
```
 64         3.51     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 12.2s (1.1s average per thread, for 65 threads)
  Tiles: 319250 created, 10908 current, 11620 peak
    micro-cache misses : 1578690 (0.6%)
    redundant reads: 115682 tiles, 2.6 GB
    Peak cache memory : 256.0 MB
    Evicted tiles held by microcaches : 35.4 MB peak, 32 flushes
  Top files by redundant I/O:
```

### stats-prod-c256-w7-free-64-mc64
```
 64         3.69     64.0x   100.0%    range 0.00	(2000000 iters/thread)
    File I/O time : 1m 13.3s (1.1s average per thread, for 65 threads)
  Tiles: 319008 created, 10908 current, 11474 peak
    micro-cache misses : 16678700 (6.1%)
    redundant reads: 115440 tiles, 2.6 GB
    Peak cache memory : 256.0 MB
    Evicted tiles held by microcaches : 21.0 MB peak, 722 flushes
  Top files by redundant I/O:
```
