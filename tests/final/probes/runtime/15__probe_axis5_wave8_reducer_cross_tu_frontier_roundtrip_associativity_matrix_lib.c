typedef struct Axis5W8Accum {
    unsigned int lane_sum[4];
    unsigned int xor_mix;
    unsigned int token;
} Axis5W8Accum;

static unsigned int axis5_w8_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w8_fold_segment(
    Axis5W8Accum* out,
    const unsigned int* lanes,
    const unsigned int* values,
    int n
) {
    for (int i = 0; i < 4; ++i) out->lane_sum[i] = 0u;
    out->xor_mix = 0u;
    out->token = 0u;
    for (int i = 0; i < n; ++i) {
        unsigned int lane = lanes[i] & 3u;
        unsigned int val = values[i] & 0xffffu;
        out->lane_sum[lane] += val;
        out->xor_mix ^= ((lane & 3u) << 12) ^ (val & 0x0fffu);
        out->token ^= ((lane & 3u) << 8) ^ (val & 0xffu) ^ 0x6b6bu;
    }
}

void axis5_w8_encode_frontier(const Axis5W8Accum* a, unsigned int wire[6]) {
    for (int i = 0; i < 4; ++i) wire[i] = a->lane_sum[i] ^ (0xA6D0u + (unsigned int)i);
    wire[4] = a->xor_mix ^ 0x5AA5u;
    wire[5] = a->token ^ 0x33CCu;
}

void axis5_w8_decode_frontier(Axis5W8Accum* a, const unsigned int wire[6]) {
    for (int i = 0; i < 4; ++i) a->lane_sum[i] = wire[i] ^ (0xA6D0u + (unsigned int)i);
    a->xor_mix = wire[4] ^ 0x5AA5u;
    a->token = wire[5] ^ 0x33CCu;
}

void axis5_w8_merge(Axis5W8Accum* dst, const Axis5W8Accum* src) {
    for (int i = 0; i < 4; ++i) dst->lane_sum[i] += src->lane_sum[i];
    dst->xor_mix ^= src->xor_mix;
    dst->token ^= src->token;
}

unsigned int axis5_w8_signature(const Axis5W8Accum* a) {
    unsigned int h = 0x811c9dc5u ^ a->token;
    for (int i = 0; i < 4; ++i) h = axis5_w8_mix(h, a->lane_sum[i]);
    h = axis5_w8_mix(h, a->xor_mix);
    return h;
}
