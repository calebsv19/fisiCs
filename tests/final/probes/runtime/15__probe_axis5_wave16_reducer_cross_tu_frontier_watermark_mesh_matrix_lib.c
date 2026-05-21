typedef struct Axis5W16Snapshot {
    unsigned int shard;
    unsigned int frontier;
    unsigned int watermark;
    unsigned int lane_a;
    unsigned int lane_b;
} Axis5W16Snapshot;

typedef struct Axis5W16Aggregate {
    unsigned int frontier[3];
    unsigned int watermark[3];
    unsigned int lane_a[3];
    unsigned int lane_b[3];
} Axis5W16Aggregate;

static unsigned int axis5_w16_snapshot_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w16_seed_snapshot(
    Axis5W16Snapshot* out,
    unsigned int shard,
    unsigned int frontier,
    unsigned int watermark,
    unsigned int lane_a,
    unsigned int lane_b
) {
    out->shard = shard % 3u;
    out->frontier = frontier;
    out->watermark = watermark;
    out->lane_a = lane_a;
    out->lane_b = lane_b;
}

void axis5_w16_encode_snapshot(const Axis5W16Snapshot* s, unsigned int wire[5]) {
    wire[0] = s->shard ^ 0x31u;
    wire[1] = s->frontier ^ 0x53u;
    wire[2] = s->watermark ^ 0x75u;
    wire[3] = s->lane_a ^ 0x97u;
    wire[4] = s->lane_b ^ 0xb9u;
}

void axis5_w16_decode_snapshot(Axis5W16Snapshot* s, const unsigned int wire[5]) {
    s->shard = wire[0] ^ 0x31u;
    s->frontier = wire[1] ^ 0x53u;
    s->watermark = wire[2] ^ 0x75u;
    s->lane_a = wire[3] ^ 0x97u;
    s->lane_b = wire[4] ^ 0xb9u;
}

void axis5_w16_clear_aggregate(Axis5W16Aggregate* a) {
    for (int shard = 0; shard < 3; ++shard) {
        a->frontier[shard] = 0u;
        a->watermark[shard] = 0u;
        a->lane_a[shard] = 0u;
        a->lane_b[shard] = 0u;
    }
}

void axis5_w16_absorb_snapshot(Axis5W16Aggregate* a, const Axis5W16Snapshot* s) {
    unsigned int shard = s->shard % 3u;
    if (s->frontier < a->frontier[shard]) {
        return;
    }
    if (s->frontier > a->frontier[shard]) {
        a->frontier[shard] = s->frontier;
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

unsigned int axis5_w16_snapshot_signature(const Axis5W16Aggregate* a) {
    unsigned int h = 2166136261u;
    for (int shard = 0; shard < 3; ++shard) {
        h = axis5_w16_snapshot_mix(h, a->frontier[shard]);
        h = axis5_w16_snapshot_mix(h, a->watermark[shard]);
        h = axis5_w16_snapshot_mix(h, a->lane_a[shard]);
        h = axis5_w16_snapshot_mix(h, a->lane_b[shard]);
    }
    return h;
}
