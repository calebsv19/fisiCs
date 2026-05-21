typedef struct Axis5W21Snapshot {
    unsigned int shard;
    unsigned int checkpoint;
    unsigned int watermark;
    unsigned int lane_a;
    unsigned int lane_b;
} Axis5W21Snapshot;

typedef struct Axis5W21Aggregate {
    unsigned int checkpoint[4];
    unsigned int watermark[4];
    unsigned int lane_a[4];
    unsigned int lane_b[4];
} Axis5W21Aggregate;

static unsigned int axis5_w21_snapshot_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w21_seed_snapshot(
    Axis5W21Snapshot* out,
    unsigned int shard,
    unsigned int checkpoint,
    unsigned int watermark,
    unsigned int lane_a,
    unsigned int lane_b
) {
    out->shard = shard % 4u;
    out->checkpoint = checkpoint;
    out->watermark = watermark;
    out->lane_a = lane_a;
    out->lane_b = lane_b;
}

void axis5_w21_encode_snapshot(const Axis5W21Snapshot* s, unsigned int wire[5]) {
    wire[0] = s->shard ^ 0x13u;
    wire[1] = s->checkpoint ^ 0x37u;
    wire[2] = s->watermark ^ 0x59u;
    wire[3] = s->lane_a ^ 0x7bu;
    wire[4] = s->lane_b ^ 0x9du;
}

void axis5_w21_decode_snapshot(Axis5W21Snapshot* s, const unsigned int wire[5]) {
    s->shard = wire[0] ^ 0x13u;
    s->checkpoint = wire[1] ^ 0x37u;
    s->watermark = wire[2] ^ 0x59u;
    s->lane_a = wire[3] ^ 0x7bu;
    s->lane_b = wire[4] ^ 0x9du;
}

void axis5_w21_clear_aggregate(Axis5W21Aggregate* a) {
    for (int shard = 0; shard < 4; ++shard) {
        a->checkpoint[shard] = 0u;
        a->watermark[shard] = 0u;
        a->lane_a[shard] = 0u;
        a->lane_b[shard] = 0u;
    }
}

void axis5_w21_absorb_snapshot(Axis5W21Aggregate* a, const Axis5W21Snapshot* s) {
    unsigned int shard = s->shard % 4u;
    if (s->checkpoint < a->checkpoint[shard]) {
        return;
    }
    if (s->checkpoint > a->checkpoint[shard]) {
        a->checkpoint[shard] = s->checkpoint;
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

unsigned int axis5_w21_snapshot_signature(const Axis5W21Aggregate* a) {
    unsigned int h = 2166136261u;
    for (int shard = 0; shard < 4; ++shard) {
        h = axis5_w21_snapshot_mix(h, a->checkpoint[shard]);
        h = axis5_w21_snapshot_mix(h, a->watermark[shard]);
        h = axis5_w21_snapshot_mix(h, a->lane_a[shard]);
        h = axis5_w21_snapshot_mix(h, a->lane_b[shard]);
    }
    return h;
}
