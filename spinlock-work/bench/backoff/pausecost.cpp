// Cost of one OIIO::pause() step and of an uncontended yield, so backoff
// lengths can be compared across machines.
#include <OpenImageIO/thread.h>
#include <chrono>
#include <cstdio>
#include <thread>
int main()
{
    for (int r = 0; r < 3; ++r) {
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < 100000; ++i)
            OIIO::pause(100);
        double ns = std::chrono::duration<double, std::nano>(
                        std::chrono::steady_clock::now() - t0)
                        .count();
        auto t1 = std::chrono::steady_clock::now();
        for (int i = 0; i < 100000; ++i)
            std::this_thread::yield();
        double yns = std::chrono::duration<double, std::nano>(
                         std::chrono::steady_clock::now() - t1)
                         .count();
        std::printf("pause %.2f ns/step, uncontended yield %.0f ns\n",
                    ns / 1e7, yns / 1e5);
    }
}
