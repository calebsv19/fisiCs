typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W3Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned q8(long double v) {
    if (v < 0.0L) {
        v = (-v * 1.375L) + 0.25L;
    }
    return (unsigned)(v * 256.0L + 0.5L);
}

struct Axis3W3Wide axis3_w3_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned seed_tag,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W3Wide out;
    static const int lane_map[5] = {2, 0, 1, 2, 1};
    int i;

    out.lo = seed_lo;
    out.mid = seed_mid;
    out.hi = seed_hi;
    out.tag = seed_tag ^ 0x9e3779b9u;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[i % 5];

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
            out.tag ^= (unsigned)(q8(ldv) + (unsigned)(i * 13 + 7));
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.lo += (long double)(iv & 31) * 0.015625L;
            out.mid += (long double)((iv >> 1) & 31) * 0.0078125L;
            out.hi -= (long double)((iv >> 2) & 15) * 0.00390625L;
            out.tag = rotl32(out.tag ^ (unsigned)(iv + i * 17), 5u);
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.lo += (long double)(uv & 63u) * 0.0078125L;
            out.mid -= (long double)((uv >> 2) & 31u) * 0.00390625L;
            out.hi += (long double)((uv >> 3) & 31u) * 0.001953125L;
            out.tag ^= rotl32(uv + (unsigned)(i * 11 + 3), (unsigned)((i & 7) + 3));
        }
    }
    va_end(ap);

    return out;
}

unsigned axis3_w3_wide_digest(struct Axis3W3Wide v, unsigned salt) {
    unsigned acc = salt ^ 0x85ebca6bu;
    acc ^= q8(v.lo) + 0x45d9f3bu;
    acc = rotl32(acc, 5u);
    acc ^= q8(v.mid) + 0x27d4eb2du;
    acc = rotl32(acc, 7u);
    acc ^= q8(v.hi) + 0xc2b2ae35u;
    acc = rotl32(acc, 11u);
    acc ^= v.tag + 0x9e3779b9u;
    acc = rotl32(acc, 13u);
    return acc;
}
