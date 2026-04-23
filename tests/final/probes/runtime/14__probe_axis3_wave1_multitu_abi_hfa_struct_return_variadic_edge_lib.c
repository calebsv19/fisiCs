typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W1Hfa {
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
        v = (-v * 1.5f) + 0.25f;
    }
    return (unsigned)(v * 1024.0f + 0.5f);
}

struct Axis3W1Hfa axis3_w1_hfa_collect(float seed, int lanes, ...) {
    va_list ap;
    struct Axis3W1Hfa out;
    int i;

    out.a = seed + 0.125f;
    out.b = seed * 0.75f + 0.5f;
    out.c = seed * 1.25f + 0.75f;
    out.d = seed * 0.5f + 1.0f;

    va_start(ap, lanes);
    for (i = 0; i < lanes; ++i) {
        int kind = va_arg(ap, int);
        if (kind == 0) {
            double dv = va_arg(ap, double);
            float fv = (float)dv;
            out.a += fv * (0.25f + (float)(i + 1) * 0.125f);
            out.b += fv * (0.125f + (float)(i + 2) * 0.0625f);
            out.c += fv * 0.1875f;
            out.d += fv * (0.15625f + (float)(i & 1) * 0.03125f);
        } else {
            int iv = va_arg(ap, int);
            out.a += (float)(iv & 7) * 0.5f;
            out.b += (float)(iv + i) * 0.0625f;
            out.c -= (float)(iv & 3) * 0.03125f;
            out.d += (float)(iv * (i + 1)) * 0.015625f;
        }
    }
    va_end(ap);

    return out;
}

unsigned axis3_w1_hfa_digest(struct Axis3W1Hfa v, unsigned salt) {
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
