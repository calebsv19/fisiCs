#include <stdio.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct SeedLane {
    long double ld;
    long long i64;
    int tag;
};

static unsigned fnv1a_u32(unsigned h, unsigned v) {
    h ^= (v & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 8) & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 16) & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 24) & 0xFFu);
    h *= 16777619u;
    return h;
}

static unsigned hash_long_double(long double v) {
    const unsigned char *p = (const unsigned char *)&v;
    unsigned h = 2166136261u;
    int i;
    for (i = 0; i < (int)sizeof(v); ++i) {
        h = fnv1a_u32(h, (unsigned)p[i]);
    }
    return h;
}

static long long fold_mix(
    int a0,
    int a1,
    int a2,
    int a3,
    int a4,
    int a5,
    int a6,
    int a7,
    struct SeedLane seed,
    int lanes,
    ...
) {
    va_list ap;
    long double acc_ld = seed.ld + (long double)(a0 + a4 - a6 + seed.tag);
    long long acc_i = seed.i64 + (long long)(a1 - a2 + a3 + a5 + a7);
    int i;

    va_start(ap, lanes);
    for (i = 0; i < lanes; ++i) {
        int kind = va_arg(ap, int);
        if (kind == 0) {
            long double ldv = va_arg(ap, long double);
            acc_ld += ldv * (long double)(i + 1);
        } else if (kind == 1) {
            long long iv = va_arg(ap, long long);
            acc_i ^= (iv + (long long)(i * 19 + 7));
        } else {
            int iv = va_arg(ap, int);
            acc_i += (long long)iv * (long long)(i + 3);
            acc_ld += (long double)iv / 16.0L;
        }
    }
    va_end(ap);

    return acc_i + (long long)(acc_ld * 128.0L);
}

int main(void) {
    struct SeedLane s0 = {1.25L, 9001LL, 3};
    struct SeedLane s1 = {-0.75L, 1234567LL, -2};

    long long x0 = fold_mix(
        1, 2, 3, 4, 5, 6, 7, 8, s0, 7,
        0, 1.5L,
        1, 9000000000LL,
        2, -3,
        0, -0.25L,
        1, 77LL,
        2, 11,
        0, 2.0L
    );
    long long x1 = fold_mix(
        1, 2, 3, 4, 5, 6, 7, 8, s0, 7,
        0, 1.5L,
        1, 9000000000LL,
        2, -3,
        0, -0.25L,
        1, 77LL,
        2, 11,
        0, 2.0L
    );
    long long y = fold_mix(
        9, -2, 13, 0, 4, -5, 2, 6, s1, 6,
        2, 5,
        0, -1.75L,
        1, 1234LL,
        2, -8,
        0, 0.5L,
        1, 999999LL
    );

    if (x0 != x1) {
        printf("abi-repeatability-fail %lld %lld\n", x0, x1);
        return 31;
    }

    printf("%lld %lld %u\n", x0, y, hash_long_double(s0.ld) ^ hash_long_double(s1.ld));
    return 0;
}
