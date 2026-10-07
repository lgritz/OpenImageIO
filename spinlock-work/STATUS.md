# STATUS: spin_mutex performance study (attempt 3)

If the machine crashed: read "Crash diagnosis" and "Run log" below. The
last `START` line without an `END` in the run log (and in
`$BENCH/bench.log`) names the culprit. Check for a new panic log:
`ls -lt /Library/Logs/DiagnosticReports/ | head`.

Plan file: ~/.claude/plans/pasted-content-id-06f3-i-m-interested-transient-cloud.md
Bench dir ($BENCH): ~/tmp/claude-501/-Users-lg-code-oiio-oiio-lg/c4fd8081-2a55-4816-8c19-f609e618a5f6/scratchpad/spin3

## Original task (user's words)
> I'm interested in seeing if we can improve performance of the spin_mutex
> class in thread.h. In addition to the current implementation, I'm curious
> about the exponential backoff as described in
> https://david.alvarezrosa.com/posts/optimizing-a-spin-lock/
> Also, am interested in any difference between our current use of
> std::atomic_flag versus that article's use of std::atomic_bool.
> Also, I was looking at https://en.cppreference.com/cpp/atomic/atomic_flag
> and see that it is documenting some additional C++20 features that we
> should assess (when building in C++20 mode). Our minimum is C++17, but
> maybe there are changes to this class that can be done conditionally on
> if we use C++20 that will help performance.
> I'm interested in you evaluating these choices, with benchmarks and
> reasoning, and making best recommendations.
>
> Plus: (1) think hard about anything that could crash the machine;
> (2) keep this STATUS.md updated before running anything so a crash
> can be diagnosed and resumed without re-explaining.

## Crash diagnosis (attempts 1 and 2), done 2026-10-06
- Panic logs: /Library/Logs/DiagnosticReports/panic-full-2026-10-06-231920.0002.panic
  and panic-full-2026-10-06-234209.0002.panic.
  Both: `Spinlock[...] timeout after ~12.58M ticks @locks.c:798`,
  xnu-12377.161.14 (Darwin 25.6.0), T6041, 16 cores.
- Panicked task was our bench both times (bench20, spinbench20). UUIDs match.
- Symbolicated panicking thread:
  crash 1 = atomic_flag::wait -> libc++ poll_with_backoff -> `__ulock_wait`;
  crash 2 = atomic_flag::notify_one -> `__ulock_wake`.
- ROOT CAUSE: C++20 atomic_flag wait/notify under 16-thread contention
  -> macOS ulock syscalls -> kernel spinlock timeout panic (XNU bug).
- Partial results from attempt 2: ~/tmp/claude-501/-Users-lg-code-oiio-oiio-lg/306ac692-8665-454f-8e38-58e2ccfcc68f/scratchpad/run1.txt

## Safety rules
- NEVER build/run anything using atomic wait/notify/futex/ulock.
  Before compiling: `grep -nE '\.wait\(|notify_' spinbench.cpp` must be empty.
- Bench uses alarm(300), fsync after each row, one variant per process.
- No-yield spin variants: <= 16 threads. Yielding variants: up to 32.
- Driver logs START/END + sync. Run under nice -n 5. No concurrent builds.
- Builds: only the needed targets, -j8. Ctest: spin/thread/ustring only, outside the sandbox.

## Latent bugs found in thread.h
1. pause() checks `__arm__`, not `__aarch64__`. On Apple Silicon and Linux
   arm64 it is an empty loop with no spin hint.
2. `*(volatile bool*)&m_locked` reads an atomic_flag as a bool. This is UB,
   and it is what TSan flags as a DCLP race.
3. MSVC atomic_flag is 4 bytes and atomic<bool> is 1. Switching types
   changes sizeof(spin_mutex) on Windows (ABI). OK for main only.

## Article's final lock (V4)
atomic_bool. Lock = `exchange(true, acquire)`; on failure, loop
`do { pause x backoff; backoff = min(backoff*2, 64) } while (load(relaxed))`.
The backoff never resets inside one lock() call and it NEVER yields.
Unlock = `store(false, release)`. Pause = `_mm_pause` (x86 only).
Article results (x86, pinned threads): 4 threads V4 = 43 ns vs TTAS = 120 ns.
It does not discuss atomic_flag or C++20 wait/notify.
Difference from OIIO: OIIO caps at 16 pauses and then calls
std::this_thread::yield() on every round. OIIO uses atomic_flag with the
volatile hack.

## Checklist
- [x] 0 STATUS.md + memory
- [x] 1 Fetch article, record its code
- [x] 2 Write bench + driver, smoke run, full run
- [x] 3 Analyze, recommend
- [x] 4 Implement in thread.h
- [x] 5 Build and test (spinlock_test, spin_rw_test, thread_test, ustring_test)
- [x] 6 Final report

## Run log (newest last)
- 23:57 step2: compiling spinbench17/20 (compile only, safe)
- 23:57 step2: running pausecost, then SMOKE run.bash 200000 1 smoke.txt (all variants, threads 1 4 16 32). Log: $BENCH/bench.log
- 23:57 step2: smoke OK (smoke.txt). pausecost: isb=8.4ns, yield-insn=0.25ns. nice is blocked by the sandbox, so it was removed. Now FULL run: run.bash 4000000 5 full.txt (background). If crash: check tail of $BENCH/bench.log
- 23:58 step4 (partial, source only): thread.h pause() now has an __aarch64__ isb branch. Not built yet.
- 00:06 FULL run done, no crash. Results in $BENCH/full.txt (format with `python3 -I table.py full.txt spinbench20`).
  Findings: (a) Early yield is the bottleneck at 8-12 threads (A_cur 55-100ns wall, 440-1000 cpu).
  The article (no yield) gets 2.6/17 at 8 threads and 6.4/75 at 12, but degrades at 16 and is unsafe when oversubscribed.
  (b) DCLP off is much worse, so TTAS matters. (c) The aarch64 yield insn is bad; use isb.
  (d) C++20 flag.test() is the same as the C++17 equivalents. (e) flag vs bool codegen is IDENTICAL on arm64
  (swpab/stlrb/ldrb). The 1-thread medium gap of 13 vs 15ns is __builtin_expect layout.
  Next: round 2 = finalists + longer spin-before-yield grid, 9 trials, spinbench17 only.
- 00:08 ROUND 2 running: BINS=spinbench17 run.bash 4000000 9 round2.txt '^(A_cur|B_flag_isb_16y1|B3|B4|D_article|E_bool_isb_64y64|E_bool_isb_128y16|X_)' 2 4 8 12 16 32 (background)
- 00:11 ROUND 2 done ($BENCH/round2.txt). WINNER: X_bool_isb_128y256 = atomic<bool> TTAS, isb pause,
  exponential to 128 pauses, then 256 more rounds at 128, then yield. Tiny, wall ns, A_cur -> winner:
  8 threads 55.7->8.6, 12: 65->5.5, 16: 24->8.7, 32: 12->4.9. Medium: 8: 61->19, 12: 73->19, 16: 39->27.
  Uncontended unchanged (~0.85ns). Article D is better at 2-8 threads tiny but bad at 16 and unsafe >16.
  Step 4: implementing in thread.h. atomic_backoff gets an optional `spins` param (default keeps the old
  behavior because imagecache wait_pixels_ready uses it for I/O waits).
- 00:12 step4 done (thread.h edited). step5: BASELINE: old build/bin/spinlock_test --wedge --iters 4000000 --trials 3 (old code, safe)
- 00:12 baseline rerun with default iters 40M
- 00:12 step5: building targets spinlock_test spin_rw_test thread_test ustring_test -j8 (compile only)
- 00:12 step5: running NEW spinlock_test --wedge --trials 3, then spin_rw_test
- 00:13 new spinlock_test: better at 2-8 threads, WORSE at 16 (0.2->0.7s). Suspect lock+counter colocated in spinlock_test. Adding colocated shape to the bench.
- 00:14 ROUND 3 running: SHAPES=coloc BINS=spinbench17 run.bash 4000000 7 round3.txt finalists, threads 2 4 8 12 14 16 32 (background)
- 00:14 round3 coloc done (round3.txt): 128y256 best in bench at all threads incl 16. Investigating why spinlock_test regresses at 16; added O_oiio_spin_mutex variant (real header).
- 00:15 hypothesis: spinlock_test has no start barrier, so long spinning starves the thread-creating main thread at 16. Testing NOBARRIER=1 coloc runs.
- 00:16 NOBARRIER did not reproduce. Note: at 32 threads the current code is better (3.3 vs 7.1). Probing spinlock_test --threads N directly.
- 00:17 REPRODUCED: bench at -O3 (OIIO's flag) is 2.6x slower than at -O2 at 12-16 threads. Diffing codegen.
- 00:30 Cause of the O3 gap: not isb unrolling (isbcost.cpp: same ns/isb). At O3 the unlock->relock path in the
  tight loop is longer (out-of-line lock call), so handoffs happen more often. Tiny-CS benches mostly measure
  lock unfairness and are chaotic. Weight medium + coloc more.
  ROUND 4 running: BINS=spinbench17o3 SHAPES="tiny medium coloc" run.bash 4000000 5 round4.txt (finalists + O_oiio) threads 2 4 8 12 16 32 (background)
- 00:19 ROUND 4 done (round4.txt). Real-header O_oiio is worse than identical inline X_128y256: at O3 pause(128) is unrolled into 128 isb, so lock() gets too big to inline. Fix: block unrolling in pause().
- 00:21 added OIIO_PAUSE_NOUNROLL to pause(). Rebuilding the O3 bench + comparing O_oiio vs X_128y256.
- 00:24 FOUND: spin_mutex::lock had an inverted hint, !OIIO_UNLIKELY(try_lock()). O2 variant (bench lock loop + OIIO backoff) is fast, so the class is the problem. Fixed to OIIO_UNLIKELY(!try_lock()). Retesting.
- 00:24 hint fix confirmed: O_oiio now == X_128y256 in all shapes. Constants stay 128/256. Step5: rebuilding OIIO test targets -j8, then running spinlock_test.
- 00:25 A/B real spinlock_test old vs new (160M iters, per thread count)
- 00:26 A/B: new 1.6-11x faster at 2-12 threads, but old wins at 16/32 (2.5 vs 6.9ns). Grid of (pausemax,spins) in real spinlock_test via header copies in $BENCH/inc_*
- 00:50 Grid in real spinlock_test (80M iters, wall/cpu ns per op):
      var       4T      8T       12T      16T     32T
      old     3.8/21  26.2/216 18.8/222  5.0/62  2.5/41
      128_256 1.2/8   2.5/23   2.5/36    6.2/95  7.5/122
      128_64  1.2/9   2.5/23   3.8/40    5.0/78  6.2/95
      128_16  1.2/9   2.5/25   5.0/66    5.0/82  5.0/76
      64_64   1.2/11  3.8/34   6.2/77   16.2/242 16.2/252
  DECISION: atomic_backoff(128, 64) in spin_mutex (shorter spin budget, about 70us on M4, safer on x86).
  Step5: rebuild + run tests.
- 00:28 step5: ctest -R 'unit_spinlock|unit_spin_rw|unit_ustring|unit_thread|texture-threadtimes$' (outside sandbox)
- 00:29 testtex baseline (OLD lib) --threadtimes 1,4,7 --wedge, outputs in $TMPDIR/testtex_old_*.txt
- 00:30 old testtex (sec, weak scaling) m1: 1T .89 8T 1.53 12T 2.08 16T 4.78 | m4: .96 1.23 1.75 2.70 | m7: 1.11 1.34 1.48 1.82. Now building testtex -j8 (background).
- 00:30 testtex rebuilt; running NEW threadtimes 1,4,7
- 00:31 !!! TEMP: src/include/OpenImageIO/thread.h REVERTED to HEAD for an old-testtex A/B. NEW version saved at $BENCH/thread.h.NEW. If crash: cp $BENCH/thread.h.NEW src/include/OpenImageIO/thread.h
- 00:32 old testtex == new within noise (m1 16T 2.7-3.4 both). thread.h RESTORED to NEW. Rebuilding testtex + unit tests.
- 00:33 TSan small test clean; C++20 compile clean. Final ctest subset.
- ctest: unit_spinlock, unit_spin_rw, unit_ustring, unit_thread and texture-threadtimes PASS. texture-texture3d
  fails only because build/testsuite/openvdb/src/sphere.vdb is missing (the openvdb test was not in the -R
  filter, so its fixture dir was never set up). Unrelated.

## FINAL REPORT (all steps done; nothing committed)
Changes in src/include/OpenImageIO/thread.h:
1. pause(): new `__aarch64__` branch using `isb`. Before, arm64 matched no branch and got an empty loop.
   `yield` is about a nop on Apple (0.25ns) and benched badly; isb is about 8ns.
   Added OIIO_PAUSE_NOUNROLL: at -O3, clang fully unrolled pause(128) into 128 isb, so lock() stopped inlining.
2. spin_mutex: atomic_flag -> std::atomic<bool>. TTAS spin on load(relaxed) replaces the UB
   `*(volatile bool*)&flag` cast, so it is TSan-clean with no DCLP switch. Codegen on arm64 is identical
   (swpab/stlrb/ldrb), so this is a correctness/cleanliness change, not a speed change.
3. spin_mutex::lock: fixed the inverted branch hint `!OIIO_UNLIKELY(try_lock())` -> `OIIO_UNLIKELY(!try_lock())`.
   It was worth up to 2x at 16-32 threads in the medium-work bench.
4. atomic_backoff gains an optional `spins` param (default 0 = the old behavior; imagecache wait_pixels_ready
   still uses the default). spin_mutex uses atomic_backoff(128, 64): exponential up to 128 pauses, then 64
   more rounds at 128, then yield.
C++20: atomic_flag::test() gives nothing over atomic<bool>::load. wait()/notify() is REJECTED: about 4x slower
  uncontended (3.4 vs 0.85ns), and under contention it PANICKED the macOS kernel twice (ulock).
  No conditional C++20 code is needed.
Article (V4, never yields): best at 2-8 threads, bad at 16 (=cores) and unsafe when oversubscribed.
  The chosen design keeps most of its gain and still yields.
Real spinlock_test A/B (ns/op wall): 2T 1.9->1.2, 4T 4.4->1.2, 8T 26.9->2.5, 12T 9.4->3.8,
  16T 5.0->5.0, 32T 2.5->6.2. Old wins only when oversubscribed with an empty critical section.
testtex threadtimes 1/4/7: unchanged within noise (ImageCache is not spin_mutex-bound).
Caveats: x86 not measured (pause is about 35-140 cycles, so the spin budget before yield is ~0.1-0.4ms; should be
  checked on a Linux x86 box). MSVC sizeof(spin_mutex) changes 4->1 (atomic_flag is 4 bytes there), which is an
  ABI change, so main only, no dev-3.1 backport. OIIO_THREAD_ALLOW_DCLP is now unused; left defined for compat.
  spin_rw_mutex is unchanged (it still uses the default backoff); candidate for the same tuning later.

## Phase 2 (2026-10-07): commit + x86 handoff
- Committed bad4c87e4 "perf(thread): faster spin_mutex under contention; fix arm64 pause" on lg-spinlock.
- Handoff folder (untracked): spinlock-x86-handoff/ (README.md = agent instructions, bench, M4 results).
- EXPERIMENT (uncommitted): ImageCacheTile::use() test-before-write (imagecache_pvt.h ~802). Hypothesis: the seq_cst
  store on the shared tile line per lookup kills scaling. Rebuilding testtex; compare testtex --threadtimes 1/2/4/7.
  If crash/abort: `git checkout src/libtexture/imagecache_pvt.h` reverts it.
- tile use() experiment RESULT (testtex weak scaling, sec): m1 16T 2.9-4.8 -> 2.2-2.3, 8T 1.37-1.40 -> 1.06-1.14;
  m2 16T 2.7-3.3 -> 2.2; m4 16T 2.6-2.7 -> 2.2; m7 unchanged. Saved as spinlock-x86-handoff/tile-use-test-before-write.patch,
  then REVERTED in the tree (not committed). build/bin/testtex still contains the experiment until the next rebuild.
- 00:51 Handoff README written: spinlock-x86-handoff/README.md. Phase 2 done. Branch not pushed.
- 09:08 phase3: committed 114a92be0 (tile use). Working tree: file use() atomic + file_sweep std::mutex. Building OIIO tests to verify.
- 09:09 ctest texture|openvdb|imagecache|unit_ -j4 (outside sandbox)
- 09:11 phase3: commits fdf355203 (file use) 7624b8e90 (file sweep std::mutex). Moved STATUS.md + handoff into spinlock-work/. Writing remote scripts.
- 09:15 DRY RUN of remote/run_all.sh: QUICK=1 VARIANTS='base sweep' WORK=$TMPDIR/spinwork, results in spinlock-work/results/*-quick (background)
- 09:19 dry run 2: STEPS='spinrw parallel prodtex' with TEXLIST of testsuite .tx files
- 09:20 dry run 3: STEPS=ctest VARIANTS='base sweep' (full build of sweep worktree, then ctest; outside sandbox)
- 10:11 Remote scripts validated by local dry run (results/dryrun-m4-quick): build, ctest (84/84 after
  excluding texture-udim, whose font differs inside a worktree), spinbench, spinlock/spinrw/parallel,
  testtex, prodtex + name scrubbing. Fixes found by the dry run: spin_rw_test needs --wedge; Python3_EXECUTABLE
  passed to cmake; image repos symlinked into worktrees; openvdb fixture no longer gates ctest.
  Finding: workload 7 has 94.9% micro-cache misses (2-tile microcache).
- Committing spinlock-work/ for push to GitHub. NEXT: the user runs spinlock-work/remote/run_all.sh on the
  x86 box and pushes results/<run>/; then analyze SUMMARY.md and decide on the micro-cache experiment.
