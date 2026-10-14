// Compute-only weak-scaling control: each thread runs the same ALU loop
// testtex adds between texture calls (30 x nchannels cosf), with no shared
// data at all. Its slowdown vs 1 thread is the machine's own ceiling
// (turbo, SMT, scheduler placement), to subtract from testtex scaling.
//
//   scalectl [iters_per_thread] [threads...]
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

static volatile float sink;

static void
work(long iters)
{
    float result[4] = { 0.1f, 0.2f, 0.3f, 0.4f };
    for (long i = 0; i < iters; ++i) {
        for (int j = 0; j < 30; ++j)
            for (int c = 0; c < 4; ++c)
                result[c] = cosf(result[c]);
        result[i & 3] += 1e-7f;
    }
    sink = result[0] + result[1] + result[2] + result[3];
}

static double
run(int nthreads, long iters)
{
    auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> threads;
    for (int t = 0; t < nthreads; ++t)
        threads.emplace_back(work, iters);
    for (auto& t : threads)
        t.join();
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0)
        .count();
}

int
main(int argc, char* argv[])
{
    long iters = argc > 1 ? atol(argv[1]) : 2000000;
    std::vector<int> counts;
    for (int i = 2; i < argc; ++i)
        counts.push_back(atoi(argv[i]));
    if (counts.empty())
        counts = { 1, 2, 4, 8, 16, 32 };
    printf("threads  best(s)  ratio-vs-first  (best of 3, %ld iters/thread)\n",
           iters);
    double first = 0;
    for (int n : counts) {
        double best = 1e30;
        for (int trial = 0; trial < 3; ++trial)
            best = std::min(best, run(n, iters));
        if (first == 0)
            first = best;
        printf("%5d   %7.3f   %6.2f\n", n, best, best / first);
        fflush(stdout);
    }
}
