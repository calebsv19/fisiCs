typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W10Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned q10(long double v) {
    if (v < 0.0L) {
        v = (-v * 1.5L) + 0.5L;
    }
    return (unsigned)(v * 1024.0L + 0.5L);
}

struct Axis3W10Wide axis3_w10_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W10Wide out;
    static const int lane_map[13] = {0, 2, 1, 0, 1, 2, 0, 2, 1, 0, 2, 1, 0};
    int i;

    out.lo = seed_lo + (long double)(epoch & 7u) * 0.09375L;
    out.mid = seed_mid + (long double)(frontier & 15u) * 0.0234375L;
    out.hi = seed_hi - (long double)(replay & 7u) * 0.00390625L;
    out.tag = epoch ^ rotl32(0x9e3779b9u, (frontier ^ replay) & 31u);

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (int)(frontier & 7u) + (int)(replay & 3u) + (int)(epoch & 3u)) % 13
        ];
        long double s = 0.0087890625L * (long double)(i + 1);

        if (kind == 0) {
            long double ldv = va_arg(ap, long double);
            if (lane == 0) {
                out.lo += ldv * (0.265625L + s);
            } else if (lane == 1) {
                out.mid += ldv * (0.2265625L + s);
            } else {
                out.hi += ldv * (0.15625L + s);
            }
            out.tag ^= q10(ldv) + (unsigned)(i * 27 + 17);
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.lo += (long double)((iv + lane + i) & 63) * 0.0068359375L;
            out.mid -= (long double)((iv >> 1) & 31) * 0.00341796875L;
            out.hi += (long double)((iv >> 2) & 31) * 0.00244140625L;
            out.tag = rotl32(out.tag ^ (unsigned)(iv + i * 31), 11u);
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.lo += (long double)((uv ^ epoch ^ frontier) & 63u) * 0.00439453125L;
            out.mid += (long double)((uv >> 2) & 31u) * 0.001708984375L;
            out.hi -= (long double)((uv >> 3) & 31u) * 0.001220703125L;
            out.tag ^= rotl32(
                uv + epoch + frontier + replay + (unsigned)(i * 23 + 11),
                (unsigned)((i & 7) + 5)
            );
        }
    }
    va_end(ap);

    out.lo += (long double)(rotl32(epoch, frontier & 31u) & 31u) * 0.01171875L;
    out.mid += (long double)(rotl32(frontier, replay & 31u) & 15u) * 0.009765625L;
    out.hi += (long double)((replay >> 2) & 15u) * 0.0087890625L;
    return out;
}

unsigned axis3_w10_wide_digest(struct Axis3W10Wide v, unsigned salt) {
    unsigned acc = salt ^ 0xc2b2ae35u;
    acc ^= q10(v.lo) + 0x45d9f3bu;
    acc = rotl32(acc, 5u);
    acc ^= q10(v.mid) + 0x27d4eb2du;
    acc = rotl32(acc, 7u);
    acc ^= q10(v.hi) + 0x85ebca6bu;
    acc = rotl32(acc, 11u);
    acc ^= v.tag + 0x9e3779b9u;
    acc = rotl32(acc, 13u);
    return acc;
}
