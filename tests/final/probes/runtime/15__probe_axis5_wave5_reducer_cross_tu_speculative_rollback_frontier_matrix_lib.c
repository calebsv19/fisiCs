typedef struct Axis5W5State {
    unsigned int committed[5];
    unsigned int scratch[5];
    unsigned int salt;
    unsigned int token;
    unsigned int in_branch;
} Axis5W5State;

typedef struct Axis5W5Frontier {
    unsigned int committed[5];
    unsigned int salt;
    unsigned int token;
} Axis5W5Frontier;

static unsigned int axis5_w5_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w5_apply_into(
    unsigned int* bins,
    unsigned int lane,
    unsigned int value,
    unsigned int salt
) {
    unsigned int idx = lane % 5u;
    bins[idx] += ((value & 0xffu) ^ ((lane & 15u) << 3) ^ (salt & 0x1fu));
}

void axis5_w5_state_init(Axis5W5State* s, unsigned int salt) {
    for (int i = 0; i < 5; ++i) {
        s->committed[i] = 0u;
        s->scratch[i] = 0u;
    }
    s->salt = salt;
    s->token = 0x811c9dc5u ^ salt;
    s->in_branch = 0u;
}

void axis5_w5_state_apply_committed(Axis5W5State* s, unsigned int lane, unsigned int value) {
    axis5_w5_apply_into(s->committed, lane, value, s->salt);
    s->token = axis5_w5_mix(s->token, (lane & 31u) ^ (value & 0xffu));
}

void axis5_w5_state_begin_branch(Axis5W5State* s) {
    for (int i = 0; i < 5; ++i) s->scratch[i] = s->committed[i];
    s->in_branch = 1u;
}

void axis5_w5_state_apply_branch(Axis5W5State* s, unsigned int lane, unsigned int value) {
    if (s->in_branch == 0u) return;
    axis5_w5_apply_into(s->scratch, lane, value, s->salt ^ 0x55u);
}

void axis5_w5_state_end_branch(Axis5W5State* s, unsigned int commit) {
    if (s->in_branch == 0u) return;
    if (commit != 0u) {
        for (int i = 0; i < 5; ++i) s->committed[i] = s->scratch[i];
        s->salt = axis5_w5_mix(s->salt, 0xC001u);
        s->token = axis5_w5_mix(s->token, 0x600du);
    } else {
        s->token = axis5_w5_mix(s->token, 0xBADu);
    }
    s->in_branch = 0u;
}

void axis5_w5_state_export_frontier(const Axis5W5State* s, Axis5W5Frontier* out) {
    for (int i = 0; i < 5; ++i) out->committed[i] = s->committed[i] ^ (0xA5A0u + (unsigned int)i);
    out->salt = s->salt ^ 0x5A5Au;
    out->token = s->token ^ 0x33CCu;
}

void axis5_w5_state_import_frontier(Axis5W5State* s, const Axis5W5Frontier* in) {
    for (int i = 0; i < 5; ++i) s->committed[i] = in->committed[i] ^ (0xA5A0u + (unsigned int)i);
    for (int i = 0; i < 5; ++i) s->scratch[i] = s->committed[i];
    s->salt = in->salt ^ 0x5A5Au;
    s->token = in->token ^ 0x33CCu;
    s->in_branch = 0u;
}

unsigned int axis5_w5_state_signature(const Axis5W5State* s) {
    unsigned int h = 0x811c9dc5u ^ s->salt ^ (s->token << 1);
    for (int i = 0; i < 5; ++i) h = axis5_w5_mix(h, s->committed[i]);
    return h;
}
