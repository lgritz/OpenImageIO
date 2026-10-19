// Backoff tuning bench for an OIIO-style TTAS spin_mutex.
// NO atomic wait/notify anywhere (kernel-panics macOS under contention).
// Usage: bench <variant> <threads> <workload> <total_ops> <trials>
//   variant: oiio (real OIIO spin_mutex from the include path),
//            P:S[:Y]  TTAS, pauses 1..P doubling, S more pauses of P, then
//                     yield; Y=1 means after yielding go back to spinning
//                     a full P:S round before the next yield.
//   workload: hammer | light
#include <OpenImageIO/thread.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <vector>

static int P = 128, S = 64, Y = 0;

struct TTAS {
    std::atomic<bool> m { false };
    void lock() noexcept
    {
        int count = 1, spins = S;
        while (!try_lock()) {
            do {
                if (count <= P) {
                    OIIO::pause(count);
                    count *= 2;
                } else if (spins > 0) {
                    OIIO::pause(P);
                    --spins;
                } else {
                    std::this_thread::yield();
                    if (Y) {
                        count = 1;
                        spins = S;
                    }
                }
            } while (m.load(std::memory_order_relaxed));
        }
    }
    bool try_lock() noexcept
    {
        return !m.exchange(true, std::memory_order_acquire);
    }
    void unlock() noexcept { m.store(false, std::memory_order_release); }
};

alignas(128) static TTAS ttas;
alignas(128) static OIIO::spin_mutex oiio_mutex;
alignas(128) static long long accum = 0;
alignas(128) static float faccum    = 0;

static std::atomic<long long> g_waits { 0 }, g_wait_ns { 0 }, g_maxsum { 0 };

template<class M> static void worker(M& mtx, long long n, bool light)
{
    float last = 0.0f;
    long long waits = 0, wait_ns = 0, maxw = 0;
    for (long long i = 0; i < n; ++i) {
        if (light) {
            // ~work outside the lock
            for (int k = 0; k < 8; ++k)
                last = std::fmod(std::sin(last + 0.5f), 1.0f);
        }
        if (!mtx.try_lock()) {
            auto t0 = std::chrono::steady_clock::now();
            mtx.lock();
            long long w = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::steady_clock::now() - t0)
                              .count();
            ++waits;
            wait_ns += w;
            maxw = std::max(maxw, w);
        }
        accum += 1;
        if (light)
            faccum = std::fmod(faccum + last, 1.0f);
        mtx.unlock();
    }
    g_waits += waits;
    g_wait_ns += wait_ns;
    g_maxsum += maxw;
}

template<class M>
static double run(M& mtx, int nt, long long total, bool light)
{
    accum         = 0;
    g_waits = 0; g_wait_ns = 0; g_maxsum = 0;
    long long per = total / nt;
    std::vector<std::thread> th;
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < nt; ++i)
        th.emplace_back([&] { worker(mtx, per, light); });
    for (auto& t : th)
        t.join();
    double s = std::chrono::duration<double>(std::chrono::steady_clock::now()
                                             - t0)
                   .count();
    if (accum != per * nt) {
        std::fprintf(stderr, "BAD accum %lld\n", accum);
        std::exit(2);
    }
    return s;
}

int main(int argc, char** argv)
{
    alarm(300);
    if (argc < 6) {
        std::fprintf(stderr, "usage\n");
        return 1;
    }
    bool use_oiio = !std::strcmp(argv[1], "oiio");
    if (!use_oiio)
        std::sscanf(argv[1], "%d:%d:%d", &P, &S, &Y);
    int nt         = std::atoi(argv[2]);
    bool light     = !std::strcmp(argv[3], "light");
    long long tot  = std::atoll(argv[4]);
    int trials     = std::atoi(argv[5]);
    double best = 1e30, worst = 0;
    for (int t = 0; t < trials; ++t) {
        double s = use_oiio ? run(oiio_mutex, nt, tot, light)
                            : run(ttas, nt, tot, light);
        best  = std::min(best, s);
        worst = std::max(worst, s);
    }
    std::printf("%s %d %s %.1f ns/op range %.2f | contended %.0f%%, mean wait %.2f us, avg per-thread max %.0f us\n",
                argv[1], nt, argv[3], best * 1e9 / tot, (worst - best) * 1e9 / tot,
                100.0 * g_waits / tot, g_waits ? g_wait_ns / 1e3 / g_waits : 0.0,
                g_maxsum / 1e3 / nt);
    std::fflush(stdout);
    fsync(1);
    return 0;
}
