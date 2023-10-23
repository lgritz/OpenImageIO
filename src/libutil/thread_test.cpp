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



int global_nontsp_value = 0;


void
test_tsp()
{
    print("\nTesting tsp\n");
    // Make a bunch
    constexpr int ntsps = 10000;
    OIIO::thread_specific_ptr<int> otsp1_arr[ntsps];
    boost::thread_specific_ptr<int> btsp_arr[ntsps];
    for (int i = 0; i < ntsps; ++i) {
        otsp1_arr[i].reset(new int(i));
        btsp_arr[i].reset(new int(i));
    }
    OIIO::thread_specific_ptr<int> otsp1;
    boost::thread_specific_ptr<int> btsp;
    std::shared_ptr<int> sptr;
    otsp1.reset(new int(0));
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
    bench("get oiio::thread_specific_ptr value once", [&]() {
        clobber_all_memory();
        int i = DoNotOptimize(*otsp1.get());
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
    bench("set and get oiio::thread_specific_ptr value", [&]() {
        int i = DoNotOptimize(*otsp1.get());
        clobber_all_memory();
        *otsp1 = i + 1;
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
    bench("reset oiio::thread_specific_ptr to new ptr", [&]() {
        clobber_all_memory();
        otsp1.reset(new int(0));
    });
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
