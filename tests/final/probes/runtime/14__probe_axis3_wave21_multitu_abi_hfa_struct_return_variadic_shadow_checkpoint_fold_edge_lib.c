typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W21Hfa {
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

struct Axis3W21Hfa axis3_w21_hfa_collect(
    float seed,
    unsigned shadow,
    unsigned checkpoint,
    unsigned frontier,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W21Hfa out;
    static const int lane_map[31] = {
        2, 1, 3, 0, 2, 0, 1, 3, 2, 1,
        0, 3, 2, 3, 1, 0, 2, 1, 3, 0,
        2, 0, 1, 3, 2, 1, 0, 3, 1, 2, 0
    };
    int i;

    out.a = seed + (float)(shadow & 63u) * 0.010498046875f;
    out.b = seed * 0.8203125f + 0.6875f;
    out.c = seed * 0.55859375f + 1.078125f;
    out.d = seed * 0.4140625f + 1.296875f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (i >> 1) + (i >> 2) + (i >> 4)
             + (int)(shadow & 7u) + (int)(checkpoint & 7u) + (int)(frontier & 3u)) % 31
        ];
        float s = (float)(i + 1) * 0.00439453125f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.220703125f + s);
            } else if (lane == 1) {
                out.b += fv * (0.17578125f + s);
            } else if (lane == 2) {
                out.c += fv * (0.166015625f + s);
            } else {
                out.d += fv * (0.150390625f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane + i) & 63) * 0.005615234375f;
            out.b -= (float)((iv >> 1) & 31) * 0.002349853515625f;
            out.c += (float)((iv >> 2) & 31) * 0.00274658203125f;
            out.d += (float)((iv ^ (i + lane + 1)) & 31) * 0.001800537109375f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)((uv ^ shadow ^ frontier) & 63u) * 0.0045166015625f;
            out.b += (float)((uv >> 2) & 31u) * 0.001434326171875f;
            out.c -= (float)((uv >> 3) & 31u) * 0.001312255859375f;
            out.d += (float)((uv ^ checkpoint ^ frontier) & 31u) * 0.0008544921875f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(shadow ^ checkpoint, frontier & 31u) & 63u) * 0.001617431640625f;
    out.b += (float)(rotl32(checkpoint ^ frontier, shadow & 31u) & 31u) * 0.001373291015625f;
    out.c += (float)(rotl32(frontier ^ shadow, checkpoint & 31u) & 31u) * 0.000946044921875f;
    out.d -= (float)((checkpoint >> 1) & 31u) * 0.001312255859375f;
    return out;
}

unsigned axis3_w21_hfa_digest(struct Axis3W21Hfa v, unsigned salt) {
    unsigned acc = salt ^ 0x85ebca6bu;
    acc ^= q10(v.a) + 0x27d4eb2du;
    acc = rotl32(acc, 5u);
    acc ^= q10(v.b) + 0x9e3779b9u;
    acc = rotl32(acc, 7u);
    acc ^= q10(v.c) + 0xc2b2ae35u;
    acc = rotl32(acc, 11u);
    acc ^= q10(v.d) + 0x45d9f3bu;
    acc = rotl32(acc, 13u);
    return acc;
}
