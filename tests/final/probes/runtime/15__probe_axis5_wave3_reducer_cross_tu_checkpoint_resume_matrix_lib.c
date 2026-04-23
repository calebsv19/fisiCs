typedef struct Axis5W3State {
    unsigned int bins[6];
    unsigned int salt;
} Axis5W3State;

void axis5_w3_state_init(Axis5W3State* s, unsigned int salt) {
    for (int i = 0; i < 6; ++i) s->bins[i] = 0u;
    s->salt = salt;
}

void axis5_w3_state_apply(Axis5W3State* s, unsigned int lane, unsigned int value) {
    unsigned int idx = lane % 6u;
    s->bins[idx] += ((value & 0xffu) ^ ((lane & 15u) << 3));
}

void axis5_w3_state_apply_batch(
    Axis5W3State* s,
    const unsigned int* lanes,
    const unsigned int* values,
    int n
) {
    for (int i = 0; i < n; ++i) axis5_w3_state_apply(s, lanes[i], values[i]);
}

void axis5_w3_state_checkpoint_encode(const Axis5W3State* s, unsigned int wire[7]) {
    for (int i = 0; i < 6; ++i) wire[i] = s->bins[i] ^ (0xA55Au + (unsigned int)i);
    wire[6] = s->salt ^ 0x5A5Au;
}

void axis5_w3_state_checkpoint_decode(Axis5W3State* s, const unsigned int wire[7]) {
    for (int i = 0; i < 6; ++i) s->bins[i] = wire[i] ^ (0xA55Au + (unsigned int)i);
    s->salt = wire[6] ^ 0x5A5Au;
}

unsigned int axis5_w3_state_signature(const Axis5W3State* s) {
    unsigned int h = 0x811c9dc5u ^ s->salt;
    for (int i = 0; i < 6; ++i) {
        h ^= s->bins[i] + 0x9e3779b9u + (h << 6) + (h >> 2);
    }
    return h;
}
