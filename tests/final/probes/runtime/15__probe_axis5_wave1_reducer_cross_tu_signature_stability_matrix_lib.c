typedef struct Axis5W1Pair {
    unsigned int op;
    int value;
} Axis5W1Pair;

static unsigned int axis5_w1_lib_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 5) + (h >> 2);
    return h;
}

unsigned int axis5_w1_lib_signature(const Axis5W1Pair* seq, int n, unsigned int salt) {
    unsigned int h = 0x811c9dc5u ^ salt;
    for (int i = 0; i < n; ++i) {
        unsigned int op = seq[i].op & 31u;
        unsigned int val = (unsigned int)(seq[i].value & 0xffff);
        h = axis5_w1_lib_mix(h, op);
        h = axis5_w1_lib_mix(h, val);
    }
    return h;
}

unsigned int axis5_w1_lib_partitioned_signature(
    const Axis5W1Pair* left,
    int n_left,
    const Axis5W1Pair* right,
    int n_right,
    unsigned int salt
) {
    unsigned int h = axis5_w1_lib_signature(left, n_left, salt);
    return axis5_w1_lib_signature(right, n_right, h ^ 0x27d4eb2du);
}
