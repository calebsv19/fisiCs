typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W3Hfa {
    float a;
    float b;
    float c;
    float d;
};

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned q11(float v) {
    if (v < 0.0f) {
        v = (-v * 1.375f) + 0.25f;
    }
    return (unsigned)(v * 2048.0f + 0.5f);
}

struct Axis3W3Hfa axis3_w3_hfa_collect(float seed, int count, ...) {
    va_list ap;
    struct Axis3W3Hfa out;
    static const int lane_map[5] = {1, 3, 0, 2, 1};
    int i;

    out.a = seed + 0.25f;
    out.b = seed * 0.8125f + 0.5f;
    out.c = seed * 0.625f + 0.75f;
    out.d = seed * 0.4375f + 1.0f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[i % 5];
        float s = (float)(i + 1) * 0.03125f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.25f + s);
            } else if (lane == 1) {
                out.b += fv * (0.1875f + s);
            } else if (lane == 2) {
                out.c += fv * (0.15625f + s);
            } else {
                out.d += fv * (0.125f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane) & 15) * 0.0625f;
            out.b += (float)(iv & 7) * 0.03125f;
            out.c -= (float)((iv >> 1) & 7) * 0.015625f;
            out.d += (float)((iv ^ i) & 15) * 0.0078125f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)(uv & 31u) * 0.015625f;
            out.b -= (float)((uv >> 2) & 15u) * 0.0078125f;
            out.c += (float)((uv >> 3) & 15u) * 0.00390625f;
            out.d += (float)((uv ^ (unsigned)i) & 31u) * 0.001953125f;
        }
    }
    va_end(ap);

    return out;
}

unsigned axis3_w3_hfa_digest(struct Axis3W3Hfa v, unsigned salt) {
    unsigned acc = salt ^ 0x9e3779b9u;
    acc ^= q11(v.a) + 0x45d9f3bu;
    acc = rotl32(acc, 5u);
    acc ^= q11(v.b) + 0x27d4eb2du;
    acc = rotl32(acc, 7u);
    acc ^= q11(v.c) + 0x85ebca6bu;
    acc = rotl32(acc, 11u);
    acc ^= q11(v.d) + 0xc2b2ae35u;
    acc = rotl32(acc, 13u);
    return acc;
}
