typedef struct Axis5W10Snapshot {
    unsigned int shard;
    unsigned int epoch;
    unsigned int bins[3];
    unsigned int token;
} Axis5W10Snapshot;

typedef struct Axis5W10Aggregate {
    unsigned int epoch_by_shard[2];
    unsigned int bins_by_shard[2][3];
    unsigned int token_mix;
} Axis5W10Aggregate;

static unsigned int axis5_w10_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w10_seed_snapshot(
    Axis5W10Snapshot* out,
    unsigned int shard,
    unsigned int epoch,
    unsigned int a,
    unsigned int b,
    unsigned int c
) {
    out->shard = shard & 1u;
    out->epoch = epoch;
    out->bins[0] = a;
    out->bins[1] = b;
    out->bins[2] = c;
    out->token = ((shard & 1u) << 12) ^ ((epoch & 0xffu) << 4) ^ ((a + b + c) & 0x0fu);
}

void axis5_w10_encode_snapshot(const Axis5W10Snapshot* s, unsigned int wire[6]) {
    wire[0] = s->shard ^ 0x51u;
    wire[1] = s->epoch ^ 0xA2u;
    wire[2] = s->bins[0] ^ 0x31u;
    wire[3] = s->bins[1] ^ 0x42u;
    wire[4] = s->bins[2] ^ 0x53u;
    wire[5] = s->token ^ 0x64u;
}

void axis5_w10_decode_snapshot(Axis5W10Snapshot* s, const unsigned int wire[6]) {
    s->shard = wire[0] ^ 0x51u;
    s->epoch = wire[1] ^ 0xA2u;
    s->bins[0] = wire[2] ^ 0x31u;
    s->bins[1] = wire[3] ^ 0x42u;
    s->bins[2] = wire[4] ^ 0x53u;
    s->token = wire[5] ^ 0x64u;
}

void axis5_w10_clear_aggregate(Axis5W10Aggregate* a) {
    for (int shard = 0; shard < 2; ++shard) {
        a->epoch_by_shard[shard] = 0u;
        for (int i = 0; i < 3; ++i) {
            a->bins_by_shard[shard][i] = 0u;
        }
    }
    a->token_mix = 0u;
}

void axis5_w10_absorb_snapshot(Axis5W10Aggregate* a, const Axis5W10Snapshot* s) {
    unsigned int shard = s->shard & 1u;
    if (s->epoch < a->epoch_by_shard[shard]) {
        return;
    }
    a->epoch_by_shard[shard] = s->epoch;
    for (int i = 0; i < 3; ++i) {
        a->bins_by_shard[shard][i] = s->bins[i];
    }
}

unsigned int axis5_w10_aggregate_signature(const Axis5W10Aggregate* a) {
    unsigned int h = 2166136261u;
    for (int shard = 0; shard < 2; ++shard) {
        h = axis5_w10_mix(h, a->epoch_by_shard[shard]);
        for (int i = 0; i < 3; ++i) {
            h = axis5_w10_mix(h, a->bins_by_shard[shard][i]);
        }
    }
    return h;
}
