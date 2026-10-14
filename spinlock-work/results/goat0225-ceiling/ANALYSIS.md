# goat0225 scaling ceiling (2026-10-13)

The lg-spinlock build (= phase 1 + 2). 2 sockets x 16 cores x 2 SMT. Each config
is best of 3; the ranges are small (prod split-16: 0.08 s).

## Raw (seconds)
| config | T | scalectl | w1 | w7 synth | w7 prod 8 GB |
|---|---|---|---|---|---|
| free | 1 | 0.603 | 0.95 | 1.28 | 2.01 |
| free | 16 | 0.616 | 0.97 | 2.05 | 2.20 |
| s0cores (1 socket, 1/core) | 16 | 0.663 | 0.99 | 1.68 | 2.14 |
| split (8+8 cores) | 16 | 0.618 | 1.00 | 2.08 | 2.74 |
| free | 24 | 0.619 | 1.44 | 2.26 | 2.59 |
| free | 32 | 0.667 | 1.68 | 2.76 | 2.94 |
| allcores (2 sockets, 1/core) | 32 | 0.681 | 1.06 | 2.42 | 3.04 |
| s0all (1 socket + SMT) | 32 | 1.261 | 1.89 | 2.82 | 3.49 |
| free | 64 | 1.278 | 1.92 | 4.33 | 5.26 |

## Residual = (time / own 1T) / (scalectl / its 1T)
1.00 means the machine's limit, so no software cost.

| config | T | machine | w1 | w7 synth | w7 prod |
|---|---|---|---|---|---|
| free | 16 | 1.02 | 1.00 | 1.57 | 1.07 |
| s0cores | 16 | 1.10 | 0.95 | 1.19 | 0.97 |
| split | 16 | 1.02 | 1.03 | 1.59 | 1.34 |
| free | 24 | 1.03 | 1.47 | 1.71 | 1.25 |
| free | 32 | 1.11 | 1.59 | 1.94 | 1.32 |
| allcores | 32 | 1.13 | 0.99 | 1.67 | 1.34 |
| s0all | 32 | 2.09 | 0.95 | 1.05 | 0.83 |
| free | 64 | 2.12 | 0.95 | 1.60 | 1.24 |

## Findings
1. **Machine ceiling:** all-core turbo costs about 13%. SMT gives this ALU loop
   nothing (2.09x at 2 threads/core). So 64T can be no better than about 2.1x 1T.
2. **w1 sits at the ceiling everywhere it is pinned** (residual 0.95-1.03).
   Phase 2 left no software cost on the per-thread-microcache hit path.
3. **The w1 jump at 24-32T in the wedges was the Linux scheduler, not OIIO.**
   Unpinned 32T is 1.68 s, while one thread per core is 1.06 s: free threads are
   stacked on SMT siblings while cores sit idle. The earlier free 24/32T wedge
   numbers overstate the slowdown.
4. **All remaining ImageCache cost is crossing sockets.** On one socket, w7
   is at or under the ceiling even at 32 threads with SMT: residual 1.05 synthetic,
   0.83 prod (memory-bound work does get something from SMT). As soon as threads
   span both sockets:
   - synthetic w7: 1.6-1.7x (16T split, 32T allcores, 64T)
   - prod w7: 1.24-1.34x
   At the same 16 threads, split vs one socket costs +24% synthetic and +28%
   prod.
5. Where it comes from: synthetic w7 (25 MB, all threads share the first 2
   files' tiles) fits in L3. So its cross-socket cost is coherence traffic:
   the bin `spin_rw_mutex` reader count and the tile refcount RMW on every
   main-cache lookup (97% of lookups miss the 2-entry microcache). Prod w7
   (4.7 GB) adds remote-NUMA DRAM reads of the pixels, which no locking change fixes.

## What this means for phase 3
A bigger per-thread microcache turns most w7 lookups into thread-local hits:
no bin lock and no refcount RMW, so none of the cross-socket line traffic.
- Upper bound: synthetic w7 at 64T 4.33 -> about 2.7 s (1.6x). Prod w7 at most
  5.26 -> about 4.2 s (1.24x); some of that is NUMA memory, which stays.
- Single-socket boxes (bear0056, and goat with one socket) have nothing left to
  gain, so phase 3 must not slow the 1T and one-socket paths.
- Measure phase 3 with pinned configs (s0cores/split/allcores). The free runs are
  dominated by scheduler noise above 16T.

## Verdict
Phase 3 is justified for multi-socket machines (render farms are mostly
2-socket). Target: w7 split-16 / allcores-32 residual from 1.6 toward 1.1
synthetic, and from 1.34 toward 1.1 prod, with no regression on w1 or
on one socket.
