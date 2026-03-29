#include <stdint.h>
#include <stdio.h>

static unsigned long long fold_mask_rebase(unsigned char* buf, int n) {
    static const int idxs[] = {3, 7, 11, 19, 27};
    const uintptr_t mask = (uintptr_t)0xF;
    /* Guardrail: use base-relative deltas, not raw pointer low bits.
       This keeps outputs deterministic for differential comparison. */
    uintptr_t base = (uintptr_t)(void*)&buf[0];
    unsigned long long acc = 0;

    (void)n;
    for (int i = 0; i < 5; ++i) {
        uintptr_t p = (uintptr_t)(void*)&buf[idxs[i]];
        uintptr_t delta = p - base;
        uintptr_t hi = delta & ~mask;
        uintptr_t lo = delta & mask;
        uintptr_t r = hi | lo;
        unsigned char* q = (unsigned char*)(void*)(base + r);
        acc += (unsigned long long)(*q) * (unsigned long long)(i + 3);
        acc += (unsigned long long)lo;
        if ((base + r) == p) {
            acc += 17ULL;
        }
    }
    return acc;
}

int main(void) {
    unsigned char buf[32];
    for (int i = 0; i < 32; ++i) {
        buf[i] = (unsigned char)(i * 9 + 5);
    }

    unsigned long long out = fold_mask_rebase(buf, 32);
    uintptr_t p1 = (uintptr_t)(void*)&buf[1] - (uintptr_t)(void*)&buf[0];
    uintptr_t p2 = (uintptr_t)(void*)&buf[29] - (uintptr_t)(void*)&buf[0];
    unsigned long long nibble_pair = (unsigned long long)(((p1 & 0xF) << 8) | (p2 & 0xF));
    printf("%llu %llu %llu\n", out, (unsigned long long)(p2 - p1), nibble_pair);
    return 0;
}
