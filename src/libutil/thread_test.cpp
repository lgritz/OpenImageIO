// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO


#include <algorithm>
#include <functional>
#include <iostream>
#include <map>

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

#include <boost/container/flat_map.hpp>

OIIO_NAMESPACE_BEGIN


// tsp_base1 has a single unordered_map_concurrent from threadid to shared_ptr
// of the payload. This has reader/writer locks and locks on every access.
class OIIO_UTIL_API tsp_base1 {
public:
    struct internals;
protected:
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

//////////////////////////////////////////////////////////////////////

// tsp_base2 uses a thread_local map (using std::unordered_map) from
// tsp to unowned raw pointer, for speedy get(). The set() still needs
// to lock the tsp_map.
class OIIO_UTIL_API tsp_base2 {
public:
    struct internals;
protected:
    typedef void deleter_t(void* p);

    tsp_base2();
    ~tsp_base2();
    tsp_base2(const tsp_base2&) = delete;
    tsp_base2(tsp_base2&&) = delete;

    void* get() const;
    void set(void* ptr, deleter_t deleter);
    std::unique_ptr<internals> m_internals;
};

// Each tsp has a map of thread -> pointer values. That map is destroyed when
// the tsp is destroyed, and should delete all the pointers it owns.

typedef std::unordered_map<void*, void*> one_thread_map_t;
static thread_local one_thread_map_t tl_one_thread_map;

struct tsp_base2::internals {
    // static thread_local std::shared_ptr<void> tsp_map;
    // For each thread, map the tsp object to a non-owning payload pointer

    OIIO::unordered_map_concurrent<std::thread::id, std::shared_ptr<void>> tsp_map;

    // Get the pointer for the current thread
    void* get() {
        return tl_one_thread_map[this];
    }

    // Set the pointer for the current thread. Here we need to do some
    // double accounting.
    void set(void* ptr, deleter_t deleter) {
        tsp_map.insert_or_replace(std::this_thread::get_id(),
                                  std::shared_ptr<void>(ptr, deleter));
        tl_one_thread_map[this] = ptr;
    }
};

tsp_base2::tsp_base2()
    : m_internals(new internals)
{
}

tsp_base2::~tsp_base2()
{
    // implicitly destroys m_internals
}

void*
tsp_base2::get() const
{
    return m_internals->get();
}

void
tsp_base2::set(void* ptr, deleter_t deleter)
{
    m_internals->set(ptr, deleter);
}


//////////////////////////////////////////////////////////////////////

// tsp_base3 is tsp_base2, but using a std::map.
class OIIO_UTIL_API tsp_base3 {
public:
    struct internals;
protected:
    typedef void deleter_t(void* p);

    tsp_base3();
    ~tsp_base3();
    tsp_base3(const tsp_base3&) = delete;
    tsp_base3(tsp_base3&&) = delete;

    void* get() const;
    void set(void* ptr, deleter_t deleter);
    std::unique_ptr<internals> m_internals;
};

// Each tsp has a map of thread -> pointer values. That map is destroyed when
// the tsp is destroyed, and should delete all the pointers it owns.

typedef std::map<void*, void*> one_thread_map_t3;
static thread_local one_thread_map_t3 tl_one_thread_map3;

struct tsp_base3::internals {
    // static thread_local std::shared_ptr<void> tsp_map;
    // For each thread, map the tsp object to a non-owning payload pointer

    OIIO::unordered_map_concurrent<std::thread::id, std::shared_ptr<void>> tsp_map;

    // Get the pointer for the current thread
    void* get() {
        return tl_one_thread_map3[this];
    }

    // Set the pointer for the current thread. Here we need to do some
    // double accounting.
    void set(void* ptr, deleter_t deleter)
    {
        tsp_map.insert_or_replace(std::this_thread::get_id(),
                                  std::shared_ptr<void>(ptr, deleter));
        tl_one_thread_map3[this] = ptr;
    }
};

tsp_base3::tsp_base3()
    : m_internals(new internals)
{
}

tsp_base3::~tsp_base3()
{
    // implicitly destroys m_internals
}

void*
tsp_base3::get() const
{
    return m_internals->get();
}

void
tsp_base3::set(void* ptr, deleter_t deleter)
{
    m_internals->set(ptr, deleter);
}


//////////////////////////////////////////////////////////////////////

// tsp_base4 is tsp_base2, but using a flat_map.
class OIIO_UTIL_API tsp_base4 {
public:
    struct internals;
protected:
    typedef void deleter_t(void* p);

    tsp_base4();
    ~tsp_base4();
    tsp_base4(const tsp_base4&) = delete;
    tsp_base4(tsp_base4&&) = delete;

    void* get() const;
    void set(void* ptr, deleter_t deleter);
    std::unique_ptr<internals> m_internals;
};

// Each tsp has a map of thread -> pointer values. That map is destroyed when
// the tsp is destroyed, and should delete all the pointers it owns.

typedef boost::container::flat_map<void*, void*> one_thread_map_t4;
static thread_local one_thread_map_t4 tl_one_thread_map4;

struct tsp_base4::internals {
    // static thread_local std::shared_ptr<void> tsp_map;
    // For each thread, map the tsp object to a non-owning payload pointer

    OIIO::unordered_map_concurrent<std::thread::id, std::shared_ptr<void>> tsp_map;

    // Get the pointer for the current thread
    void* get() {
        return tl_one_thread_map3[this];
    }

    // Set the pointer for the current thread. Here we need to do some
    // double accounting.
    void set(void* ptr, deleter_t deleter)
    {
        tsp_map.insert_or_replace(std::this_thread::get_id(),
                                  std::shared_ptr<void>(ptr, deleter));
        tl_one_thread_map3[this] = ptr;
    }
};

tsp_base4::tsp_base4()
    : m_internals(new internals)
{
}

tsp_base4::~tsp_base4()
{
    // implicitly destroys m_internals
}

void*
tsp_base4::get() const
{
    return m_internals->get();
}

void
tsp_base4::set(void* ptr, deleter_t deleter)
{
    m_internals->set(ptr, deleter);
}


//////////////////////////////////////////////////////////////////////

// tsp_base5 is tsp_base3 (std::map for the global thread_local map),
// but uses a locked flat_map for the per-tsp thread->payload map.
class OIIO_UTIL_API tsp_base5 {
public:
    struct internals;
protected:
    typedef void deleter_t(void* p);

    tsp_base5();
    ~tsp_base5();
    tsp_base5(const tsp_base5&) = delete;
    tsp_base5(tsp_base5&&) = delete;

    void* get() const;
    void set(void* ptr, deleter_t deleter);
    std::unique_ptr<internals> m_internals;
};

// Each tsp has a map of thread -> pointer values. That map is destroyed when
// the tsp is destroyed, and should delete all the pointers it owns.

typedef std::map<void*, void*> one_thread_map_t5;
static thread_local one_thread_map_t5 tl_one_thread_map5;

struct tsp_base5::internals {
    // static thread_local std::shared_ptr<void> tsp_map;
    // For each thread, map the tsp object to a non-owning payload pointer

    // boost::container::flat_map<std::thread::id, std::shared_ptr<void>> tsp_map;
    std::unordered_map<std::thread::id, std::shared_ptr<void>> tsp_map;
    spin_mutex tsp_mutex;

    // Get the pointer for the current thread
    void* get() {
        return tl_one_thread_map5[this];
    }

    // Set the pointer for the current thread. Here we need to do some
    // double accounting.
    void set(void* ptr, deleter_t deleter)
    {
        {
            std::lock_guard<spin_mutex> lock(tsp_mutex);
            tsp_map[std::this_thread::get_id()].reset(ptr, deleter);
        }
        tl_one_thread_map5[this] = ptr;
    }
};

tsp_base5::tsp_base5()
    : m_internals(new internals)
{
}

tsp_base5::~tsp_base5()
{
    // implicitly destroys m_internals
}

void*
tsp_base5::get() const
{
    return m_internals->get();
}

void
tsp_base5::set(void* ptr, deleter_t deleter)
{
    m_internals->set(ptr, deleter);
}





OIIO_NAMESPACE_END


int global_nontsp_value = 0;


void
test_tsp()
{
    print("\nTesting tsp\n");
    // Make a bunch
    constexpr int ntsps = 1000;
    OIIO::thread_specific_ptr<int, tsp_base1> otsp1_arr[ntsps];
    OIIO::thread_specific_ptr<int, tsp_base2> otsp2_arr[ntsps];
    OIIO::thread_specific_ptr<int, tsp_base3> otsp3_arr[ntsps];
    OIIO::thread_specific_ptr<int, tsp_base4> otsp4_arr[ntsps];
    OIIO::thread_specific_ptr<int, tsp_base5> otsp5_arr[ntsps];
    boost::thread_specific_ptr<int> btsp_arr[ntsps];
    for (int i = 0; i < ntsps; ++i) {
        otsp1_arr[i].reset(new int(i));
        otsp2_arr[i].reset(new int(i));
        otsp3_arr[i].reset(new int(i));
        otsp4_arr[i].reset(new int(i));
        otsp5_arr[i].reset(new int(i));
        btsp_arr[i].reset(new int(i));
    }
    OIIO::thread_specific_ptr<int, tsp_base1> otsp1;
    OIIO::thread_specific_ptr<int, tsp_base2> otsp2;
    OIIO::thread_specific_ptr<int, tsp_base3> otsp3;
    OIIO::thread_specific_ptr<int, tsp_base4> otsp4;
    OIIO::thread_specific_ptr<int, tsp_base5> otsp5;
    boost::thread_specific_ptr<int> btsp;
    std::shared_ptr<int> sptr;
    otsp1.reset(new int(0));
    otsp2.reset(new int(0));
    otsp3.reset(new int(0));
    otsp4.reset(new int(0));
    otsp5.reset(new int(0));
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
    bench("create oiio::thread_specific_ptr type 1", [&]() {
        OIIO::thread_specific_ptr<int, tsp_base1> ptr;
        ptr.reset(new int(0));
        clobber_all_memory();
    });
    bench("create oiio::thread_specific_ptr type 2", [&]() {
        OIIO::thread_specific_ptr<int, tsp_base2> ptr;
        ptr.reset(new int(0));
        clobber_all_memory();
    });
    bench("create oiio::thread_specific_ptr type 3", [&]() {
        OIIO::thread_specific_ptr<int, tsp_base3> ptr;
        ptr.reset(new int(0));
        clobber_all_memory();
    });
    bench("create oiio::thread_specific_ptr type 4", [&]() {
        OIIO::thread_specific_ptr<int, tsp_base4> ptr;
        ptr.reset(new int(0));
        clobber_all_memory();
    });
    bench("create oiio::thread_specific_ptr type 5", [&]() {
        OIIO::thread_specific_ptr<int, tsp_base5> ptr;
        ptr.reset(new int(0));
        clobber_all_memory();
    });
    bench("create thread_local shared_ptr", [&]() {
        tlptr.reset(new int(0));
        clobber_all_memory();
    });

    bench("get shared_ptr value", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*sptr);
        iacc += i;
    });
    bench("get boost::thread_specific_ptr value once", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*btsp.get());
        iacc += i;
    });
    bench("get oiio::thread_specific_ptr1 value once", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*otsp1.get());
        iacc += i;
    });
    bench("get oiio::thread_specific_ptr2 value once", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*otsp2.get());
        iacc += i;
    });
    bench("get oiio::thread_specific_ptr3 value once", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*otsp3.get());
        iacc += i;
    });
    bench("get oiio::thread_specific_ptr4 value once", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*otsp4.get());
        iacc += i;
    });
    bench("get oiio::thread_specific_ptr5 value once", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*otsp5.get());
        iacc += i;
    });

    bench("set and get shared_ptr value", [&]() {
        int i = DoNotOptimize(*sptr);
        clobber_all_memory();
        *sptr = i + 1;
    });
    bench("set and get boost::thread_specific_ptr value", [&]() {
        int i = DoNotOptimize(*btsp.get());
        clobber_all_memory();
        *btsp = i + 1;
    });
    bench("set and get oiio::thread_specific_ptr1 value", [&]() {
        int i = DoNotOptimize(*otsp1.get());
        clobber_all_memory();
        *otsp1 = i + 1;
    });
    bench("set and get oiio::thread_specific_ptr2 value", [&]() {
        int i = DoNotOptimize(*otsp2.get());
        clobber_all_memory();
        *otsp2 = i + 1;
    });
    bench("set and get oiio::thread_specific_ptr3 value", [&]() {
        int i = DoNotOptimize(*otsp3.get());
        clobber_all_memory();
        *otsp3 = i + 1;
    });
    bench("set and get oiio::thread_specific_ptr4 value", [&]() {
        int i = DoNotOptimize(*otsp4.get());
        clobber_all_memory();
        *otsp3 = i + 1;
    });
    bench("set and get oiio::thread_specific_ptr5 value", [&]() {
        int i = DoNotOptimize(*otsp5.get());
        clobber_all_memory();
        *otsp3 = i + 1;
    });

    tlptr.reset(new int(42));
    bench("get thread_local ptr value", [&]() {
        clobber_all_memory();
        tlptr.reset(new int(0));
    });

    bench("reset shared_ptr to new ptr", [&]() {
        clobber_all_memory();
        sptr.reset(new int(0));
    });
    bench("reset boost::thread_specific_ptr to new ptr", [&]() {
        clobber_all_memory();
        btsp.reset(new int(0));
    });
    bench("reset oiio::thread_specific_ptr1 to new ptr", [&]() {
        clobber_all_memory();
        otsp1.reset(new int(0));
    });
    bench("reset oiio::thread_specific_ptr2 to new ptr", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*otsp2.get());
        iacc += i;
    });
    bench("reset oiio::thread_specific_ptr3 to new ptr", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*otsp3.get());
        iacc += i;
    });
    bench("reset oiio::thread_specific_ptr4 to new ptr", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*otsp4.get());
        iacc += i;
    });
    bench("reset oiio::thread_specific_ptr5 to new ptr", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*otsp5.get());
        iacc += i;
    });

    print("sizeof(tsp_base1) = {}\n", sizeof(OIIO::tsp_base1::internals));
    print("sizeof(tsp_base2) = {}\n", sizeof(OIIO::tsp_base2::internals));
    print("sizeof(tsp_base3) = {}\n", sizeof(OIIO::tsp_base3::internals));
    print("sizeof(tsp_base4) = {}\n", sizeof(OIIO::tsp_base4::internals));
    print("sizeof(tsp_base5) = {}\n", sizeof(OIIO::tsp_base5::internals));
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
