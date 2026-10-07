#include <OpenImageIO/thread.h>
#include <cstdio>
#include <thread>
#include <vector>
static OIIO::spin_mutex m;
static long counter = 0;  // plain, protected by m
int main() {
    std::vector<std::thread> t;
    for (int i = 0; i < 4; ++i)
        t.emplace_back([] { for (int j = 0; j < 100000; ++j) { OIIO::spin_lock l(m); ++counter; } });
    for (auto& x : t) x.join();
    printf("counter=%ld (expect 400000)\n", counter);
    return counter == 400000 ? 0 : 1;
}
