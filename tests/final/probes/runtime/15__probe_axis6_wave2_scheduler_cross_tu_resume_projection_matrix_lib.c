typedef struct Axis6W2Snapshot {
    unsigned int shard;
    unsigned int phase;
    unsigned int budget;
    unsigned int lane_a;
    unsigned int lane_b;
} Axis6W2Snapshot;

typedef struct Axis6W2Aggregate {
    unsigned int phase[4];
    unsigned int budget[4];
    unsigned int lane_a[4];
    unsigned int lane_b[4];
} Axis6W2Aggregate;

static unsigned int axis6_w2_snapshot_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis6_w2_seed_snapshot(Axis6W2Snapshot* out, unsigned int shard, unsigned int phase, unsigned int budget, unsigned int lane_a, unsigned int lane_b) {
    out->shard = shard % 4u;
    out->phase = phase;
    out->budget = budget;
    out->lane_a = lane_a;
    out->lane_b = lane_b;
}

void axis6_w2_encode_snapshot(const Axis6W2Snapshot* s, unsigned int wire[5]) {
    wire[0] = s->shard ^ 0x1du;
    wire[1] = s->phase ^ 0x3fu;
    wire[2] = s->budget ^ 0x61u;
    wire[3] = s->lane_a ^ 0x83u;
    wire[4] = s->lane_b ^ 0xa5u;
}

void axis6_w2_decode_snapshot(Axis6W2Snapshot* s, const unsigned int wire[5]) {
    s->shard = wire[0] ^ 0x1du;
    s->phase = wire[1] ^ 0x3fu;
    s->budget = wire[2] ^ 0x61u;
    s->lane_a = wire[3] ^ 0x83u;
    s->lane_b = wire[4] ^ 0xa5u;
}

void axis6_w2_clear_aggregate(Axis6W2Aggregate* a) {
    for (int shard = 0; shard < 4; ++shard) {
        a->phase[shard] = 0u;
        a->budget[shard] = 0u;
        a->lane_a[shard] = 0u;
        a->lane_b[shard] = 0u;
    }
}

void axis6_w2_absorb_snapshot(Axis6W2Aggregate* a, const Axis6W2Snapshot* s) {
    unsigned int shard = s->shard % 4u;
    if (s->phase < a->phase[shard]) return;
    if (s->phase > a->phase[shard]) {
        a->phase[shard] = s->phase;
        a->budget[shard] = s->budget;
        a->lane_a[shard] = s->lane_a;
        a->lane_b[shard] = s->lane_b;
        return;
    }
    if (s->budget > a->budget[shard]) {
        a->budget[shard] = s->budget;
        a->lane_a[shard] = s->lane_a;
        a->lane_b[shard] = s->lane_b;
    } else if (s->budget == a->budget[shard]) {
        a->lane_a[shard] += s->lane_a;
        a->lane_b[shard] += s->lane_b;
    }
}

unsigned int axis6_w2_snapshot_signature(const Axis6W2Aggregate* a) {
    unsigned int h = 2166136261u;
    for (int shard = 0; shard < 4; ++shard) {
        h = axis6_w2_snapshot_mix(h, a->phase[shard]);
        h = axis6_w2_snapshot_mix(h, a->budget[shard]);
        h = axis6_w2_snapshot_mix(h, a->lane_a[shard]);
        h = axis6_w2_snapshot_mix(h, a->lane_b[shard]);
    }
    return h;
}
