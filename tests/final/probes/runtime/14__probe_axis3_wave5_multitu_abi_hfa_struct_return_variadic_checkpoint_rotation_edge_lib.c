typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W5Hfa {
    float p;
    float q;
    float r;
    float s;
};

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned q9(float v) {
    if (v < 0.0f) {
        v = (-v * 1.5f) + 0.5f;
    }
    return (unsigned)(v * 512.0f + 0.5f);
}

struct Axis3W5Hfa axis3_w5_hfa_collect(float seed, unsigned checkpoint, unsigned rotate, int count, ...) {
    va_list ap;
    struct Axis3W5Hfa out;
    static const int lane_map[4] = {3, 1, 0, 2};
    int i;

    out.p = seed + (float)(checkpoint & 15u) * 0.03125f;
    out.q = seed * 0.875f + 0.5f;
    out.r = seed * 0.6875f + 0.75f;
    out.s = seed * 0.5f + 1.0f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[(i + (int)(rotate & 3u)) & 3];
        float m = (float)(i + 1) * 0.015625f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.p += fv * (0.3125f + m);
            } else if (lane == 1) {
                out.q += fv * (0.1875f + m);
            } else if (lane == 2) {
                out.r += fv * (0.15625f + m);
            } else {
                out.s += fv * (0.125f + m);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.p += (float)((iv + lane) & 31) * 0.0078125f;
            out.q -= (float)((iv >> 1) & 15) * 0.00390625f;
            out.r += (float)((iv >> 2) & 15) * 0.001953125f;
            out.s += (float)((iv ^ i) & 7) * 0.0009765625f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.p += (float)(uv & 31u) * 0.00390625f;
            out.q += (float)((uv >> 2) & 31u) * 0.001953125f;
            out.r -= (float)((uv >> 3) & 15u) * 0.0009765625f;
            out.s += (float)((uv ^ checkpoint) & 31u) * 0.00048828125f;
        }
    }
    va_end(ap);

    out.p += (float)(rotl32(checkpoint, rotate & 31u) & 31u) * 0.001953125f;
    out.r += (float)((checkpoint >> 3) & 15u) * 0.0009765625f;
    return out;
}

unsigned axis3_w5_hfa_digest(struct Axis3W5Hfa v, unsigned salt) {
    unsigned acc = salt ^ 0xc2b2ae35u;
    acc ^= q9(v.p) + 0x45d9f3bu;
    acc = rotl32(acc, 5u);
    acc ^= q9(v.q) + 0x27d4eb2du;
    acc = rotl32(acc, 7u);
    acc ^= q9(v.r) + 0x85ebca6bu;
    acc = rotl32(acc, 11u);
    acc ^= q9(v.s) + 0x9e3779b9u;
    acc = rotl32(acc, 13u);
    return acc;
}
