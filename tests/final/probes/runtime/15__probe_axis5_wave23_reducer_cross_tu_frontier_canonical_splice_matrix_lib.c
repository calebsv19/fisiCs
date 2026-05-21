typedef struct Axis5W23Snapshot {
    unsigned int shard;
    unsigned int frontier;
    unsigned int checkpoint;
    unsigned int lane_a;
    unsigned int lane_b;
} Axis5W23Snapshot;

typedef struct Axis5W23Aggregate {
    unsigned int frontier[4];
    unsigned int checkpoint[4];
    unsigned int lane_a[4];
    unsigned int lane_b[4];
} Axis5W23Aggregate;

static unsigned int axis5_w23_snapshot_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w23_seed_snapshot(Axis5W23Snapshot* out, unsigned int shard, unsigned int frontier, unsigned int checkpoint, unsigned int lane_a, unsigned int lane_b) {
    out->shard = shard % 4u;
    out->frontier = frontier;
    out->checkpoint = checkpoint;
    out->lane_a = lane_a;
    out->lane_b = lane_b;
}

void axis5_w23_encode_snapshot(const Axis5W23Snapshot* s, unsigned int wire[5]) {
    wire[0] = s->shard ^ 0x15u;
    wire[1] = s->frontier ^ 0x35u;
    wire[2] = s->checkpoint ^ 0x57u;
    wire[3] = s->lane_a ^ 0x79u;
    wire[4] = s->lane_b ^ 0x9bu;
}

void axis5_w23_decode_snapshot(Axis5W23Snapshot* s, const unsigned int wire[5]) {
    s->shard = wire[0] ^ 0x15u;
    s->frontier = wire[1] ^ 0x35u;
    s->checkpoint = wire[2] ^ 0x57u;
    s->lane_a = wire[3] ^ 0x79u;
    s->lane_b = wire[4] ^ 0x9bu;
}

void axis5_w23_clear_aggregate(Axis5W23Aggregate* a) {
    for (int shard = 0; shard < 4; ++shard) {
        a->frontier[shard] = 0u;
        a->checkpoint[shard] = 0u;
        a->lane_a[shard] = 0u;
        a->lane_b[shard] = 0u;
    }
}

void axis5_w23_absorb_snapshot(Axis5W23Aggregate* a, const Axis5W23Snapshot* s) {
    unsigned int shard = s->shard % 4u;
    if (s->frontier < a->frontier[shard]) return;
    if (s->frontier > a->frontier[shard]) {
        a->frontier[shard] = s->frontier;
        a->checkpoint[shard] = s->checkpoint;
        a->lane_a[shard] = s->lane_a;
        a->lane_b[shard] = s->lane_b;
        return;
    }
    if (s->checkpoint > a->checkpoint[shard]) {
        a->checkpoint[shard] = s->checkpoint;
        a->lane_a[shard] = s->lane_a;
        a->lane_b[shard] = s->lane_b;
    } else if (s->checkpoint == a->checkpoint[shard]) {
        a->lane_a[shard] += s->lane_a;
        a->lane_b[shard] += s->lane_b;
    }
}

unsigned int axis5_w23_snapshot_signature(const Axis5W23Aggregate* a) {
    unsigned int h = 2166136261u;
    for (int shard = 0; shard < 4; ++shard) {
        h = axis5_w23_snapshot_mix(h, a->frontier[shard]);
        h = axis5_w23_snapshot_mix(h, a->checkpoint[shard]);
        h = axis5_w23_snapshot_mix(h, a->lane_a[shard]);
        h = axis5_w23_snapshot_mix(h, a->lane_b[shard]);
    }
    return h;
}
