// Standalone spin lock benchmark (attempt 3, crash-safe).
//   c++ -std=c++17 -O2 -pthread spinbench.cpp -o spinbench17
//   c++ -std=c++20 -O2 -pthread spinbench.cpp -o spinbench20
// Usage:
//   spinbench list                         -- list variant names
//   spinbench unc <variant>                -- uncontended lock/unlock ns
//   spinbench con <variant> <threads> <tiny|medium|coloc>|medium> <total_ops> <trials>
//   spinbench pausecost                    -- ns per pause instruction
//
// SAFETY: this file must never use atomic wait/notify (macOS ulock
// syscalls under contention panicked the kernel twice). No std::mutex
// contention either (pthread mutexes may block via ulock).

#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <sys/resource.h>
#include <unistd.h>
#if defined(__x86_64__) || defined(__i386__)
#    include <immintrin.h>
#endif

// ---- pause primitives -------------------------------------------------

struct PauseEmpty {  // what OIIO's pause() compiles to on aarch64 today
    static inline void p(int n)
    {
        for (int i = 0; i < n; ++i)
            ;
    }
};
struct PauseIsb {  // isb on aarch64, pause on x86
    static inline void p(int n)
    {
#ifdef ISB_NOUNROLL
#    pragma nounroll
#endif
        for (int i = 0; i < n; ++i) {
#if defined(__aarch64__)
            __asm__ __volatile__("isb" ::: "memory");
#elif defined(__x86_64__) || defined(__i386__)
            _mm_pause();
#endif
        }
    }
};
struct PauseYieldInsn {  // aarch64 'yield' hint
    static inline void p(int n)
    {
        for (int i = 0; i < n; ++i) {
#if defined(__aarch64__)
            __asm__ __volatile__("yield" ::: "memory");
#elif defined(__x86_64__) || defined(__i386__)
            _mm_pause();
#endif
        }
    }
};

// ---- backoff ----------------------------------------------------------
// Pause count doubles while < Cap; then pause(Cap) for YieldAfter more
// rounds; then sched-yield every round. Cap=16,YieldAfter=1 is exactly
// OIIO's atomic_backoff (1,2,4,8,16 then yield). YieldAfter=INT_MAX
// never yields (article V4 when Cap=64).
template<class P, int Cap, int YieldAfter> struct Backoff {
    static constexpr bool yields = (YieldAfter != INT_MAX);
    int count = 1, rounds = 0;
    inline void operator()()
    {
        if (count < Cap) {
            P::p(count);
            count *= 2;
        } else if (rounds < YieldAfter) {
            P::p(Cap);
            ++rounds;
        } else {
            std::this_thread::yield();
        }
    }
};

// ---- lock variants ----------------------------------------------------

// Current OIIO spin_mutex (DCLP on).
template<class B> struct FlagVolatile {
    static constexpr bool yields = B::yields;
    std::atomic_flag f           = ATOMIC_FLAG_INIT;
    bool try_lock() { return !f.test_and_set(std::memory_order_acquire); }
    void lock()
    {
        B b;
        while (__builtin_expect(!try_lock(), 0)) {
            do {
                b();
            } while (*(volatile bool*)&f);
        }
    }
    void unlock() { f.clear(std::memory_order_release); }
};

// Current OIIO spin_mutex with DCLP off.
template<class B> struct FlagNoTTAS {
    static constexpr bool yields = B::yields;
    std::atomic_flag f           = ATOMIC_FLAG_INIT;
    void lock()
    {
        B b;
        while (f.test_and_set(std::memory_order_acquire))
            b();
    }
    void unlock() { f.clear(std::memory_order_release); }
};

// atomic<bool> TTAS (article structure, backoff policy parameterized).
template<class B> struct BoolTTAS {
    static constexpr bool yields = B::yields;
    std::atomic<bool> f { false };
    void lock()
    {
        B b;
        while (f.exchange(true, std::memory_order_acquire)) {
            do {
                b();
            } while (f.load(std::memory_order_relaxed));
        }
    }
    void unlock() { f.store(false, std::memory_order_release); }
};

// atomic<bool> TTAS with unlikely hint (proposed OIIO shape).
template<class B> struct BoolTTASx {
    static constexpr bool yields = B::yields;
    std::atomic<bool> f { false };
    bool try_lock() { return !f.exchange(true, std::memory_order_acquire); }
    void lock()
    {
        B b;
        while (__builtin_expect(!try_lock(), 0)) {
            do {
                b();
            } while (f.load(std::memory_order_relaxed));
        }
    }
    void unlock() { f.store(false, std::memory_order_release); }
};

// atomic<bool> TTAS where the first attempt is a plain load (avoids an
// RMW when already locked).
template<class B> struct BoolTTAS2 {
    static constexpr bool yields = B::yields;
    std::atomic<bool> f { false };
    void lock()
    {
        B b;
        for (;;) {
            if (!f.exchange(true, std::memory_order_acquire))
                return;
            while (f.load(std::memory_order_relaxed))
                b();
        }
    }
    void unlock() { f.store(false, std::memory_order_release); }
};

#if defined(__cpp_lib_atomic_flag_test)
template<class B> struct FlagTest {
    static constexpr bool yields = B::yields;
    std::atomic_flag f;  // C++20: default-constructed clear
    void lock()
    {
        B b;
        while (f.test_and_set(std::memory_order_acquire)) {
            do {
                b();
            } while (f.test(std::memory_order_relaxed));
        }
    }
    void unlock() { f.clear(std::memory_order_release); }
};
#endif

#ifdef WITH_OIIO
#    include <OpenImageIO/thread.h>
struct OiioBO {
    static constexpr bool yields = true;
    OIIO::atomic_backoff b { 128, 256 };
    void operator()() { b(); }
};
struct OiioSpin {
    static constexpr bool yields = true;
    OIIO::spin_mutex m;
    void lock() { m.lock(); }
    void unlock() { m.unlock(); }
};
#endif

struct StdMutex {  // uncontended timing ONLY
    static constexpr bool yields = true;
    std::mutex m;
    void lock() { m.lock(); }
    void unlock() { m.unlock(); }
};

// ---- harness ----------------------------------------------------------

static double cpu_seconds()
{
    rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_utime.tv_sec + ru.ru_stime.tv_sec
           + 1e-6 * (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec);
}

struct alignas(128) Counter {
    long long v = 0;
    float f     = 0;
};

static volatile float g_sink;

struct CounterU {  // unaligned: shares the lock cache line
    long long v = 0;
    float f     = 0;
};

template<class L, bool Medium, class C>
static void worker(L& lock, C& c, long iters, std::atomic<int>& go)
{
    while (!go.load(std::memory_order_acquire))
        std::this_thread::yield();
    float last = 0.0f;
    for (long i = 0; i < iters; ++i) {
        if (Medium)
            last = std::fmod(std::sin(last + float(i)), 1.0f);
        lock.lock();
        c.v += 1;
        if (Medium)
            c.f = std::fmod(std::sin(c.f + last), 1.0f);
        lock.unlock();
    }
    g_sink = last;
}

struct Result {
    double wall_ns, cpu_ns;
};

template<class L, bool Medium, bool Coloc = false>
static Result run_once(int nthreads, long total)
{
    struct alignas(128) Holder {
        L l;
        CounterU cu;
        char pad[128];
    };
    using C     = std::conditional_t<Coloc, CounterU, Counter>;
    auto holder = std::make_unique<Holder>();
    L& lock     = holder->l;
    auto csep   = std::make_unique<Counter>();
    C* c;
    if constexpr (Coloc)
        c = &holder->cu;
    else
        c = csep.get();
    long iters  = total / nthreads;
    std::atomic<int> go { getenv("NOBARRIER") ? 1 : 0 };
    std::vector<std::thread> th;
    double c0 = cpu_seconds();
    auto t0   = std::chrono::steady_clock::now();
    for (int t = 0; t < nthreads; ++t)
        th.emplace_back(worker<L, Medium, C>, std::ref(lock), std::ref(*c), iters,
                        std::ref(go));
    if (!go.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        c0 = cpu_seconds();
        t0 = std::chrono::steady_clock::now();
    }
    go.store(1, std::memory_order_release);
    for (auto& x : th)
        x.join();
    auto t1   = std::chrono::steady_clock::now();
    double c1 = cpu_seconds();
    long ops  = iters * nthreads;
    if (c->v != ops) {
        fprintf(stderr, "CORRECTNESS FAIL %lld != %ld\n", c->v, ops);
        exit(1);
    }
    return { std::chrono::duration<double, std::nano>(t1 - t0).count() / ops,
             (c1 - c0) * 1e9 / ops };
}

template<class L> static int uncontended()
{
    L l;
    const long N = 50000000;
    auto t0      = std::chrono::steady_clock::now();
    for (long i = 0; i < N; ++i) {
        l.lock();
        __asm__ __volatile__("" ::: "memory");
        l.unlock();
    }
    auto t1 = std::chrono::steady_clock::now();
    printf("%.2f\n",
           std::chrono::duration<double, std::nano>(t1 - t0).count() / N);
    return 0;
}

template<class L>
static int contended(int nt, int shape, long total, int trials)
{
    const int hw = int(std::thread::hardware_concurrency());
#ifdef __APPLE__
    if (std::is_same<L, StdMutex>::value) {
        fprintf(stderr, "refusing contended std::mutex on macOS (ulock risk)\n");
        return 2;
    }
#endif
    if (!L::yields && nt > hw) {
        fprintf(stderr, "refusing more threads than hw threads for a no-yield variant\n");
        return 2;
    }
    std::vector<Result> r;
    for (int t = 0; t < trials; ++t)
        r.push_back(shape == 1   ? run_once<L, true>(nt, total)
                    : shape == 2 ? run_once<L, false, true>(nt, total)
                                 : run_once<L, false>(nt, total));
    std::sort(r.begin(), r.end(),
              [](auto& a, auto& b) { return a.wall_ns < b.wall_ns; });
    Result m = r[r.size() / 2];
    printf("%.1f/%.1f\n", m.wall_ns, m.cpu_ns);
    return 0;
}

template<class P> static double pausecost()
{
    const int N = 10000000;
    auto t0     = std::chrono::steady_clock::now();
    P::p(N);
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::nano>(t1 - t0).count() / N;
}

// Variants
using CurBO    = Backoff<PauseEmpty, 16, 1>;  // today on arm64
using CurIsb   = Backoff<PauseIsb, 16, 1>;
using CurYld   = Backoff<PauseYieldInsn, 16, 1>;
using Art      = Backoff<PauseIsb, 64, INT_MAX>;  // article V4
using ArtYld   = Backoff<PauseYieldInsn, 64, INT_MAX>;
using I32_16   = Backoff<PauseIsb, 32, 16>;
using I64_4    = Backoff<PauseIsb, 64, 4>;
using I64_16   = Backoff<PauseIsb, 64, 16>;
using I64_64   = Backoff<PauseIsb, 64, 64>;
using I128_16  = Backoff<PauseIsb, 128, 16>;
using I16_16   = Backoff<PauseIsb, 16, 16>;
using Emp64_16 = Backoff<PauseEmpty, 64, 16>;
using I64_256  = Backoff<PauseIsb, 64, 256>;
using I64_1k   = Backoff<PauseIsb, 64, 1024>;
using I128_64  = Backoff<PauseIsb, 128, 64>;
using I128_256 = Backoff<PauseIsb, 128, 256>;
using I32_256  = Backoff<PauseIsb, 32, 256>;

#define BASE_VARIANTS(F)                           \
    F(FlagVolatile<CurBO>, "A_cur")                \
    F(FlagNoTTAS<CurBO>, "A2_cur_nodclp")          \
    F(FlagVolatile<CurIsb>, "B_flag_isb_16y1")     \
    F(FlagVolatile<CurYld>, "B2_flag_yld_16y1")    \
    F(BoolTTAS<CurIsb>, "C_bool_isb_16y1")         \
    F(BoolTTAS<Art>, "D_article")                  \
    F(BoolTTAS<ArtYld>, "D2_article_yldinsn")      \
    F(BoolTTAS<I16_16>, "E_bool_isb_16y16")        \
    F(BoolTTAS<I32_16>, "E_bool_isb_32y16")        \
    F(BoolTTAS<I64_4>, "E_bool_isb_64y4")          \
    F(BoolTTAS<I64_16>, "E_bool_isb_64y16")        \
    F(BoolTTAS<I64_64>, "E_bool_isb_64y64")        \
    F(BoolTTAS<I128_16>, "E_bool_isb_128y16")      \
    F(BoolTTAS<Emp64_16>, "E_bool_empty_64y16")    \
    F(BoolTTAS2<I64_16>, "E2_bool2_isb_64y16")     \
    F(FlagVolatile<I64_16>, "B3_flag_isb_64y16")   \
    F(BoolTTASx<I64_64>, "X_bool_isb_64y64")      \
    F(BoolTTASx<I64_256>, "X_bool_isb_64y256")    \
    F(BoolTTASx<I64_1k>, "X_bool_isb_64y1k")      \
    F(BoolTTASx<I128_16>, "X_bool_isb_128y16")    \
    F(BoolTTASx<I128_64>, "X_bool_isb_128y64")    \
    F(BoolTTASx<I128_256>, "X_bool_isb_128y256")  \
    F(BoolTTASx<I32_256>, "X_bool_isb_32y256")    \
    F(FlagVolatile<I128_64>, "B4_flag_isb_128y64") \
    F(StdMutex, "H_stdmutex")

#if defined(__cpp_lib_atomic_flag_test)
#    define CXX20_VARIANTS(F)                       \
        F(FlagTest<CurIsb>, "F_flagtest_isb_16y1") \
        F(FlagTest<I64_16>, "F_flagtest_isb_64y16")
#else
#    define CXX20_VARIANTS(F)
#endif

#ifdef WITH_OIIO
#    define OIIO_VARIANTS(F) F(OiioSpin, "O_oiio_spin_mutex") F(BoolTTASx<OiioBO>, "O2_ttasx_oiiobackoff")
#else
#    define OIIO_VARIANTS(F)
#endif
#define ALL_VARIANTS(F) BASE_VARIANTS(F) CXX20_VARIANTS(F) OIIO_VARIANTS(F)

int main(int argc, char** argv)
{
    alarm(300);  // hard self-kill if anything hangs
    setvbuf(stdout, nullptr, _IOLBF, 0);
    if (argc < 2) {
        fprintf(stderr, "see usage in source\n");
        return 2;
    }
    std::string mode = argv[1];
    if (mode == "list") {
#define LIST(T, N) printf("%s\n", N);
        ALL_VARIANTS(LIST)
        return 0;
    }
    if (mode == "pausecost") {
        printf("empty %.3f ns\nisb %.3f ns\nyield-insn %.3f ns\n",
               pausecost<PauseEmpty>(), pausecost<PauseIsb>(),
               pausecost<PauseYieldInsn>());
        return 0;
    }
    if (argc < 3)
        return 2;
    std::string v = argv[2];
    if (mode == "unc") {
#define UNC(T, N) \
    if (v == N)   \
        return uncontended<T>();
        ALL_VARIANTS(UNC)
    } else if (mode == "con" && argc >= 7) {
        int nt      = atoi(argv[3]);
        int shape    = !strcmp(argv[4], "medium") ? 1
                      : !strcmp(argv[4], "coloc") ? 2
                                                  : 0;
        long total  = atol(argv[5]);
        int trials  = atoi(argv[6]);
        if (nt < 1 || nt > 4 * int(std::thread::hardware_concurrency()) || total < nt || trials < 1
            || total > 200000000) {
            fprintf(stderr, "bad args\n");
            return 2;
        }
#define CON(T, N) \
    if (v == N)   \
        return contended<T>(nt, shape, total, trials);
        ALL_VARIANTS(CON)
    }
    fprintf(stderr, "unknown variant/mode\n");
    return 2;
}
