typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W29Hfa {
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
        v = (-v * 1.625f) + 0.5f;
    }
    return (unsigned)(v * 1024.0f + 0.5f);
}

struct Axis3W29Hfa axis3_w29_hfa_collect(
    float seed,
    unsigned replay,
    unsigned frontier,
    unsigned checkpoint,
    unsigned shadow,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W29Hfa out;
    static const int lane_map[41] = {
        0, 2, 1, 3, 0, 3, 2, 1, 0, 2, 3, 1, 0, 1,
        2, 3, 0, 2, 1, 3, 0, 3, 1, 2, 0, 2, 3, 1,
        0, 1, 3, 2, 0, 3, 2, 1, 0, 2, 1, 3, 0
    };
    int i;

    out.a = seed + (float)(replay & 63u) * 0.00927734375f;
    out.b = seed * 0.796875f + 0.6875f;
    out.c = seed * 0.5546875f + 1.15625f;
    out.d = seed * 0.3984375f + 1.40625f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (i >> 1) + (i >> 2)
             + (int)(replay & 7u) + (int)(frontier & 7u)
             + (int)(checkpoint & 3u) + (int)(shadow & 3u)) % 41
        ];
        float s = (float)(i + 1) * 0.00360107421875f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.24609375f + s);
            } else if (lane == 1) {
                out.b += fv * (0.15625f + s);
            } else if (lane == 2) {
                out.c += fv * (0.150390625f + s);
            } else {
                out.d += fv * (0.126953125f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane + i) & 63) * 0.00555419921875f;
            out.b -= (float)((iv >> 1) & 31) * 0.00250244140625f;
            out.c += (float)((iv >> 2) & 31) * 0.00299072265625f;
            out.d += (float)((iv ^ (i + lane + 7)) & 31) * 0.00164794921875f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)((uv ^ replay ^ frontier) & 63u) * 0.00457763671875f;
            out.b += (float)((uv >> 2) & 31u) * 0.00164794921875f;
            out.c -= (float)((uv >> 3) & 31u) * 0.001312255859375f;
            out.d += (float)((uv ^ checkpoint ^ shadow) & 31u) * 0.00115966796875f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(replay ^ frontier, checkpoint & 31u) & 63u) * 0.0015869140625f;
    out.b += (float)(rotl32(frontier ^ checkpoint, shadow & 31u) & 31u) * 0.001434326171875f;
    out.c += (float)(rotl32(checkpoint ^ shadow, replay & 31u) & 31u) * 0.001190185546875f;
    out.d -= (float)(rotl32(shadow ^ replay, frontier & 31u) & 31u) * 0.0010986328125f;
    return out;
}

unsigned axis3_w29_hfa_digest(struct Axis3W29Hfa v, unsigned salt) {
    unsigned acc = salt ^ 0xc2b2ae35u;
    acc ^= q10(v.a) + 0x27d4eb2du;
    acc = rotl32(acc, 5u);
    acc ^= q10(v.b) + 0x85ebca6bu;
    acc = rotl32(acc, 7u);
    acc ^= q10(v.c) + 0x9e3779b9u;
    acc = rotl32(acc, 11u);
    acc ^= q10(v.d) + 0x165667b1u;
    acc = rotl32(acc, 13u);
    return acc;
}
