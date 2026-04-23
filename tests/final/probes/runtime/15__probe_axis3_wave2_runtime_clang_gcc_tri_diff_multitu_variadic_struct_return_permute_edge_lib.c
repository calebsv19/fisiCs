typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W2Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned q9(long double v) {
    if (v < 0.0L) {
        v = (-v * 1.5L) + 0.25L;
    }
    return (unsigned)(v * 512.0L + 0.5L);
}

struct Axis3W2Wide axis3_w2_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned seed_tag,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W2Wide out;
    static const int p[3] = {1, 2, 0};
    int i;

    out.lo = seed_lo;
    out.mid = seed_mid;
    out.hi = seed_hi;
    out.tag = seed_tag ^ 0x9e3779b9u;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = p[i % 3];
        if (kind == 0) {
            long double ldv = va_arg(ap, long double);
            long double s = 0.0625L * (long double)(i + 1);
            if (lane == 0) {
                out.lo += ldv * (0.25L + s);
            } else if (lane == 1) {
                out.mid += ldv * (0.1875L + s);
            } else {
                out.hi += ldv * (0.15625L + s);
            }
            out.tag ^= (unsigned)(q9(ldv) + (unsigned)(i * 13 + 5));
        } else {
            unsigned uv = (unsigned)va_arg(ap, int);
            out.lo += (long double)(uv & 31u) * 0.015625L;
            out.mid += (long double)((uv >> 2) & 31u) * 0.0078125L;
            out.hi -= (long double)((uv >> 3) & 15u) * 0.00390625L;
            out.tag = rotl32(out.tag ^ (uv + (unsigned)(i * 17 + 9)), 5u);
        }
    }
    va_end(ap);

    return out;
}

unsigned axis3_w2_wide_digest(struct Axis3W2Wide v, unsigned salt) {
    unsigned acc = salt ^ 0x85ebca6bu;
    acc ^= q9(v.lo) + 0x45d9f3bu;
    acc = rotl32(acc, 5u);
    acc ^= q9(v.mid) + 0x27d4eb2du;
    acc = rotl32(acc, 7u);
    acc ^= q9(v.hi) + 0xc2b2ae35u;
    acc = rotl32(acc, 11u);
    acc ^= v.tag + 0x9e3779b9u;
    acc = rotl32(acc, 13u);
    return acc;
}
