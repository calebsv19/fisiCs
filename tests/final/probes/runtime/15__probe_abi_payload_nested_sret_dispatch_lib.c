union AbiPayloadBits {
    unsigned word[3];
    struct {
        unsigned lo;
        unsigned hi;
        unsigned mask;
    } named;
};

struct AbiPayloadLeaf {
    unsigned tag;
    int delta;
    long double impulse;
    union AbiPayloadBits bits;
};

struct AbiPayloadFrame {
    unsigned epoch;
    struct AbiPayloadLeaf lanes[4];
    union {
        struct {
            int slot;
            unsigned salt;
        } route;
        unsigned raw[2];
    } meta;
    long double scale;
};

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned q12(long double v) {
    if (v < 0.0L) {
        v = -v + 0.125L;
    }
    return (unsigned)(v * 4096.0L + 0.5L);
}

struct AbiPayloadFrame abi_payload_make_frame(unsigned seed, int flip) {
    struct AbiPayloadFrame out;
    unsigned i;

    out.epoch = seed ^ rotl32((unsigned)(flip * 17 + 211), seed & 31u);
    out.meta.route.slot = flip;
    out.meta.route.salt = seed * 2654435761u + (unsigned)(flip * 97 + 19);
    out.scale = 0.75L + (long double)(seed & 15u) * 0.03125L;

    for (i = 0u; i < 4u; ++i) {
        unsigned base = seed + i * 41u + (unsigned)(flip * (int)(i + 3u));
        out.lanes[i].tag = rotl32(base ^ 0x9e3779b9u, (i * 5u + seed) & 31u);
        out.lanes[i].delta = flip + (int)(i * 7u) - (int)(seed & 9u);
        out.lanes[i].impulse = out.scale * (long double)(i + 1u) + (long double)flip * 0.015625L;
        out.lanes[i].bits.named.lo = base * 17u + 3u;
        out.lanes[i].bits.named.hi = rotl32(base + 0x45d9f3bu, i + 7u);
        out.lanes[i].bits.named.mask = (out.lanes[i].bits.named.lo ^ out.lanes[i].bits.named.hi) + seed;
    }

    return out;
}

unsigned abi_payload_fold_frame(struct AbiPayloadFrame frame, unsigned salt) {
    unsigned acc = salt ^ frame.epoch ^ frame.meta.raw[1];
    unsigned i;

    for (i = 0u; i < 4u; ++i) {
        struct AbiPayloadLeaf leaf = frame.lanes[(i + (unsigned)frame.meta.route.slot) & 3u];
        acc ^= leaf.tag + (unsigned)(leaf.delta * 131) + q12(leaf.impulse);
        acc = rotl32(acc, (i * 3u + 5u) & 31u);
        acc += leaf.bits.word[i % 3u] ^ leaf.bits.named.mask;
        acc ^= rotl32(leaf.bits.named.lo + leaf.bits.named.hi, i + 11u);
    }

    acc ^= q12(frame.scale) + frame.meta.raw[0] * 17u;
    return acc ^ (acc >> 16u);
}
