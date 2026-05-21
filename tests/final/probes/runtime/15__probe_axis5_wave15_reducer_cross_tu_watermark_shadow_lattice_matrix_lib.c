typedef struct Axis5W15Snapshot {
    unsigned int shard;
    unsigned int watermark;
    unsigned int shadow;
    unsigned int lane_a;
    unsigned int lane_b;
} Axis5W15Snapshot;

typedef struct Axis5W15Aggregate {
    unsigned int watermark[3];
    unsigned int shadow[3];
    unsigned int lane_a[3];
    unsigned int lane_b[3];
} Axis5W15Aggregate;

static unsigned int axis5_w15_snapshot_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w15_seed_snapshot(
    Axis5W15Snapshot* out,
    unsigned int shard,
    unsigned int watermark,
    unsigned int shadow,
    unsigned int lane_a,
    unsigned int lane_b
) {
    out->shard = shard % 3u;
    out->watermark = watermark;
    out->shadow = shadow;
    out->lane_a = lane_a;
    out->lane_b = lane_b;
}

void axis5_w15_encode_snapshot(const Axis5W15Snapshot* s, unsigned int wire[5]) {
    wire[0] = s->shard ^ 0x21u;
    wire[1] = s->watermark ^ 0x43u;
    wire[2] = s->shadow ^ 0x65u;
    wire[3] = s->lane_a ^ 0x87u;
    wire[4] = s->lane_b ^ 0xa9u;
}

void axis5_w15_decode_snapshot(Axis5W15Snapshot* s, const unsigned int wire[5]) {
    s->shard = wire[0] ^ 0x21u;
    s->watermark = wire[1] ^ 0x43u;
    s->shadow = wire[2] ^ 0x65u;
    s->lane_a = wire[3] ^ 0x87u;
    s->lane_b = wire[4] ^ 0xa9u;
}

void axis5_w15_clear_aggregate(Axis5W15Aggregate* a) {
    for (int shard = 0; shard < 3; ++shard) {
        a->watermark[shard] = 0u;
        a->shadow[shard] = 0u;
        a->lane_a[shard] = 0u;
        a->lane_b[shard] = 0u;
    }
}

void axis5_w15_absorb_snapshot(Axis5W15Aggregate* a, const Axis5W15Snapshot* s) {
    unsigned int shard = s->shard % 3u;
    if (s->watermark < a->watermark[shard]) {
        return;
    }
    if (s->watermark > a->watermark[shard]) {
        a->watermark[shard] = s->watermark;
        a->shadow[shard] = s->shadow;
        a->lane_a[shard] = s->lane_a;
        a->lane_b[shard] = s->lane_b;
        return;
    }
    if (s->shadow > a->shadow[shard]) {
        a->shadow[shard] = s->shadow;
        a->lane_a[shard] = s->lane_a;
        a->lane_b[shard] = s->lane_b;
    } else if (s->shadow == a->shadow[shard]) {
        a->lane_a[shard] += s->lane_a;
        a->lane_b[shard] += s->lane_b;
    }
}

unsigned int axis5_w15_snapshot_signature(const Axis5W15Aggregate* a) {
    unsigned int h = 2166136261u;
    for (int shard = 0; shard < 3; ++shard) {
        h = axis5_w15_snapshot_mix(h, a->watermark[shard]);
        h = axis5_w15_snapshot_mix(h, a->shadow[shard]);
        h = axis5_w15_snapshot_mix(h, a->lane_a[shard]);
        h = axis5_w15_snapshot_mix(h, a->lane_b[shard]);
    }
    return h;
}
