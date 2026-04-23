typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W7Hfa {
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

struct Axis3W7Hfa axis3_w7_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W7Hfa out;
    static const int lane_map[7] = {1, 3, 0, 2, 1, 0, 3};
    int i;

    out.a = seed + (float)(epoch & 15u) * 0.03125f;
    out.b = seed * 0.875f + 0.5f;
    out.c = seed * 0.6875f + 0.75f;
    out.d = seed * 0.53125f + 1.0f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[(i + (int)(frontier & 3u) + (int)(replay & 1u)) % 7];
        float s = (float)(i + 1) * 0.015625f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.3125f + s);
            } else if (lane == 1) {
                out.b += fv * (0.1875f + s);
            } else if (lane == 2) {
                out.c += fv * (0.15625f + s);
            } else {
                out.d += fv * (0.125f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane) & 63) * 0.0078125f;
            out.b -= (float)((iv >> 1) & 31) * 0.00390625f;
            out.c += (float)((iv >> 2) & 31) * 0.001953125f;
            out.d += (float)((iv ^ i) & 31) * 0.0009765625f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)(uv & 63u) * 0.00390625f;
            out.b += (float)((uv >> 2) & 31u) * 0.001953125f;
            out.c -= (float)((uv >> 3) & 31u) * 0.0009765625f;
            out.d += (float)((uv ^ replay) & 31u) * 0.00048828125f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(epoch, frontier & 31u) & 31u) * 0.001953125f;
    out.c += (float)((replay >> 1) & 15u) * 0.0009765625f;
    return out;
}

unsigned axis3_w7_hfa_digest(struct Axis3W7Hfa v, unsigned salt) {
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
