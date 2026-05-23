typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W35Hfa {
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
        v = (-v * 1.6796875f) + 0.5f;
    }
    return (unsigned)(v * 1024.0f + 0.5f);
}

struct Axis3W35Hfa axis3_w35_hfa_collect(
    float seed,
    unsigned checkpoint,
    unsigned handoff,
    unsigned shadow,
    unsigned replay,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W35Hfa out;
    static const int lane_map[47] = {
        0, 2, 1, 3, 0, 3, 2, 1, 0, 2, 3, 1, 0, 1, 2, 3,
        0, 2, 1, 3, 0, 3, 1, 2, 0, 2, 3, 1, 0, 1, 3, 2,
        0, 3, 2, 1, 0, 2, 1, 3, 0, 3, 1, 2, 0, 1, 3
    };
    int i;

    out.a = seed + (float)(checkpoint & 63u) * 0.0091552734375f;
    out.b = seed * 0.806640625f + 0.677734375f;
    out.c = seed * 0.546875f + 1.166015625f;
    out.d = seed * 0.3984375f + 1.404296875f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (i >> 1) + (i >> 2) + (i >> 4)
             + (int)(checkpoint & 7u) + (int)(handoff & 7u)
             + (int)(shadow & 3u) + (int)(replay & 3u)) % 47
        ];
        float s = (float)(i + 1) * 0.003265380859375f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.240234375f + s);
            } else if (lane == 1) {
                out.b += fv * (0.16015625f + s);
            } else if (lane == 2) {
                out.c += fv * (0.150390625f + s);
            } else {
                out.d += fv * (0.12744140625f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane + i) & 63) * 0.00555419921875f;
            out.b -= (float)((iv >> 1) & 31) * 0.00250244140625f;
            out.c += (float)((iv >> 2) & 31) * 0.002960205078125f;
            out.d += (float)((iv ^ (i + lane + 19)) & 31) * 0.00164794921875f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)((uv ^ checkpoint ^ shadow) & 63u) * 0.0045318603515625f;
            out.b += (float)((uv >> 2) & 31u) * 0.00165557861328125f;
            out.c -= (float)((uv >> 3) & 31u) * 0.001312255859375f;
            out.d += (float)((uv ^ handoff ^ replay) & 31u) * 0.001129150390625f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(checkpoint ^ handoff, shadow & 31u) & 63u) * 0.0015869140625f;
    out.b += (float)(rotl32(handoff ^ shadow, replay & 31u) & 31u) * 0.001434326171875f;
    out.c += (float)(rotl32(shadow ^ replay, checkpoint & 31u) & 31u) * 0.00119781494140625f;
    out.d -= (float)(rotl32(replay ^ checkpoint, handoff & 31u) & 31u) * 0.0010833740234375f;
    return out;
}

unsigned axis3_w35_hfa_digest(struct Axis3W35Hfa v, unsigned salt) {
    unsigned acc = salt ^ 0x9e3779b9u;
    acc ^= q10(v.a) + 0x27d4eb2du;
    acc = rotl32(acc, 5u);
    acc ^= q10(v.b) + 0x85ebca6bu;
    acc = rotl32(acc, 7u);
    acc ^= q10(v.c) + 0xc2b2ae35u;
    acc = rotl32(acc, 11u);
    acc ^= q10(v.d) + 0x165667b1u;
    acc = rotl32(acc, 13u);
    return acc;
}
