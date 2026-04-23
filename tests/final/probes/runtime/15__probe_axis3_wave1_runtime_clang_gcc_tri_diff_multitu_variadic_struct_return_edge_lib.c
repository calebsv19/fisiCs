typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W1Wide {
    long double lo;
    long double hi;
    int lane;
};

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned q8(long double v) {
    if (v < 0.0L) {
        v = (-v * 1.375L) + 0.5L;
    }
    return (unsigned)(v * 256.0L + 0.5L);
}

struct Axis3W1Wide axis3_w1_wide_collect(
    long double seed_lo,
    long double seed_hi,
    int lane,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W1Wide out;
    int i;

    out.lo = seed_lo + (long double)lane * 0.125L;
    out.hi = seed_hi + (long double)(lane + 1) * 0.0625L;
    out.lane = lane;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        if (kind == 0) {
            long double ldv = va_arg(ap, long double);
            out.lo += ldv * (0.25L + (long double)(i + 1) * 0.0625L);
            out.hi += ldv * (0.125L + (long double)(i & 1) * 0.03125L);
        } else {
            int iv = va_arg(ap, int);
            out.lo += (long double)(iv + i) * 0.03125L;
            out.hi += (long double)(iv - i) * 0.015625L;
            out.lane ^= (iv + i * 3);
        }
    }
    va_end(ap);

    return out;
}

unsigned axis3_w1_wide_digest(struct Axis3W1Wide v, unsigned salt) {
    unsigned acc = salt ^ 0x9e3779b9u;
    acc ^= q8(v.lo) + 0x45d9f3bu;
    acc = rotl32(acc, 5u);
    acc ^= q8(v.hi) + 0x27d4eb2du;
    acc = rotl32(acc, 11u);
    acc ^= (unsigned)(v.lane * 97 + 13);
    acc = rotl32(acc, 7u);
    return acc;
}
