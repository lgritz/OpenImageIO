// Single-threaded: cost of looped vs unrolled isb.
#include <chrono>
#include <cstdio>
__attribute__((noinline)) void looped(int n) {
    for (int i = 0; i < n; ++i) __asm__ __volatile__("isb" ::: "memory");
}
__attribute__((noinline)) void unrolled128() {
#pragma clang loop unroll(full)
    for (int i = 0; i < 128; ++i) __asm__ __volatile__("isb" ::: "memory");
}
int main() {
    volatile int n = 128;
    const int R = 100000;
    for (int k = 0; k < 2; ++k) {
        auto t0 = std::chrono::steady_clock::now();
        for (int r = 0; r < R; ++r) looped(n);
        auto t1 = std::chrono::steady_clock::now();
        for (int r = 0; r < R; ++r) unrolled128();
        auto t2 = std::chrono::steady_clock::now();
        printf("looped %.2f ns/isb   unrolled %.2f ns/isb\n",
               std::chrono::duration<double, std::nano>(t1 - t0).count() / (R * 128.0),
               std::chrono::duration<double, std::nano>(t2 - t1).count() / (R * 128.0));
    }
}
