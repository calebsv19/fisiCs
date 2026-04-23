typedef struct Axis5W2Accum {
    unsigned int sum0;
    unsigned int sum1;
    unsigned int xormix;
} Axis5W2Accum;

static void axis5_w2_accum_reset(Axis5W2Accum* a) {
    a->sum0 = 0u;
    a->sum1 = 0u;
    a->xormix = 0u;
}

void axis5_w2_accum_update(Axis5W2Accum* a, unsigned int lane, unsigned int value) {
    if ((lane & 1u) == 0u) a->sum0 += (value & 0xffffu);
    else a->sum1 += (value & 0xffffu);
    a->xormix ^= ((lane & 15u) << 12) ^ (value & 0x0fffu);
}

void axis5_w2_accum_fold_segment(
    Axis5W2Accum* out,
    const unsigned int* lanes,
    const unsigned int* values,
    int n
) {
    axis5_w2_accum_reset(out);
    for (int i = 0; i < n; ++i) axis5_w2_accum_update(out, lanes[i], values[i]);
}

void axis5_w2_accum_merge(Axis5W2Accum* dst, const Axis5W2Accum* src) {
    dst->sum0 += src->sum0;
    dst->sum1 += src->sum1;
    dst->xormix ^= src->xormix;
}

unsigned int axis5_w2_accum_signature(const Axis5W2Accum* a) {
    unsigned int h = 0x811c9dc5u;
    h ^= a->sum0 + 0x9e3779b9u + (h << 6) + (h >> 2);
    h ^= a->sum1 + 0x9e3779b9u + (h << 6) + (h >> 2);
    h ^= a->xormix + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}
