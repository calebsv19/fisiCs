struct Bucket10TraceB {
    unsigned left;
    unsigned right;
};

static struct Bucket10TraceB lane[2];

unsigned bucket10_trace_b_seed(unsigned seed) {
    lane[0].left = seed + 4u;
    lane[0].right = seed * 2u + 3u;
    lane[1].left = seed + 9u;
    lane[1].right = seed * 3u + 5u;
    return lane[0].left ^ lane[1].right;
}

unsigned bucket10_trace_b_mix(unsigned acc, unsigned slot, unsigned value) {
    unsigned idx = slot & 1u;
    lane[idx].left += value * 3u + slot;
    lane[idx].right ^= value + slot * 11u;
    return (acc + lane[idx].left) ^ lane[idx ^ 1u].right;
}

unsigned bucket10_trace_b_peek(void) {
    return lane[0].left + lane[0].right + lane[1].left + lane[1].right;
}
