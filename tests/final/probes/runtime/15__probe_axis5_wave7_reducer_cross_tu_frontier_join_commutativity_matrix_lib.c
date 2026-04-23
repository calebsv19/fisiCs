typedef struct Axis5W7Accum {
    unsigned int bins[4];
    unsigned int xor_mix;
    unsigned int token;
} Axis5W7Accum;

static unsigned int axis5_w7_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w7_fold_segment(
    Axis5W7Accum* out,
    const unsigned int* lanes,
    const unsigned int* values,
    int n
) {
    for (int i = 0; i < 4; ++i) out->bins[i] = 0u;
    out->xor_mix = 0u;
    out->token = 0u;
    for (int i = 0; i < n; ++i) {
        unsigned int lane = lanes[i] & 3u;
        unsigned int val = values[i] & 0xffffu;
        out->bins[lane] += val;
        out->xor_mix ^= ((lane & 3u) << 12) ^ (val & 0x0fffu);
        out->token ^= ((lane & 3u) << 8) ^ (val & 0xffu) ^ 0x5a5au;
    }
}

void axis5_w7_encode_frontier(const Axis5W7Accum* a, unsigned int wire[6]) {
    for (int i = 0; i < 4; ++i) wire[i] = a->bins[i] ^ (0xA590u + (unsigned int)i);
    wire[4] = a->xor_mix ^ 0x5AA5u;
    wire[5] = a->token ^ 0x33CCu;
}

void axis5_w7_decode_frontier(Axis5W7Accum* a, const unsigned int wire[6]) {
    for (int i = 0; i < 4; ++i) a->bins[i] = wire[i] ^ (0xA590u + (unsigned int)i);
    a->xor_mix = wire[4] ^ 0x5AA5u;
    a->token = wire[5] ^ 0x33CCu;
}

void axis5_w7_merge(Axis5W7Accum* dst, const Axis5W7Accum* src) {
    for (int i = 0; i < 4; ++i) dst->bins[i] += src->bins[i];
    dst->xor_mix ^= src->xor_mix;
    dst->token ^= src->token;
}

unsigned int axis5_w7_signature(const Axis5W7Accum* a) {
    unsigned int h = 0x811c9dc5u ^ a->token;
    for (int i = 0; i < 4; ++i) h = axis5_w7_mix(h, a->bins[i]);
    h = axis5_w7_mix(h, a->xor_mix);
    return h;
}
