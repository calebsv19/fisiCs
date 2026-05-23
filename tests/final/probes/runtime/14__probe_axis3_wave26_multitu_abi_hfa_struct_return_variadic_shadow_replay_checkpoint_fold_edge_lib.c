typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W26Hfa {
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
        v = (-v * 1.71875f) + 0.5f;
    }
    return (unsigned)(v * 1024.0f + 0.5f);
}

struct Axis3W26Hfa axis3_w26_hfa_collect(
    float seed,
    unsigned shadow,
    unsigned replay,
    unsigned checkpoint,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W26Hfa out;
    static const int lane_map[41] = {
        3, 1, 0, 2, 3, 2, 1, 0, 3, 1, 2, 0, 3, 0,
        1, 2, 3, 1, 0, 2, 3, 2, 0, 1, 3, 1, 2, 0,
        3, 0, 1, 2, 3, 2, 1, 0, 3, 1, 0, 2, 3
    };
    int i;

    out.a = seed + (float)(shadow & 63u) * 0.01019287109375f;
    out.b = seed * 0.859375f + 0.609375f;
    out.c = seed * 0.51953125f + 1.234375f;
    out.d = seed * 0.375f + 1.453125f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (i >> 1) + (i >> 3) + (i >> 4)
             + (int)(shadow & 7u) + (int)(replay & 7u) + (int)(checkpoint & 3u)) % 41
        ];
        float s = (float)(i + 1) * 0.0037841796875f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.25f + s);
            } else if (lane == 1) {
                out.b += fv * (0.15625f + s);
            } else if (lane == 2) {
                out.c += fv * (0.15625f + s);
            } else {
                out.d += fv * (0.130859375f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane + i) & 63) * 0.0059814453125f;
            out.b -= (float)((iv >> 1) & 31) * 0.00250244140625f;
            out.c += (float)((iv >> 2) & 31) * 0.0030517578125f;
            out.d += (float)((iv ^ (i + lane + 1)) & 31) * 0.00164794921875f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)((uv ^ shadow ^ checkpoint) & 63u) * 0.0048828125f;
            out.b += (float)((uv >> 2) & 31u) * 0.0015869140625f;
            out.c -= (float)((uv >> 3) & 31u) * 0.00146484375f;
            out.d += (float)((uv ^ replay ^ checkpoint) & 31u) * 0.001007080078125f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(shadow ^ replay, checkpoint & 31u) & 63u) * 0.00177001953125f;
    out.b += (float)(rotl32(replay ^ checkpoint, shadow & 31u) & 31u) * 0.00152587890625f;
    out.c += (float)(rotl32(checkpoint ^ shadow, replay & 31u) & 31u) * 0.0010986328125f;
    out.d -= (float)((checkpoint >> 1) & 31u) * 0.00146484375f;
    return out;
}

unsigned axis3_w26_hfa_digest(struct Axis3W26Hfa v, unsigned salt) {
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
