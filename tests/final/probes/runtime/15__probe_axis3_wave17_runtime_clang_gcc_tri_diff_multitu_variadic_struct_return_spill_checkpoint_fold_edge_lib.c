typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W17Wide {
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

struct Axis3W17Wide axis3_w17_wide_collect(
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
    struct Axis3W17Wide out;
    static const int lane_map[23] = {2, 1, 0, 2, 1, 0, 1, 2, 0, 1, 0, 2, 1, 2, 0, 2, 1, 0, 2, 1, 0, 1, 2};
    int i;

    out.lo = seed_lo + (long double)(epoch & 63u) * 0.0234375L;
    out.mid = seed_mid + (long double)(frontier & 63u) * 0.01220703125L;
    out.hi = seed_hi - (long double)(replay & 63u) * 0.001220703125L;
    out.tag = (epoch ^ frontier ^ replay) + rotl32(0x85ebca6bu ^ replay, (frontier + replay + epoch) & 31u);

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (i >> 1) + (i >> 2) + (i >> 3) + (int)(frontier & 7u) + (int)(replay & 3u) + (int)(epoch & 3u)) % 23
        ];
        long double s = 0.00537109375L * (long double)(i + 1);

        if (kind == 0) {
            long double ldv = va_arg(ap, long double);
            if (lane == 0) {
                out.lo += ldv * (0.21875L + s);
            } else if (lane == 1) {
                out.mid += ldv * (0.1953125L + s);
            } else {
                out.hi += ldv * (0.18359375L + s);
            }
            out.tag ^= q10(ldv) + (unsigned)(i * 53 + 19);
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.lo += (long double)((iv + lane + i) & 63) * 0.005615234375L;
            out.mid -= (long double)((iv >> 1) & 31) * 0.0023193359375L;
            out.hi += (long double)((iv >> 2) & 31) * 0.00274658203125L;
            out.tag = rotl32(out.tag ^ (unsigned)(iv + i * 59), 9u);
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.lo += (long double)((uv ^ epoch ^ replay) & 63u) * 0.00439453125L;
            out.mid += (long double)((uv >> 2) & 31u) * 0.00140380859375L;
            out.hi -= (long double)((uv >> 3) & 31u) * 0.00128173828125L;
            out.tag ^= rotl32(
                uv + epoch + frontier + replay + (unsigned)(i * 47 + 13),
                (unsigned)((i & 7) + 6)
            );
        }
    }
    va_end(ap);

    out.lo += (long double)(rotl32(epoch ^ frontier, replay & 31u) & 63u) * 0.00732421875L;
    out.mid += (long double)(rotl32(frontier ^ replay, epoch & 31u) & 63u) * 0.00732421875L;
    out.hi += (long double)(rotl32(replay ^ epoch, frontier & 31u) & 31u) * 0.005859375L;
    return out;
}

unsigned axis3_w17_wide_digest(struct Axis3W17Wide v, unsigned salt) {
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
