typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W24Hfa {
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
        v = (-v * 1.6875f) + 0.5f;
    }
    return (unsigned)(v * 1024.0f + 0.5f);
}

struct Axis3W24Hfa axis3_w24_hfa_collect(
    float seed,
    unsigned shadow,
    unsigned frontier,
    unsigned handoff,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W24Hfa out;
    static const int lane_map[37] = {
        1, 3, 0, 2, 1, 2, 3, 0, 1, 3, 2, 0,
        1, 0, 3, 2, 1, 2, 0, 3, 1, 3, 0, 2,
        1, 0, 2, 3, 1, 2, 3, 0, 1, 3, 2, 1, 0
    };
    int i;

    out.a = seed + (float)(shadow & 63u) * 0.01043701171875f;
    out.b = seed * 0.84375f + 0.640625f;
    out.c = seed * 0.53515625f + 1.171875f;
    out.d = seed * 0.390625f + 1.390625f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (i >> 1) + (i >> 3) + (i >> 4)
             + (int)(shadow & 7u) + (int)(frontier & 7u) + (int)(handoff & 3u)) % 37
        ];
        float s = (float)(i + 1) * 0.0040283203125f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.23828125f + s);
            } else if (lane == 1) {
                out.b += fv * (0.1640625f + s);
            } else if (lane == 2) {
                out.c += fv * (0.16015625f + s);
            } else {
                out.d += fv * (0.138671875f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane + i) & 63) * 0.005859375f;
            out.b -= (float)((iv >> 1) & 31) * 0.00244140625f;
            out.c += (float)((iv >> 2) & 31) * 0.0029296875f;
            out.d += (float)((iv ^ (i + lane + 1)) & 31) * 0.001708984375f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)((uv ^ shadow ^ handoff) & 63u) * 0.0047607421875f;
            out.b += (float)((uv >> 2) & 31u) * 0.00152587890625f;
            out.c -= (float)((uv >> 3) & 31u) * 0.00140380859375f;
            out.d += (float)((uv ^ frontier ^ handoff) & 31u) * 0.000946044921875f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(shadow ^ frontier, handoff & 31u) & 63u) * 0.001708984375f;
    out.b += (float)(rotl32(frontier ^ handoff, shadow & 31u) & 31u) * 0.00146484375f;
    out.c += (float)(rotl32(handoff ^ shadow, frontier & 31u) & 31u) * 0.00103759765625f;
    out.d -= (float)((handoff >> 1) & 31u) * 0.00140380859375f;
    return out;
}

unsigned axis3_w24_hfa_digest(struct Axis3W24Hfa v, unsigned salt) {
    unsigned acc = salt ^ 0x85ebca6bu;
    acc ^= q10(v.a) + 0x27d4eb2du;
    acc = rotl32(acc, 5u);
    acc ^= q10(v.b) + 0xc2b2ae35u;
    acc = rotl32(acc, 7u);
    acc ^= q10(v.c) + 0x9e3779b9u;
    acc = rotl32(acc, 11u);
    acc ^= q10(v.d) + 0x45d9f3bu;
    acc = rotl32(acc, 13u);
    return acc;
}
