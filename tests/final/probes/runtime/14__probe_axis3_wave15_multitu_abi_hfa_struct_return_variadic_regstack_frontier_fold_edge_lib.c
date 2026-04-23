typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W15Hfa {
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

struct Axis3W15Hfa axis3_w15_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W15Hfa out;
    static const int lane_map[23] = {3, 1, 0, 2, 3, 2, 1, 0, 3, 2, 0, 1, 2, 3, 1, 0, 2, 1, 3, 0, 2, 3, 1};
    int i;

    out.a = seed + (float)(epoch & 63u) * 0.0107421875f;
    out.b = seed * 0.796875f + 0.6875f;
    out.c = seed * 0.5546875f + 1.03125f;
    out.d = seed * 0.4375f + 1.171875f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (i >> 1) + (i >> 2) + (i >> 3) + (int)(frontier & 7u) + (int)(replay & 3u) + (int)(epoch & 3u)) % 23
        ];
        float s = (float)(i + 1) * 0.005859375f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.2109375f + s);
            } else if (lane == 1) {
                out.b += fv * (0.1875f + s);
            } else if (lane == 2) {
                out.c += fv * (0.171875f + s);
            } else {
                out.d += fv * (0.15234375f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane + i) & 63) * 0.005859375f;
            out.b -= (float)((iv >> 1) & 31) * 0.00244140625f;
            out.c += (float)((iv >> 2) & 31) * 0.002685546875f;
            out.d += (float)((iv ^ (i + lane + 1)) & 31) * 0.001953125f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)((uv ^ epoch ^ replay) & 63u) * 0.004150390625f;
            out.b += (float)((uv >> 2) & 31u) * 0.00146484375f;
            out.c -= (float)((uv >> 3) & 31u) * 0.001220703125f;
            out.d += (float)((uv ^ replay ^ frontier) & 31u) * 0.00079345703125f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(epoch ^ frontier, replay & 31u) & 63u) * 0.00146484375f;
    out.b += (float)(rotl32(frontier ^ replay, epoch & 31u) & 31u) * 0.001220703125f;
    out.c += (float)(rotl32(replay ^ epoch, frontier & 31u) & 31u) * 0.0009765625f;
    out.d -= (float)((epoch >> 1) & 31u) * 0.0010986328125f;
    return out;
}

unsigned axis3_w15_hfa_digest(struct Axis3W15Hfa v, unsigned salt) {
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
