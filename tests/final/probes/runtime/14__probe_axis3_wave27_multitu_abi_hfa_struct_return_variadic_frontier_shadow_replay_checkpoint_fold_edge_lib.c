typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W27Hfa {
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
    if (v < 0.0f) {
        v = (-v * 1.59375f) + 0.5f;
    }
    return (unsigned)(v * 1024.0f + 0.5f);
}

struct Axis3W27Hfa axis3_w27_hfa_collect(
    float seed,
    unsigned frontier,
    unsigned shadow,
    unsigned replay,
    unsigned checkpoint,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W27Hfa out;
    static const int lane_map[43] = {
        2, 0, 3, 1, 2, 1, 0, 3, 2, 0, 1, 3, 2, 3,
        0, 1, 2, 0, 3, 1, 2, 1, 3, 0, 2, 0, 1, 3,
        2, 3, 1, 0, 2, 1, 0, 3, 2, 0, 3, 1, 2, 1,
        3
    };
    int i;

    out.a = seed + (float)(frontier & 63u) * 0.009521484375f;
    out.b = seed * 0.8125f + 0.671875f;
    out.c = seed * 0.546875f + 1.171875f;
    out.d = seed * 0.40625f + 1.390625f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (i >> 1) + (i >> 2)
             + (int)(frontier & 7u) + (int)(shadow & 7u)
             + (int)(replay & 3u) + (int)(checkpoint & 3u)) % 43
        ];
        float s = (float)(i + 1) * 0.00341796875f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.2421875f + s);
            } else if (lane == 1) {
                out.b += fv * (0.1640625f + s);
            } else if (lane == 2) {
                out.c += fv * (0.1484375f + s);
            } else {
                out.d += fv * (0.12890625f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane + i) & 63) * 0.005615234375f;
            out.b -= (float)((iv >> 1) & 31) * 0.00262451171875f;
            out.c += (float)((iv >> 2) & 31) * 0.00286865234375f;
            out.d += (float)((iv ^ (i + lane + 3)) & 31) * 0.00177001953125f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)((uv ^ frontier ^ checkpoint) & 63u) * 0.004638671875f;
            out.b += (float)((uv >> 2) & 31u) * 0.001708984375f;
            out.c -= (float)((uv >> 3) & 31u) * 0.0013427734375f;
            out.d += (float)((uv ^ shadow ^ replay) & 31u) * 0.001129150390625f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(frontier ^ shadow, replay & 31u) & 63u) * 0.001708984375f;
    out.b += (float)(rotl32(shadow ^ replay, checkpoint & 31u) & 31u) * 0.00140380859375f;
    out.c += (float)(rotl32(replay ^ checkpoint, frontier & 31u) & 31u) * 0.00115966796875f;
    out.d -= (float)(rotl32(checkpoint ^ frontier, shadow & 31u) & 31u) * 0.00103759765625f;
    return out;
}

unsigned axis3_w27_hfa_digest(struct Axis3W27Hfa v, unsigned salt) {
    unsigned acc = salt ^ 0x85ebca6bu;
    acc ^= q10(v.a) + 0x27d4eb2du;
    acc = rotl32(acc, 5u);
    acc ^= q10(v.b) + 0x9e3779b9u;
    acc = rotl32(acc, 7u);
    acc ^= q10(v.c) + 0xc2b2ae35u;
    acc = rotl32(acc, 11u);
    acc ^= q10(v.d) + 0x165667b1u;
    acc = rotl32(acc, 13u);
    return acc;
}
