typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W22Hfa {
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
        v = (-v * 1.65625f) + 0.5f;
    }
    return (unsigned)(v * 1024.0f + 0.5f);
}

struct Axis3W22Hfa axis3_w22_hfa_collect(
    float seed,
    unsigned frontier,
    unsigned replay,
    unsigned handoff,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W22Hfa out;
    static const int lane_map[33] = {
        3, 0, 2, 1, 3, 1, 0, 2, 3, 2, 1,
        0, 3, 1, 2, 0, 3, 2, 1, 0, 2, 3,
        1, 0, 2, 3, 1, 3, 0, 2, 1, 0, 2
    };
    int i;

    out.a = seed + (float)(frontier & 63u) * 0.0106201171875f;
    out.b = seed * 0.828125f + 0.671875f;
    out.c = seed * 0.55078125f + 1.109375f;
    out.d = seed * 0.40625f + 1.328125f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (i >> 1) + (i >> 3) + (i >> 4)
             + (int)(frontier & 7u) + (int)(replay & 7u) + (int)(handoff & 3u)) % 33
        ];
        float s = (float)(i + 1) * 0.0042724609375f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.2265625f + s);
            } else if (lane == 1) {
                out.b += fv * (0.171875f + s);
            } else if (lane == 2) {
                out.c += fv * (0.1640625f + s);
            } else {
                out.d += fv * (0.146484375f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane + i) & 63) * 0.0057373046875f;
            out.b -= (float)((iv >> 1) & 31) * 0.00238037109375f;
            out.c += (float)((iv >> 2) & 31) * 0.0028076171875f;
            out.d += (float)((iv ^ (i + lane + 1)) & 31) * 0.00177001953125f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)((uv ^ frontier ^ handoff) & 63u) * 0.004638671875f;
            out.b += (float)((uv >> 2) & 31u) * 0.00146484375f;
            out.c -= (float)((uv >> 3) & 31u) * 0.0013427734375f;
            out.d += (float)((uv ^ replay ^ handoff) & 31u) * 0.000885009765625f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(frontier ^ replay, handoff & 31u) & 63u) * 0.00164794921875f;
    out.b += (float)(rotl32(replay ^ handoff, frontier & 31u) & 31u) * 0.00140380859375f;
    out.c += (float)(rotl32(handoff ^ frontier, replay & 31u) & 31u) * 0.0009765625f;
    out.d -= (float)((handoff >> 1) & 31u) * 0.0013427734375f;
    return out;
}

unsigned axis3_w22_hfa_digest(struct Axis3W22Hfa v, unsigned salt) {
    unsigned acc = salt ^ 0xc2b2ae35u;
    acc ^= q10(v.a) + 0x27d4eb2du;
    acc = rotl32(acc, 5u);
    acc ^= q10(v.b) + 0x85ebca6bu;
    acc = rotl32(acc, 7u);
    acc ^= q10(v.c) + 0x9e3779b9u;
    acc = rotl32(acc, 11u);
    acc ^= q10(v.d) + 0x45d9f3bu;
    acc = rotl32(acc, 13u);
    return acc;
}
