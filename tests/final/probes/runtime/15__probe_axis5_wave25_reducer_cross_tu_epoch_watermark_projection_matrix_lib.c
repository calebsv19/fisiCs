typedef struct Axis5W25Snapshot {
    unsigned int shard;
    unsigned int epoch;
    unsigned int watermark;
    unsigned int lane_a;
    unsigned int lane_b;
} Axis5W25Snapshot;

typedef struct Axis5W25Aggregate {
    unsigned int epoch[4];
    unsigned int watermark[4];
    unsigned int lane_a[4];
    unsigned int lane_b[4];
} Axis5W25Aggregate;

static unsigned int axis5_w25_snapshot_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w25_seed_snapshot(Axis5W25Snapshot* out, unsigned int shard, unsigned int epoch, unsigned int watermark, unsigned int lane_a, unsigned int lane_b) {
    out->shard = shard % 4u;
    out->epoch = epoch;
    out->watermark = watermark;
    out->lane_a = lane_a;
    out->lane_b = lane_b;
}

void axis5_w25_encode_snapshot(const Axis5W25Snapshot* s, unsigned int wire[5]) {
    wire[0] = s->shard ^ 0x19u;
    wire[1] = s->epoch ^ 0x3bu;
    wire[2] = s->watermark ^ 0x5du;
    wire[3] = s->lane_a ^ 0x7fu;
    wire[4] = s->lane_b ^ 0xa1u;
}

void axis5_w25_decode_snapshot(Axis5W25Snapshot* s, const unsigned int wire[5]) {
    s->shard = wire[0] ^ 0x19u;
    s->epoch = wire[1] ^ 0x3bu;
    s->watermark = wire[2] ^ 0x5du;
    s->lane_a = wire[3] ^ 0x7fu;
    s->lane_b = wire[4] ^ 0xa1u;
}

void axis5_w25_clear_aggregate(Axis5W25Aggregate* a) {
    for (int shard = 0; shard < 4; ++shard) {
        a->epoch[shard] = 0u;
        a->watermark[shard] = 0u;
        a->lane_a[shard] = 0u;
        a->lane_b[shard] = 0u;
    }
}

void axis5_w25_absorb_snapshot(Axis5W25Aggregate* a, const Axis5W25Snapshot* s) {
    unsigned int shard = s->shard % 4u;
    if (s->epoch < a->epoch[shard]) return;
    if (s->epoch > a->epoch[shard]) {
        a->epoch[shard] = s->epoch;
        a->watermark[shard] = s->watermark;
        a->lane_a[shard] = s->lane_a;
        a->lane_b[shard] = s->lane_b;
        return;
    }
    if (s->watermark > a->watermark[shard]) {
        a->watermark[shard] = s->watermark;
        a->lane_a[shard] = s->lane_a;
        a->lane_b[shard] = s->lane_b;
    } else if (s->watermark == a->watermark[shard]) {
        a->lane_a[shard] += s->lane_a;
        a->lane_b[shard] += s->lane_b;
    }
}

unsigned int axis5_w25_snapshot_signature(const Axis5W25Aggregate* a) {
    unsigned int h = 2166136261u;
    for (int shard = 0; shard < 4; ++shard) {
        h = axis5_w25_snapshot_mix(h, a->epoch[shard]);
        h = axis5_w25_snapshot_mix(h, a->watermark[shard]);
        h = axis5_w25_snapshot_mix(h, a->lane_a[shard]);
        h = axis5_w25_snapshot_mix(h, a->lane_b[shard]);
    }
    return h;
}
