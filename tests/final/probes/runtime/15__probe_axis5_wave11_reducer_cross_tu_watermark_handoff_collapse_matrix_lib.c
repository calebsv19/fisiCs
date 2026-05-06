typedef struct Axis5W11Snapshot {
    unsigned int shard;
    unsigned int watermark;
    unsigned int epoch;
    unsigned int bins[2];
} Axis5W11Snapshot;

typedef struct Axis5W11Aggregate {
    unsigned int watermark_by_shard[2];
    unsigned int epoch_by_shard[2];
    unsigned int bins_by_shard[2][2];
} Axis5W11Aggregate;

static unsigned int axis5_w11_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w11_seed_snapshot(
    Axis5W11Snapshot* out,
    unsigned int shard,
    unsigned int watermark,
    unsigned int epoch,
    unsigned int a,
    unsigned int b
) {
    out->shard = shard & 1u;
    out->watermark = watermark;
    out->epoch = epoch;
    out->bins[0] = a;
    out->bins[1] = b;
}

void axis5_w11_encode_snapshot(const Axis5W11Snapshot* s, unsigned int wire[5]) {
    wire[0] = s->shard ^ 0x31u;
    wire[1] = s->watermark ^ 0x42u;
    wire[2] = s->epoch ^ 0x53u;
    wire[3] = s->bins[0] ^ 0x64u;
    wire[4] = s->bins[1] ^ 0x75u;
}

void axis5_w11_decode_snapshot(Axis5W11Snapshot* s, const unsigned int wire[5]) {
    s->shard = wire[0] ^ 0x31u;
    s->watermark = wire[1] ^ 0x42u;
    s->epoch = wire[2] ^ 0x53u;
    s->bins[0] = wire[3] ^ 0x64u;
    s->bins[1] = wire[4] ^ 0x75u;
}

void axis5_w11_clear_aggregate(Axis5W11Aggregate* a) {
    for (int shard = 0; shard < 2; ++shard) {
        a->watermark_by_shard[shard] = 0u;
        a->epoch_by_shard[shard] = 0u;
        for (int i = 0; i < 2; ++i) {
            a->bins_by_shard[shard][i] = 0u;
        }
    }
}

void axis5_w11_absorb_snapshot(Axis5W11Aggregate* a, const Axis5W11Snapshot* s) {
    unsigned int shard = s->shard & 1u;
    if (s->watermark < a->watermark_by_shard[shard]) {
        return;
    }
    if (s->watermark == a->watermark_by_shard[shard] &&
        s->epoch < a->epoch_by_shard[shard]) {
        return;
    }
    a->watermark_by_shard[shard] = s->watermark;
    a->epoch_by_shard[shard] = s->epoch;
    a->bins_by_shard[shard][0] = s->bins[0];
    a->bins_by_shard[shard][1] = s->bins[1];
}

unsigned int axis5_w11_signature(const Axis5W11Aggregate* a) {
    unsigned int h = 2166136261u;
    for (int shard = 0; shard < 2; ++shard) {
        h = axis5_w11_mix(h, a->watermark_by_shard[shard]);
        h = axis5_w11_mix(h, a->epoch_by_shard[shard]);
        h = axis5_w11_mix(h, a->bins_by_shard[shard][0]);
        h = axis5_w11_mix(h, a->bins_by_shard[shard][1]);
    }
    return h;
}
