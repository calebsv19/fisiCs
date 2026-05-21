typedef struct Axis5W19Snapshot {
    unsigned int shard;
    unsigned int checkpoint;
    unsigned int watermark;
    unsigned int lane_a;
    unsigned int lane_b;
} Axis5W19Snapshot;

typedef struct Axis5W19Aggregate {
    unsigned int checkpoint[3];
    unsigned int watermark[3];
    unsigned int lane_a[3];
    unsigned int lane_b[3];
} Axis5W19Aggregate;

static unsigned int axis5_w19_snapshot_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w19_seed_snapshot(
    Axis5W19Snapshot* out,
    unsigned int shard,
    unsigned int checkpoint,
    unsigned int watermark,
    unsigned int lane_a,
    unsigned int lane_b
) {
    out->shard = shard % 3u;
    out->checkpoint = checkpoint;
    out->watermark = watermark;
    out->lane_a = lane_a;
    out->lane_b = lane_b;
}

void axis5_w19_encode_snapshot(const Axis5W19Snapshot* s, unsigned int wire[5]) {
    wire[0] = s->shard ^ 0x1bu;
    wire[1] = s->checkpoint ^ 0x35u;
    wire[2] = s->watermark ^ 0x57u;
    wire[3] = s->lane_a ^ 0x71u;
    wire[4] = s->lane_b ^ 0x9bu;
}

void axis5_w19_decode_snapshot(Axis5W19Snapshot* s, const unsigned int wire[5]) {
    s->shard = wire[0] ^ 0x1bu;
    s->checkpoint = wire[1] ^ 0x35u;
    s->watermark = wire[2] ^ 0x57u;
    s->lane_a = wire[3] ^ 0x71u;
    s->lane_b = wire[4] ^ 0x9bu;
}

void axis5_w19_clear_aggregate(Axis5W19Aggregate* a) {
    for (int shard = 0; shard < 3; ++shard) {
        a->checkpoint[shard] = 0u;
        a->watermark[shard] = 0u;
        a->lane_a[shard] = 0u;
        a->lane_b[shard] = 0u;
    }
}

void axis5_w19_absorb_snapshot(Axis5W19Aggregate* a, const Axis5W19Snapshot* s) {
    unsigned int shard = s->shard % 3u;
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

unsigned int axis5_w19_snapshot_signature(const Axis5W19Aggregate* a) {
    unsigned int h = 2166136261u;
    for (int shard = 0; shard < 3; ++shard) {
        h = axis5_w19_snapshot_mix(h, a->checkpoint[shard]);
        h = axis5_w19_snapshot_mix(h, a->watermark[shard]);
        h = axis5_w19_snapshot_mix(h, a->lane_a[shard]);
        h = axis5_w19_snapshot_mix(h, a->lane_b[shard]);
    }
    return h;
}
