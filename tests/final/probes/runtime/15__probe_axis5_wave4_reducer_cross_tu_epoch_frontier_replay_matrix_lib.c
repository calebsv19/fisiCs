typedef struct Axis5W4State {
    unsigned int bins[5];
    unsigned int epoch;
    unsigned int salt;
} Axis5W4State;

typedef struct Axis5W4Frontier {
    unsigned int bins[5];
    unsigned int epoch;
    unsigned int token;
} Axis5W4Frontier;

static unsigned int axis5_w4_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w4_state_init(Axis5W4State* s, unsigned int salt) {
    for (int i = 0; i < 5; ++i) s->bins[i] = 0u;
    s->epoch = 0u;
    s->salt = salt;
}

static void axis5_w4_state_apply(Axis5W4State* s, unsigned int lane, unsigned int value) {
    unsigned int idx = lane % 5u;
    unsigned int mixed = (value & 0xffu) ^ ((lane & 31u) << 2) ^ (s->epoch & 0x1fu);
    s->bins[idx] += mixed;
}

void axis5_w4_state_apply_batch(
    Axis5W4State* s,
    const unsigned int* lanes,
    const unsigned int* values,
    const unsigned int* fences,
    int n
) {
    for (int i = 0; i < n; ++i) {
        axis5_w4_state_apply(s, lanes[i], values[i]);
        if (fences[i] != 0u) {
            s->epoch += 1u;
            s->salt = axis5_w4_mix(s->salt, 0xA5A5u ^ s->epoch);
        }
    }
}

void axis5_w4_state_export_frontier(const Axis5W4State* s, Axis5W4Frontier* out) {
    for (int i = 0; i < 5; ++i) out->bins[i] = s->bins[i];
    out->epoch = s->epoch;
    out->token = s->salt ^ (0x5A5Au + s->epoch);
}

void axis5_w4_state_import_frontier(Axis5W4State* s, const Axis5W4Frontier* in) {
    for (int i = 0; i < 5; ++i) s->bins[i] = in->bins[i];
    s->epoch = in->epoch;
    s->salt = in->token ^ (0x5A5Au + in->epoch);
}

unsigned int axis5_w4_state_signature(const Axis5W4State* s) {
    unsigned int h = 0x811c9dc5u ^ s->salt ^ (s->epoch << 8);
    for (int i = 0; i < 5; ++i) {
        h = axis5_w4_mix(h, s->bins[i]);
    }
    return h;
}
