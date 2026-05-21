typedef struct Axis6W3Snapshot {
    unsigned int shard;
    unsigned int checkpoint;
    unsigned int budget;
    unsigned int lane_a;
    unsigned int lane_b;
} Axis6W3Snapshot;

typedef struct Axis6W3Aggregate {
    unsigned int checkpoint[4];
    unsigned int budget[4];
    unsigned int lane_a[4];
    unsigned int lane_b[4];
} Axis6W3Aggregate;

static unsigned int axis6_w3_snapshot_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis6_w3_seed_snapshot(Axis6W3Snapshot* out, unsigned int shard, unsigned int checkpoint, unsigned int budget, unsigned int lane_a, unsigned int lane_b) {
    out->shard = shard % 4u;
    out->checkpoint = checkpoint;
    out->budget = budget;
    out->lane_a = lane_a;
    out->lane_b = lane_b;
}

void axis6_w3_encode_snapshot(const Axis6W3Snapshot* s, unsigned int wire[5]) {
    wire[0] = s->shard ^ 0x1fu;
    wire[1] = s->checkpoint ^ 0x41u;
    wire[2] = s->budget ^ 0x63u;
    wire[3] = s->lane_a ^ 0x85u;
    wire[4] = s->lane_b ^ 0xa7u;
}

void axis6_w3_decode_snapshot(Axis6W3Snapshot* s, const unsigned int wire[5]) {
    s->shard = wire[0] ^ 0x1fu;
    s->checkpoint = wire[1] ^ 0x41u;
    s->budget = wire[2] ^ 0x63u;
    s->lane_a = wire[3] ^ 0x85u;
    s->lane_b = wire[4] ^ 0xa7u;
}

void axis6_w3_clear_aggregate(Axis6W3Aggregate* a) {
    for (int shard = 0; shard < 4; ++shard) {
        a->checkpoint[shard] = 0u;
        a->budget[shard] = 0u;
        a->lane_a[shard] = 0u;
        a->lane_b[shard] = 0u;
    }
}

void axis6_w3_absorb_snapshot(Axis6W3Aggregate* a, const Axis6W3Snapshot* s) {
    unsigned int shard = s->shard % 4u;
    if (s->checkpoint < a->checkpoint[shard]) return;
    if (s->checkpoint > a->checkpoint[shard]) {
        a->checkpoint[shard] = s->checkpoint;
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

unsigned int axis6_w3_snapshot_signature(const Axis6W3Aggregate* a) {
    unsigned int h = 2166136261u;
    for (int shard = 0; shard < 4; ++shard) {
        h = axis6_w3_snapshot_mix(h, a->checkpoint[shard]);
        h = axis6_w3_snapshot_mix(h, a->budget[shard]);
        h = axis6_w3_snapshot_mix(h, a->lane_a[shard]);
        h = axis6_w3_snapshot_mix(h, a->lane_b[shard]);
    }
    return h;
}
