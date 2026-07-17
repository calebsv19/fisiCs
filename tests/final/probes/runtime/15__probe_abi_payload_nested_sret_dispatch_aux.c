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

struct AbiPayloadFrame abi_payload_relay_frame(struct AbiPayloadFrame in, unsigned step) {
    struct AbiPayloadFrame slots[2];
    unsigned lane = step & 3u;

    slots[0] = in;
    slots[1] = slots[0];
    slots[1].epoch ^= rotl32(step + slots[0].meta.raw[1], (step & 7u) + 3u);
    slots[1].meta.route.slot = (slots[0].meta.route.slot + (int)lane + 1) & 3;
    slots[1].meta.route.salt += step * 313u + slots[1].lanes[lane].bits.named.lo;
    slots[1].lanes[lane].delta += (int)(step & 15u) - 5;
    slots[1].lanes[lane].impulse += (long double)(step & 7u) * 0.01953125L;
    slots[1].lanes[lane].bits.named.mask ^= rotl32(slots[1].epoch + step, lane + 5u);
    slots[1].scale += (long double)(slots[1].meta.raw[0] & 7u) * 0.00390625L;
    return slots[1];
}

struct AbiPayloadFrame abi_payload_permute_frame(struct AbiPayloadFrame in, unsigned step) {
    struct AbiPayloadFrame out = in;
    struct AbiPayloadLeaf scratch[4];
    unsigned i;

    for (i = 0u; i < 4u; ++i) {
        scratch[i] = out.lanes[(i + step) & 3u];
        scratch[i].tag ^= rotl32(step * 19u + i, i + 9u);
        scratch[i].bits.word[(i + 1u) % 3u] += out.meta.raw[i & 1u] ^ (step * (i + 3u));
    }

    for (i = 0u; i < 4u; ++i) {
        out.lanes[i] = scratch[3u - i];
        out.lanes[i].impulse -= (long double)((step + i) & 3u) * 0.01171875L;
    }

    out.epoch += rotl32(out.meta.raw[0] ^ step, 13u);
    out.meta.raw[0] ^= scratch[0].bits.named.mask + step;
    out.meta.raw[1] += scratch[3].tag ^ rotl32(step, 17u);
    out.scale += (long double)(step & 31u) * 0.001953125L;
    return out;
}
