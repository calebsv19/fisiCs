typedef struct Axis5W6Accum {
    unsigned int lane_sum[4];
    unsigned int xormix;
    unsigned int token;
} Axis5W6Accum;

static unsigned int axis5_w6_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w6_accum_fold_segment(
    Axis5W6Accum* out,
    const unsigned int* lanes,
    const unsigned int* values,
    int n,
    unsigned int seed
) {
    for (int i = 0; i < 4; ++i) out->lane_sum[i] = 0u;
    out->xormix = 0u;
    out->token = seed;
    for (int i = 0; i < n; ++i) {
        unsigned int lane = lanes[i] & 3u;
        unsigned int val = values[i] & 0xffffu;
        out->lane_sum[lane] += val;
        out->xormix ^= ((lane & 3u) << 12) ^ (val & 0x0fffu);
        out->token ^= ((lane << 8) ^ (val & 0xffu) ^ 0x5a5au);
    }
}

void axis5_w6_accum_encode(const Axis5W6Accum* a, unsigned int wire[6]) {
    for (int i = 0; i < 4; ++i) wire[i] = a->lane_sum[i] ^ (0xA550u + (unsigned int)i);
    wire[4] = a->xormix ^ 0x5AA5u;
    wire[5] = a->token ^ 0x33CCu;
}

void axis5_w6_accum_decode(Axis5W6Accum* a, const unsigned int wire[6]) {
    for (int i = 0; i < 4; ++i) a->lane_sum[i] = wire[i] ^ (0xA550u + (unsigned int)i);
    a->xormix = wire[4] ^ 0x5AA5u;
    a->token = wire[5] ^ 0x33CCu;
}

void axis5_w6_accum_merge(Axis5W6Accum* dst, const Axis5W6Accum* src) {
    for (int i = 0; i < 4; ++i) dst->lane_sum[i] += src->lane_sum[i];
    dst->xormix ^= src->xormix;
    dst->token ^= src->token;
}

unsigned int axis5_w6_accum_signature(const Axis5W6Accum* a) {
    unsigned int h = 0x811c9dc5u ^ a->token;
    for (int i = 0; i < 4; ++i) h = axis5_w6_mix(h, a->lane_sum[i]);
    h = axis5_w6_mix(h, a->xormix);
    return h;
}
