typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W19ReducedHfa {
    float a;
    float b;
    float c;
    float d;
};

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

static unsigned q10(float v) {
    return (unsigned)(v * 1024.0f + 0.5f);
}

struct Axis3W19ReducedHfa axis3_w19_reduced_collect(
    float seed,
    unsigned epoch,
    unsigned shadow,
    unsigned replay,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W19ReducedHfa out;
    int i;

    out.a = seed + (float)(epoch & 31u) * 0.015625f;
    out.b = seed * 0.75f + 0.5f;
    out.c = seed * 0.5f + 1.0f;
    out.d = seed * 0.375f + 1.25f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            out.a += fv * 0.125f;
            out.b += fv * 0.09375f;
            out.c += fv * 0.0625f;
            out.d += fv * 0.046875f;
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)(iv & 15) * 0.015625f;
            out.b -= (float)((iv >> 1) & 7) * 0.0078125f;
            out.c += (float)((iv >> 2) & 7) * 0.005859375f;
            out.d += (float)((iv ^ i) & 7) * 0.00390625f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)((uv ^ epoch) & 15u) * 0.01171875f;
            out.b += (float)((uv ^ shadow) & 7u) * 0.00390625f;
            out.c -= (float)((uv ^ replay) & 7u) * 0.0029296875f;
            out.d += (float)((uv >> 1) & 7u) * 0.001953125f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(epoch ^ shadow, replay & 31u) & 15u) * 0.00390625f;
    out.b += (float)(rotl32(shadow ^ replay, epoch & 31u) & 7u) * 0.00390625f;
    out.c += (float)(rotl32(replay ^ epoch, shadow & 31u) & 7u) * 0.001953125f;
    out.d -= (float)((epoch >> 1) & 7u) * 0.001953125f;
    return out;
}

unsigned axis3_w19_reduced_digest(struct Axis3W19ReducedHfa v, unsigned salt) {
    unsigned acc = salt ^ 0x9e3779b9u;
    acc ^= q10(v.a) + 0x27d4eb2du;
    acc = rotl32(acc, 5u);
    acc ^= q10(v.b) + 0x85ebca6bu;
    acc = rotl32(acc, 7u);
    acc ^= q10(v.c) + 0xc2b2ae35u;
    acc = rotl32(acc, 11u);
    acc ^= q10(v.d) + 0x45d9f3bu;
    acc = rotl32(acc, 13u);
    return acc;
}
