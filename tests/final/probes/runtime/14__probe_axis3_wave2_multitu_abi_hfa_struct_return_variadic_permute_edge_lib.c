typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W2Hfa {
    float x;
    float y;
    float z;
    float w;
};

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned q12(float v) {
    if (v < 0.0f) {
        v = (-v * 1.25f) + 0.5f;
    }
    return (unsigned)(v * 4096.0f + 0.5f);
}

struct Axis3W2Hfa axis3_w2_hfa_collect(float seed, int count, ...) {
    va_list ap;
    struct Axis3W2Hfa out;
    static const int permute[4] = {2, 0, 3, 1};
    int i;

    out.x = seed + 0.125f;
    out.y = seed * 0.75f + 0.5f;
    out.z = seed * 1.125f + 0.75f;
    out.w = seed * 0.625f + 1.0f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int p = permute[i & 3];
        float lane_scale = 0.03125f * (float)(i + 1);
        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (p == 0) {
                out.x += fv * (0.25f + lane_scale);
            } else if (p == 1) {
                out.y += fv * (0.1875f + lane_scale);
            } else if (p == 2) {
                out.z += fv * (0.15625f + lane_scale);
            } else {
                out.w += fv * (0.125f + lane_scale);
            }
        } else {
            int iv = va_arg(ap, int);
            float step = (float)((iv & 15) + i) * 0.0625f;
            out.x += step * (float)(p == 0);
            out.y += step * (float)(p == 1);
            out.z += step * (float)(p == 2);
            out.w += step * (float)(p == 3);
            out.x += (float)(iv & 3) * 0.03125f;
            out.z -= (float)((iv >> 1) & 3) * 0.015625f;
        }
    }
    va_end(ap);

    return out;
}

unsigned axis3_w2_hfa_digest(struct Axis3W2Hfa v, unsigned salt) {
    unsigned acc = salt ^ 0x9e3779b9u;
    acc ^= q12(v.x) + 0x45d9f3bu;
    acc = rotl32(acc, 5u);
    acc ^= q12(v.y) + 0x27d4eb2du;
    acc = rotl32(acc, 7u);
    acc ^= q12(v.z) + 0x85ebca6bu;
    acc = rotl32(acc, 11u);
    acc ^= q12(v.w) + 0xc2b2ae35u;
    acc = rotl32(acc, 13u);
    return acc;
}
