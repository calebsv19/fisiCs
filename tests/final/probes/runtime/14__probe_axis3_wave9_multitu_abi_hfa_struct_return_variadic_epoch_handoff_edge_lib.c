typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W9Hfa {
    float a;
    float b;
    float c;
    float d;
};

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned q10(float v) {
    if (v < 0.0f) {
        v = (-v * 1.5f) + 0.5f;
    }
    return (unsigned)(v * 1024.0f + 0.5f);
}

struct Axis3W9Hfa axis3_w9_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W9Hfa out;
    static const int lane_map[11] = {3, 1, 0, 2, 3, 0, 1, 2, 0, 3, 1};
    int i;

    out.a = seed + (float)(epoch & 15u) * 0.02734375f;
    out.b = seed * 0.84375f + 0.625f;
    out.c = seed * 0.65625f + 0.8125f;
    out.d = seed * 0.5f + 1.0625f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (int)(frontier & 7u) + (int)(replay & 3u) + (int)(epoch & 3u)) % 11
        ];
        float s = (float)(i + 1) * 0.009765625f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.265625f + s);
            } else if (lane == 1) {
                out.b += fv * (0.2109375f + s);
            } else if (lane == 2) {
                out.c += fv * (0.1796875f + s);
            } else {
                out.d += fv * (0.1484375f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane + i) & 63) * 0.00732421875f;
            out.b -= (float)((iv >> 1) & 31) * 0.003173828125f;
            out.c += (float)((iv >> 2) & 31) * 0.002197265625f;
            out.d += (float)((iv ^ (i + lane + 1)) & 31) * 0.00146484375f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)((uv ^ epoch ^ frontier) & 63u) * 0.004150390625f;
            out.b += (float)((uv >> 2) & 31u) * 0.001953125f;
            out.c -= (float)((uv >> 3) & 31u) * 0.0009765625f;
            out.d += (float)((uv ^ replay ^ frontier) & 31u) * 0.0008544921875f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(epoch, frontier & 31u) & 31u) * 0.002197265625f;
    out.b += (float)(rotl32(frontier, replay & 31u) & 15u) * 0.001708984375f;
    out.c += (float)((replay >> 1) & 15u) * 0.0009765625f;
    out.d -= (float)((epoch >> 3) & 15u) * 0.001220703125f;
    return out;
}

unsigned axis3_w9_hfa_digest(struct Axis3W9Hfa v, unsigned salt) {
    unsigned acc = salt ^ 0x9e3779b9u;
    acc ^= q10(v.a) + 0x45d9f3bu;
    acc = rotl32(acc, 5u);
    acc ^= q10(v.b) + 0x27d4eb2du;
    acc = rotl32(acc, 7u);
    acc ^= q10(v.c) + 0x85ebca6bu;
    acc = rotl32(acc, 11u);
    acc ^= q10(v.d) + 0xc2b2ae35u;
    acc = rotl32(acc, 13u);
    return acc;
}
