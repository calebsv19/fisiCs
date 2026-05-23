typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W31Hfa {
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
        v = (-v * 1.640625f) + 0.5f;
    }
    return (unsigned)(v * 1024.0f + 0.5f);
}

struct Axis3W31Hfa axis3_w31_hfa_collect(
    float seed,
    unsigned checkpoint,
    unsigned replay,
    unsigned handoff,
    unsigned shadow,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W31Hfa out;
    static const int lane_map[41] = {
        2, 0, 3, 1, 2, 1, 0, 3, 2, 0, 1, 3, 2, 3,
        0, 1, 2, 0, 3, 1, 2, 1, 3, 0, 2, 0, 1, 3,
        2, 3, 1, 0, 2, 1, 0, 3, 2, 0, 3, 1, 2
    };
    int i;

    out.a = seed + (float)(checkpoint & 63u) * 0.0093994140625f;
    out.b = seed * 0.80078125f + 0.68359375f;
    out.c = seed * 0.55078125f + 1.16015625f;
    out.d = seed * 0.39453125f + 1.41015625f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (i >> 1) + (i >> 2)
             + (int)(checkpoint & 7u) + (int)(replay & 7u)
             + (int)(handoff & 3u) + (int)(shadow & 3u)) % 41
        ];
        float s = (float)(i + 1) * 0.003662109375f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.244140625f + s);
            } else if (lane == 1) {
                out.b += fv * (0.158203125f + s);
            } else if (lane == 2) {
                out.c += fv * (0.1513671875f + s);
            } else {
                out.d += fv * (0.126953125f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane + i) & 63) * 0.005615234375f;
            out.b -= (float)((iv >> 1) & 31) * 0.002532958984375f;
            out.c += (float)((iv >> 2) & 31) * 0.002960205078125f;
            out.d += (float)((iv ^ (i + lane + 11)) & 31) * 0.001678466796875f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)((uv ^ checkpoint ^ replay) & 63u) * 0.004547119140625f;
            out.b += (float)((uv >> 2) & 31u) * 0.001678466796875f;
            out.c -= (float)((uv >> 3) & 31u) * 0.0013275146484375f;
            out.d += (float)((uv ^ handoff ^ shadow) & 31u) * 0.0011444091796875f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(checkpoint ^ replay, handoff & 31u) & 63u) * 0.0016021728515625f;
    out.b += (float)(rotl32(replay ^ handoff, shadow & 31u) & 31u) * 0.0014495849609375f;
    out.c += (float)(rotl32(handoff ^ shadow, checkpoint & 31u) & 31u) * 0.0012054443359375f;
    out.d -= (float)(rotl32(shadow ^ checkpoint, replay & 31u) & 31u) * 0.0010833740234375f;
    return out;
}

unsigned axis3_w31_hfa_digest(struct Axis3W31Hfa v, unsigned salt) {
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
