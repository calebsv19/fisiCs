typedef union BridgeBytes {
    unsigned char bytes[4];
    unsigned short half[2];
} BridgeBytes;

typedef struct BridgeState {
    BridgeBytes payload;
    unsigned short guard;
    unsigned calls;
} BridgeState;

typedef struct BridgeReport {
    unsigned short guard;
    unsigned char lane0;
    unsigned char lane3;
    unsigned calls;
    unsigned fold;
} BridgeReport;

typedef void (*BridgeStep)(BridgeState *, unsigned);

static BridgeState state;

static void add_step(BridgeState *value, unsigned salt) {
    value->payload.bytes[0] = (unsigned char)(value->payload.bytes[0] + salt);
    value->payload.bytes[3] = (unsigned char)(value->payload.bytes[3] ^ value->payload.bytes[1]);
    value->guard = (unsigned short)(value->guard + value->payload.half[0]);
}

static unsigned fold_state(const BridgeState *value) {
    unsigned acc = (unsigned)value->guard * 67u + value->calls;
    unsigned i;
    for (i = 0u; i < 4u; ++i) {
        acc = acc * 97u + (unsigned)value->payload.bytes[i];
    }
    return acc;
}

void wave336_bridge_seed(unsigned seed) {
    unsigned i;
    state.guard = (unsigned short)(0x3100u + seed * 9u);
    state.calls = 0u;
    for (i = 0u; i < 4u; ++i) {
        state.payload.bytes[i] = (unsigned char)(0x22u + seed * 7u + i * 5u);
    }
}

BridgeReport wave336_bridge_apply(unsigned salt) {
    BridgeStep step = add_step;
    BridgeReport report;

    step(&state, salt);
    ++state.calls;
    report.guard = state.guard;
    report.lane0 = state.payload.bytes[0];
    report.lane3 = state.payload.bytes[3];
    report.calls = state.calls;
    report.fold = fold_state(&state);
    return report;
}
