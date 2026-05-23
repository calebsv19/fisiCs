typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W30Hfa {
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
        v = (-v * 1.671875f) + 0.5f;
    }
    return (unsigned)(v * 1024.0f + 0.5f);
}

struct Axis3W30Hfa axis3_w30_hfa_collect(
    float seed,
    unsigned shadow,
    unsigned handoff,
    unsigned frontier,
    unsigned checkpoint,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W30Hfa out;
    static const int lane_map[43] = {
        3, 1, 0, 2, 3, 2, 1, 0, 3, 1, 2, 0, 3, 0,
        1, 2, 3, 1, 0, 2, 3, 2, 0, 1, 3, 1, 2, 0,
        3, 0, 1, 2, 3, 2, 1, 0, 3, 1, 0, 2, 3, 2,
        1
    };
    int i;

    out.a = seed + (float)(shadow & 63u) * 0.0091552734375f;
    out.b = seed * 0.8046875f + 0.6796875f;
    out.c = seed * 0.54296875f + 1.1640625f;
    out.d = seed * 0.40234375f + 1.3984375f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (i >> 1) + (i >> 3)
             + (int)(shadow & 7u) + (int)(handoff & 7u)
             + (int)(frontier & 3u) + (int)(checkpoint & 3u)) % 43
        ];
        float s = (float)(i + 1) * 0.00347900390625f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.240234375f + s);
            } else if (lane == 1) {
                out.b += fv * (0.162109375f + s);
            } else if (lane == 2) {
                out.c += fv * (0.1494140625f + s);
            } else {
                out.d += fv * (0.1279296875f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane + i) & 63) * 0.00567626953125f;
            out.b -= (float)((iv >> 1) & 31) * 0.0025634765625f;
            out.c += (float)((iv >> 2) & 31) * 0.002899169921875f;
            out.d += (float)((iv ^ (i + lane + 9)) & 31) * 0.001708984375f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)((uv ^ shadow ^ frontier) & 63u) * 0.004608154296875f;
            out.b += (float)((uv >> 2) & 31u) * 0.001708984375f;
            out.c -= (float)((uv >> 3) & 31u) * 0.0013427734375f;
            out.d += (float)((uv ^ handoff ^ checkpoint) & 31u) * 0.001129150390625f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(shadow ^ handoff, frontier & 31u) & 63u) * 0.001617431640625f;
    out.b += (float)(rotl32(handoff ^ frontier, checkpoint & 31u) & 31u) * 0.001434326171875f;
    out.c += (float)(rotl32(frontier ^ checkpoint, shadow & 31u) & 31u) * 0.001190185546875f;
    out.d -= (float)(rotl32(checkpoint ^ shadow, handoff & 31u) & 31u) * 0.001068115234375f;
    return out;
}

unsigned axis3_w30_hfa_digest(struct Axis3W30Hfa v, unsigned salt) {
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
