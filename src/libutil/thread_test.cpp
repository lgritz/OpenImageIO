// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO


#include <algorithm>
#include <functional>
#include <iostream>

#include <OpenImageIO/argparse.h>
#include <OpenImageIO/benchmark.h>
#include <OpenImageIO/sysutil.h>
#include <OpenImageIO/thread.h>
#include <OpenImageIO/timer.h>
#include <OpenImageIO/unittest.h>
#include <OpenImageIO/ustring.h>

#include <boost/thread/tss.hpp>

using namespace OIIO;

static int iterations     = 100000;
static int numthreads     = 16;
static int ntrials        = 1;
static bool verbose       = false;
static bool wedge         = false;
static int threadcounts[] = { 1,  2,  4,  8,  12,  16,   20,
                              24, 28, 32, 64, 128, 1024, 1 << 30 };



static void
getargs(int argc, char* argv[])
{
    // clang-format off
    ArgParse ap;
    ap.intro("thread_test\n" OIIO_INTRO_STRING)
      .usage("thread_test [options]");

    ap.arg("-v", &verbose)
      .help("Verbose mode");
    ap.arg("--threads %d", &numthreads)
      .help(Strutil::sprintf("Number of threads (default: %d)", numthreads));
    ap.arg("--iters %d", &iterations)
      .help(Strutil::sprintf("Number of iterations (default: %d)", iterations));
    ap.arg("--trials %d", &ntrials)
      .help("Number of trials");
    ap.arg("--wedge", &wedge)
      .help("Do a wedge test");
    // clang-format on

    ap.parse(argc, (const char**)argv);
}



void
do_nothing(int /*thread_id*/)
{
}



void
time_thread_group()
{
    std::cout << "\nTiming how long it takes to start/end thread_group:\n";
    std::cout << "threads\ttime (best of " << ntrials << ")\n";
    std::cout << "-------\t----------\n";
    for (int i = 0; threadcounts[i] <= numthreads; ++i) {
        int nt  = wedge ? threadcounts[i] : numthreads;
        int its = iterations / nt;

        // make a lambda function that spawns a bunch of threads, calls a
        // trivial function, then waits for them to finish and tears down
        // the group.
        auto func = [=]() {
            thread_group g;
            for (int i = 0; i < nt; ++i)
                g.create_thread(do_nothing, i);
            g.join_all();
        };

        double range;
        double t = time_trial(func, ntrials, its, &range);

        Strutil::printf("%2d\t%5.1f   launch %8.1f threads/sec\n", nt, t,
                        (nt * its) / t);
        if (!wedge)
            break;  // don't loop if we're not wedging
    }
}



void
time_thread_pool()
{
    std::cout << "\nTiming how long it takes to launch from thread_pool:\n";
    std::cout << "threads\ttime (best of " << ntrials << ")\n";
    std::cout << "-------\t----------\n";
    thread_pool* pool(default_thread_pool());
    for (int i = 0; threadcounts[i] <= numthreads; ++i) {
        int nt = wedge ? threadcounts[i] : numthreads;
        pool->resize(nt);
        int its = iterations / nt;

        // make a lambda function that spawns a bunch of threads, calls a
        // trivial function, then waits for them to finish and tears down
        // the group.
        auto func = [=]() {
            task_set taskset(pool);
            for (int i = 0; i < nt; ++i) {
                taskset.push(pool->push(do_nothing));
            }
            taskset.wait();
        };

        double range;
        double t = time_trial(func, ntrials, its, &range);

        std::cout << Strutil::sprintf("%2d\t%5.1f   launch %8.1f threads/sec\n",
                                      nt, t, (nt * its) / t);
        if (!wedge)
            break;  // don't loop if we're not wedging
    }

    Benchmarker bench;
    bench("std::this_thread::get_id()",
          [=]() { DoNotOptimize(std::this_thread::get_id()); });
    std::thread::id threadid = std::this_thread::get_id();
    bench("register/deregister pool worker", [=]() {
        pool->register_worker(threadid);
        pool->deregister_worker(threadid);
    });
}



#include <OpenImageIO/unordered_map_concurrent.h>

OIIO_NAMESPACE_BEGIN


// tsp_base1 has a single unordered_map_concurrent from threadid to shared_ptr
// of the payload. This has reader/writer locks and locks on every access.
class OIIO_UTIL_API tsp_base1 {
protected:
    struct internals;
    typedef void deleter_t(void* p);

    tsp_base1();
    ~tsp_base1();
    tsp_base1(const tsp_base1&) = delete;
    tsp_base1(tsp_base1&&) = delete;

    void* get() const;
    void set(void* ptr, deleter_t deleter);
    std::unique_ptr<internals> m_internals;
};

// Each tsp has a map of thread -> pointer values. That map is destroyed when
// the tsp is destroyed, and should delete all the pointers it owns.

struct tsp_base1::internals {
    // std::unordered_map<std::thread::id, std::shared_ptr<void>> tsp_map;
    OIIO::unordered_map_concurrent<std::thread::id, std::shared_ptr<void>> tsp_map;
    // std::mutex tsp_mutex;

    // Get the pointer for the current thread
    void* get() {
        // std::lock_guard<std::mutex> lock(tsp_mutex);
        auto found = tsp_map.find(std::this_thread::get_id());
        return (found != tsp_map.end()) ? found->second.get() : nullptr;
    }
    // Set the pointer for the current thread
    void set(void* ptr, deleter_t deleter) {
        // std::lock_guard<std::mutex> lock(tsp_mutex);
        // tsp_map[std::this_thread::get_id()] = std::shared_ptr<void>(ptr, deleter);
        tsp_map.insert_or_replace(std::this_thread::get_id(),
                                  std::shared_ptr<void>(ptr, deleter));
    }
};

tsp_base1::tsp_base1()
    : m_internals(new internals)
{
}

tsp_base1::~tsp_base1()
{
    // implicitly destroys m_internals
}

void*
tsp_base1::get() const
{
    return m_internals->get();
}

void
tsp_base1::set(void* ptr, deleter_t deleter)
{
    m_internals->set(ptr, deleter);
}



/// thread_specific_ptr<T> is a per-thread `T*`. It behaves like thread_local,
/// except the tsp may be an object, unlike the C++ thread_local storage
/// class, which can only apply to static variables.
///
/// This is an attempt to replace boost::thread_specific_ptr. It's a lot
/// simpler and probably not as robust, but it's enough for our purposes.
///
///
/// Accessing the tsp with `*`, `->` or `get()` will retrieve the specific
/// pointer for the calling thread. If the tsp is not set for the calling
/// thread, it will return nullptr.
///
/// The pointers are owned by the tsp itself. So when the tsp is destroyed,
/// all the pointers stored in the tsp (for all threads) will be deleted. When
/// a thread is destroyed, the pointers are not yet freed.
///
template<typename T, typename Base = tsp_base1>
class thread_specific_ptr : public Base
{
public:
    /// Construct a tsp<T>.
    thread_specific_ptr() { }

    /// You can't copy a tsp
    thread_specific_ptr(const thread_specific_ptr&) = delete;
    /// You can't move a tsp
    thread_specific_ptr(thread_specific_ptr&&) = delete;

    /// Destroy a tsp.
    ~thread_specific_ptr() { }

    /// Retrieve the pointer for the calling thread, which will be nullptr if
    /// reset() was never called to supply a pointer for the calling thread.
    T* get() const { return reinterpret_cast<T*>(Base::get()); }
    /// Dereference the pointer of the calling thread.
    T* operator->() const { return this->get(); }
    /// Dereference the pointer of the calling thread.
    T& operator*() const { return *this->get(); }

    /// Implicit cast to bool checks if the calling thread's pointer is null.
    operator bool () const { return Base::get() != nullptr; }

    /// Replace the calling thread's pointer with a new value and call delete
    /// on any old value.
    void reset(T* obj = nullptr) {
        Base::set(obj, deleter);
    }

private:
    static void deleter(void* p) { delete reinterpret_cast<T*>(p); }
};







OIIO_NAMESPACE_END


int global_nontsp_value = 0;


void
test_tsp()
{
    print("\nTesting tsp\n");
    OIIO::thread_specific_ptr<int> otsp;
    boost::thread_specific_ptr<int> btsp;
    std::shared_ptr<int> sptr;
    otsp.reset(new int(0));
    btsp.reset(new int(0));
    sptr.reset(new int(0));
    static thread_local std::shared_ptr<int> tlptr;
    int iacc = 0;

    Benchmarker bench;
    bench("create shared_ptr", [&]() {
        std::shared_ptr<int> ptr;
        ptr.reset(new int(0));
        clobber_all_memory();
    });
    bench("create boost::thread_specific_ptr", [&]() {
        boost::thread_specific_ptr<int> ptr;
        ptr.reset(new int(0));
        clobber_all_memory();
    });
    bench("create oiio::thread_specific_ptr", [&]() {
        OIIO::thread_specific_ptr<int> ptr;
        ptr.reset(new int(0));
        clobber_all_memory();
    });
    bench("create thread_local shared_ptr", [&]() {
        tlptr.reset(new int(0));
        clobber_all_memory();
    });

    bench("set and get shared_ptr", [&]() {
        int i = DoNotOptimize(*sptr);
        clobber_all_memory();
        *sptr = i + 1;
    });
    bench("set and get boost::thread_specific_ptr", [&]() {
        int i = DoNotOptimize(*btsp.get());
        clobber_all_memory();
        *btsp = i + 1;
    });
    bench("set and get oiio::thread_specific_ptr", [&]() {
        int i = DoNotOptimize(*otsp.get());
        clobber_all_memory();
        *otsp = i + 1;
    });

    bench("get shared_ptr", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*sptr);
        iacc += i;
    });
    bench("get boost::thread_specific_ptr", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*btsp.get());
        iacc += i;
    });
    bench("get oiio::thread_specific_ptr", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*otsp.get());
        iacc += i;
    });

    tlptr.reset(new int(42));
    bench("get thread_local ptr", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*tlptr);
        iacc += i;
    });


    // otsp.reset(nullptr);
    // btsp.reset(nullptr);
}



int
main(int argc, char** argv)
{
#if !defined(NDEBUG) || defined(OIIO_CI) || defined(OIIO_CODE_COVERAGE)
    // For the sake of test time, reduce the default iterations for DEBUG,
    // CI, and code coverage builds. Explicit use of --iters or --trials
    // will override this, since it comes before the getargs() call.
    iterations /= 10;
    ntrials = 1;
#endif

    getargs(argc, argv);

    std::cout << "hw threads = " << Sysutil::hardware_concurrency() << "\n";

    time_thread_group();
    time_thread_pool();

    test_tsp();

    return unit_test_failures;
}
