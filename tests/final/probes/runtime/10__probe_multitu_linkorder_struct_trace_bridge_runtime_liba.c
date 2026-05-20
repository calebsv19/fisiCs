struct Bucket10TraceA {
    int left;
    int right;
};

static struct Bucket10TraceA lane[2];

int bucket10_trace_a_seed(int seed) {
    lane[0].left = seed + 2;
    lane[0].right = seed + 5;
    lane[1].left = seed + 7;
    lane[1].right = seed + 11;
    return lane[0].left + lane[1].right;
}

int bucket10_trace_a_push(int slot, int value) {
    int idx = slot & 1;
    lane[idx].left = lane[idx].left * 2 + value + idx;
    lane[idx].right += value - idx;
    return lane[idx].left + lane[idx].right;
}

int bucket10_trace_a_peek(void) {
    return lane[0].left + lane[0].right - lane[1].left + lane[1].right;
}
