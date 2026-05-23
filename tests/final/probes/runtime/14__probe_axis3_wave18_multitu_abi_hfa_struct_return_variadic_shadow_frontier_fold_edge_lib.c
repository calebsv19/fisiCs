typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W18Hfa {
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
        v = (-v * 1.75f) + 0.5f;
    }
    return (unsigned)(v * 1024.0f + 0.5f);
}

struct Axis3W18Hfa axis3_w18_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned frontier,
    unsigned shadow,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W18Hfa out;
    static const int lane_map[25] = {
        2, 0, 3, 1, 2, 1, 0, 3, 2, 0, 1, 3, 2,
        3, 1, 0, 2, 1, 3, 0, 2, 0, 1, 3, 2
    };
    int i;

    out.a = seed + (float)(epoch & 63u) * 0.01123046875f;
    out.b = seed * 0.7890625f + 0.7265625f;
    out.c = seed * 0.5859375f + 0.96875f;
    out.d = seed * 0.4375f + 1.171875f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (i >> 1) + (i >> 2) + (i >> 3)
             + (int)(frontier & 7u) + (int)(shadow & 7u) + (int)(epoch & 3u)) % 25
        ];
        float s = (float)(i + 1) * 0.0048828125f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.203125f + s);
            } else if (lane == 1) {
                out.b += fv * (0.1875f + s);
            } else if (lane == 2) {
                out.c += fv * (0.17578125f + s);
            } else {
                out.d += fv * (0.1640625f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane + i) & 63) * 0.005126953125f;
            out.b -= (float)((iv >> 1) & 31) * 0.002197265625f;
            out.c += (float)((iv >> 2) & 31) * 0.0025634765625f;
            out.d += (float)((iv ^ (i + lane + 1)) & 31) * 0.001953125f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)((uv ^ epoch ^ shadow) & 63u) * 0.004150390625f;
            out.b += (float)((uv >> 2) & 31u) * 0.0013427734375f;
            out.c -= (float)((uv >> 3) & 31u) * 0.001220703125f;
            out.d += (float)((uv ^ shadow ^ frontier) & 31u) * 0.000732421875f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(epoch ^ frontier, shadow & 31u) & 63u) * 0.00152587890625f;
    out.b += (float)(rotl32(frontier ^ shadow, epoch & 31u) & 31u) * 0.00128173828125f;
    out.c += (float)(rotl32(shadow ^ epoch, frontier & 31u) & 31u) * 0.0008544921875f;
    out.d -= (float)((epoch >> 1) & 31u) * 0.001220703125f;
    return out;
}

unsigned axis3_w18_hfa_digest(struct Axis3W18Hfa v, unsigned salt) {
    unsigned acc = salt ^ 0x85ebca6bu;
    acc ^= q10(v.a) + 0x27d4eb2du;
    acc = rotl32(acc, 5u);
    acc ^= q10(v.b) + 0x45d9f3bu;
    acc = rotl32(acc, 7u);
    acc ^= q10(v.c) + 0xc2b2ae35u;
    acc = rotl32(acc, 11u);
    acc ^= q10(v.d) + 0x9e3779b9u;
    acc = rotl32(acc, 13u);
    return acc;
}
