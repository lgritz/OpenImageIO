# goat0225 (2-socket Xeon Gold 6226R): phase 1 and 2 analysis

Runs: `goat0225-full`, `goat0225-prod-c8192`, `goat0225-prod-c1024`
(2026-10-13).

Machine: 2 sockets x 16 cores x 2 SMT, so 32 cores and 64 hw threads. There are
2 NUMA nodes (node0 = even CPUs). Cascade Lake, GCC 11.5, governor
`performance`. There is no `numactl` and no `perf`, so no node0 spinbench and no
profiles. ctest passes 86/86.

## Phase 1 (lg-spinmutex): holds on 2 sockets; no change to the PR

### spinlock_test (s, best of 3, lower is better)
| T | 1 | 2 | 4 | 8 | 16 | 32 | 48 | 64 | 96 | 128 |
|---|---|---|---|---|---|---|---|---|---|---|
| base | 1.2 | 1.8 | 7.4 | 27.2 | 47.5 | 75.5 | 74.1 | 76.7 | 60.9 | 58.0 |
| spin | 1.2 | 1.3 | 2.2 | 3.9 | 10.9 | 47.2 | 52.8 | 61.6 | 59.1 | 54.4 |

The new spin_mutex is never worse, and 7x faster at 8T, 4.4x at 16T, 1.6x at
32T and 1.25x at 64T. Process CPU time is 2-6x lower at 4-32T. The cliff at
32T is where the threads span both sockets.

### spinbench (wall ns/op), the shipped lock (O = 128y64) vs old (A_cur)
- 4-16T: 2.3-5x faster than the old lock in every shape (tiny 8T: 29.5 vs 158).
- 32T+: noisy (the cross-socket line transfer dominates), and on average
  equal to or better than the old lock.
- std::mutex is flat (tiny ~120, coloc ~75 ns) and **beats every spin
  variant at 16T and above** on this box. Spin wins below that (uncontended
  7.6 vs 15 ns; 8T tiny 29 vs 120). So: a spin_mutex is right only for locks
  that are rarely contended by many threads at once. Locks that are hot across
  threads belong on std::mutex, which is what phase 2 did for the file sweep lock.

### Pause cost and backoff constants
- The measured `pause` cost is 11.5 ns (not ~45 ns as feared). backoff(128,64)
  is about 8450 pauses, about 97 us before the first yield. Zen3 is
  about 123 us. Same ballpark, so there's no per-arch tuning needed.
- 128y256 vs 128y64: 256 is 25-35% better at 32T+ on goat. On bear0056 it is
  10-20% worse when 2x oversubscribed (64T on 32 hw threads). It is equal
  at 16T and below on both. The gain shows up only in the regime where std::mutex is better
  anyway. **Keep 128y64.**

### testtex, base vs spin
Spin_mutex is not on the texture lookup hot path, so spin vs base measures
noise:
- Prod w1/w4/w7/w8 at 64T: 0.95-1.04x.
- spin_rw_test (spin_rw_mutex code unchanged): +/-9%, which is the noise floor here.
- The synthetic w1 wedge looks like a regression (64T: 14.9 vs 7.2 s), but
  it is chaos in the use() line ping-pong regime: base has a 5.7 s range at 64T.
  The single 64T `--runstats` run shows the opposite (spin 9.96 vs base
  12.21), and prod w1 is 1.01x / 0.95x. Phase 2 removes this regime entirely.

### parallel_test
Spin is +8-13% at 32T+ and -17% at 2T. The 2T number is noise: it sits within the
run-to-run spread seen on bear0056.

## Phase 2 (lg-ic-contention): large wins at high core count

Speedup of sweep vs base at 64T (weak scaling):

| workload | synthetic | prod 8 GB | prod 1 GB |
|---|---|---|---|
| w1 hot file, handles | 3.80x | 5.68x | 5.40x |
| w2 hot file, names | 5.97x | - | - |
| w4 one file, offsets | 2.76x | 4.21x | 3.00x |
| w7 many files | 1.23x | 1.15x | 1.10x |
| w8 many files, no ALU | 1.30x | 1.12x | 1.12x |
| maxfiles16 / cache64 | 1.05x / 1.06x | | |

- w1/w2/w4 are now flat (perfect weak scaling) up to 16T. base degrades
  3.4-4.4x by 16T.
- Nothing is slower, at any thread count, in any config.
- prod 1 GB at 64T: 2.6 GB of redundant reads (113k tiles); there is no difference between
  variants, and eviction costs about 5% vs 8 GB (w7 4.96 vs 4.77).

## What is left: scaling above 16 threads

Sweep, weak scaling, time relative to 1T:

| workload | 16T | 24T | 32T | 64T |
|---|---|---|---|---|
| w1 synth (0% microcache miss) | 1.02 | 1.57 | 1.85 | 2.00 |
| w7 prod 8 GB (97% microcache miss) | 1.04 | 1.20 | 1.41 | 2.28 |
| w8 prod 8 GB | 1.13 | 1.32 | 1.54 | 2.66 |

w1 never leaves the per-thread microcache, yet it degrades as much as w7
does. So most of the loss above 16T is **not** ImageCache contention. Likely causes are the
all-core turbo drop, SMT sharing (above 32T), and scheduler placement / socket
crossing (24T on 32 cores should not need SMT, but it still jumps 1.5x).

w7/w8 lose at most 15-30% more than w1 at 64T. That part is the main cache path
(bin rw-lock fetch_add plus tile refcount RMW, cross-socket), and it is the most
phase 3 (a bigger microcache) could win on this box. w7 rotates over 8 files per
pixel, so a 2-entry microcache misses 97% of the time. Real shaders
reading 5-20 textures per shading point look like w7.

To size phase 3 before building it, the next cheap run should measure the
machine's own ceiling: a compute-only control (same per-iteration ALU, no texture
calls) at the same thread counts, plus w1/w7 pinned to one socket vs
spread across both (`taskset`). Residual = sweep / control.

## Verdict
- Phase 1 PR: confirmed on 2-socket Intel. Keep the constants.
- Phase 2 PR: confirmed. The win grows with core count (3-6x at 64T on the hot-file
  workloads).
- Phase 3: worth up to ~15-30% on many-texture workloads at 64T. Run the control first
  to confirm how much is really contention.
