typedef struct Axis5W9Accum {
    unsigned int bins[4];
    unsigned int xormix;
    unsigned int token;
} Axis5W9Accum;

static unsigned int axis5_w9_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w9_fold_segment(
    Axis5W9Accum* out,
    const unsigned int* lanes,
    const unsigned int* values,
    int n
) {
    for (int i = 0; i < 4; ++i) out->bins[i] = 0u;
    out->xormix = 0u;
    out->token = 0u;
    for (int i = 0; i < n; ++i) {
        unsigned int lane = lanes[i] & 3u;
        unsigned int val = values[i] & 0xffffu;
        out->bins[lane] += val;
        out->xormix ^= ((lane & 3u) << 12) ^ (val & 0x0fffu);
        out->token ^= ((lane & 3u) << 8) ^ (val & 0xffu) ^ 0x7c7cu;
    }
}

void axis5_w9_encode_checkpoint(const Axis5W9Accum* a, unsigned int wire[6]) {
    for (int i = 0; i < 4; ++i) wire[i] = a->bins[i] ^ (0xA740u + (unsigned int)i);
    wire[4] = a->xormix ^ 0x5AA5u;
    wire[5] = a->token ^ 0x33CCu;
}

void axis5_w9_decode_checkpoint(Axis5W9Accum* a, const unsigned int wire[6]) {
    for (int i = 0; i < 4; ++i) a->bins[i] = wire[i] ^ (0xA740u + (unsigned int)i);
    a->xormix = wire[4] ^ 0x5AA5u;
    a->token = wire[5] ^ 0x33CCu;
}

void axis5_w9_merge(Axis5W9Accum* dst, const Axis5W9Accum* src) {
    for (int i = 0; i < 4; ++i) dst->bins[i] += src->bins[i];
    dst->xormix ^= src->xormix;
    dst->token ^= src->token;
}

unsigned int axis5_w9_signature(const Axis5W9Accum* a) {
    unsigned int h = 0x811c9dc5u ^ a->token;
    for (int i = 0; i < 4; ++i) h = axis5_w9_mix(h, a->bins[i]);
    h = axis5_w9_mix(h, a->xormix);
    return h;
}
