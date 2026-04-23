typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W4Hfa {
    float x;
    float y;
    float z;
    float w;
};

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned q10(float v) {
    if (v < 0.0f) {
        v = (-v * 1.25f) + 0.75f;
    }
    return (unsigned)(v * 1024.0f + 0.5f);
}

struct Axis3W4Hfa axis3_w4_hfa_collect(float seed, unsigned phase, int count, ...) {
    va_list ap;
    struct Axis3W4Hfa out;
    static const int lane_map[6] = {2, 0, 3, 1, 2, 1};
    int i;

    out.x = seed + (float)(phase & 7u) * 0.03125f;
    out.y = seed * 0.75f + 0.5f;
    out.z = seed * 1.125f + 0.75f;
    out.w = seed * 0.5625f + 1.0f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[i % 6];
        float s = (float)(i + 3) * 0.0234375f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.x += fv * (0.3125f + s);
            } else if (lane == 1) {
                out.y += fv * (0.21875f + s);
            } else if (lane == 2) {
                out.z += fv * (0.15625f + s);
            } else {
                out.w += fv * (0.125f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.x += (float)((iv + i) & 31) * 0.015625f;
            out.y -= (float)((iv >> 1) & 15) * 0.0078125f;
            out.z += (float)((iv ^ lane) & 15) * 0.00390625f;
            out.w += (float)(iv & 7) * 0.001953125f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.x += (float)(uv & 63u) * 0.0078125f;
            out.y += (float)((uv >> 2) & 31u) * 0.00390625f;
            out.z -= (float)((uv >> 3) & 15u) * 0.001953125f;
            out.w += (float)((uv ^ phase) & 31u) * 0.0009765625f;
        }
    }
    va_end(ap);

    out.x += (float)(phase & 31u) * 0.015625f;
    out.z += (float)((phase >> 3) & 15u) * 0.0078125f;
    return out;
}

unsigned axis3_w4_hfa_digest(struct Axis3W4Hfa v, unsigned salt) {
    unsigned acc = salt ^ 0x85ebca6bu;
    acc ^= q10(v.x) + 0x45d9f3bu;
    acc = rotl32(acc, 5u);
    acc ^= q10(v.y) + 0x27d4eb2du;
    acc = rotl32(acc, 7u);
    acc ^= q10(v.z) + 0x9e3779b9u;
    acc = rotl32(acc, 11u);
    acc ^= q10(v.w) + 0xc2b2ae35u;
    acc = rotl32(acc, 13u);
    return acc;
}
