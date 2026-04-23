typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W4Wide {
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
        v = (-v * 1.25L) + 0.5L;
    }
    return (unsigned)(v * 512.0L + 0.5L);
}

struct Axis3W4Wide axis3_w4_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned seed_tag,
    unsigned seed_mode,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W4Wide out;
    static const int lane_map[6] = {1, 2, 0, 2, 1, 0};
    int i;

    out.lo = seed_lo + (long double)(seed_mode & 3u) * 0.125L;
    out.mid = seed_mid;
    out.hi = seed_hi;
    out.tag = seed_tag ^ rotl32(0x9e3779b9u, seed_mode & 31u);

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[(i + (int)(seed_mode & 3u)) % 6];
        long double s = 0.03125L * (long double)(i + 1);

        if (kind == 0) {
            long double ldv = va_arg(ap, long double);
            if (lane == 0) {
                out.lo += ldv * (0.28125L + s);
            } else if (lane == 1) {
                out.mid += ldv * (0.21875L + s);
            } else {
                out.hi += ldv * (0.15625L + s);
            }
            out.tag ^= q9(ldv) + (unsigned)(i * 19 + 5);
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.lo += (long double)(iv & 63) * 0.0078125L;
            out.mid -= (long double)((iv >> 1) & 31) * 0.00390625L;
            out.hi += (long double)((iv >> 2) & 31) * 0.001953125L;
            out.tag = rotl32(out.tag ^ (unsigned)(iv + i * 23), 7u);
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.lo += (long double)(uv & 63u) * 0.00390625L;
            out.mid += (long double)((uv >> 2) & 31u) * 0.001953125L;
            out.hi -= (long double)((uv >> 3) & 15u) * 0.0009765625L;
            out.tag ^= rotl32(uv + (unsigned)(i * 13 + 9), (unsigned)((i & 7) + 5));
        }
    }
    va_end(ap);

    out.lo += (long double)(seed_mode & 15u) * 0.015625L;
    out.hi += (long double)((seed_tag >> 3) & 15u) * 0.0078125L;
    return out;
}

unsigned axis3_w4_wide_digest(struct Axis3W4Wide v, unsigned salt) {
    unsigned acc = salt ^ 0xc2b2ae35u;
    acc ^= q9(v.lo) + 0x45d9f3bu;
    acc = rotl32(acc, 5u);
    acc ^= q9(v.mid) + 0x27d4eb2du;
    acc = rotl32(acc, 7u);
    acc ^= q9(v.hi) + 0x85ebca6bu;
    acc = rotl32(acc, 11u);
    acc ^= v.tag + 0x9e3779b9u;
    acc = rotl32(acc, 13u);
    return acc;
}
