typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

struct Axis3W14Hfa {
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
        v = (-v * 1.5f) + 0.5f;
    }
    return (unsigned)(v * 1024.0f + 0.5f);
}

struct Axis3W14Hfa axis3_w14_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
) {
    va_list ap;
    struct Axis3W14Hfa out;
    static const int lane_map[19] = {3, 0, 2, 1, 3, 2, 0, 1, 2, 3, 1, 0, 2, 1, 3, 0, 2, 3, 1};
    int i;

    out.a = seed + (float)(epoch & 31u) * 0.013671875f;
    out.b = seed * 0.8046875f + 0.65625f;
    out.c = seed * 0.56640625f + 1.0f;
    out.d = seed * 0.4296875f + 1.21875f;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        int kind = va_arg(ap, int);
        int lane = lane_map[
            (i + (i >> 1) + (i >> 2) + (int)(frontier & 7u) + (int)(replay & 3u) + (int)(epoch & 3u)) % 19
        ];
        float s = (float)(i + 1) * 0.00634765625f;

        if (kind == 0) {
            float fv = (float)va_arg(ap, double);
            if (lane == 0) {
                out.a += fv * (0.21875f + s);
            } else if (lane == 1) {
                out.b += fv * (0.1953125f + s);
            } else if (lane == 2) {
                out.c += fv * (0.1796875f + s);
            } else {
                out.d += fv * (0.14453125f + s);
            }
        } else if (kind == 1) {
            int iv = va_arg(ap, int);
            out.a += (float)((iv + lane + i) & 63) * 0.006103515625f;
            out.b -= (float)((iv >> 1) & 31) * 0.002685546875f;
            out.c += (float)((iv >> 2) & 31) * 0.0025634765625f;
            out.d += (float)((iv ^ (i + lane + 1)) & 31) * 0.0018310546875f;
        } else {
            unsigned uv = va_arg(ap, unsigned);
            out.a += (float)((uv ^ epoch ^ replay) & 63u) * 0.00390625f;
            out.b += (float)((uv >> 2) & 31u) * 0.0015869140625f;
            out.c -= (float)((uv >> 3) & 31u) * 0.00115966796875f;
            out.d += (float)((uv ^ replay ^ frontier) & 31u) * 0.0008544921875f;
        }
    }
    va_end(ap);

    out.a += (float)(rotl32(epoch ^ frontier, replay & 31u) & 31u) * 0.0018310546875f;
    out.b += (float)(rotl32(frontier ^ replay, epoch & 31u) & 15u) * 0.0015869140625f;
    out.c += (float)(rotl32(replay ^ epoch, frontier & 31u) & 15u) * 0.00103759765625f;
    out.d -= (float)((epoch >> 2) & 15u) * 0.00140380859375f;
    return out;
}

unsigned axis3_w14_hfa_digest(struct Axis3W14Hfa v, unsigned salt) {
    unsigned acc = salt ^ 0x9e3779b9u;
    acc ^= q10(v.a) + 0x45d9f3bu;
    acc = rotl32(acc, 5u);
    acc ^= q10(v.b) + 0x27d4eb2du;
    acc = rotl32(acc, 7u);
    acc ^= q10(v.c) + 0x85ebca6bu;
    acc = rotl32(acc, 11u);
    acc ^= q10(v.d) + 0xc2b2ae35u;
    acc = rotl32(acc, 13u);
    return acc;
}
