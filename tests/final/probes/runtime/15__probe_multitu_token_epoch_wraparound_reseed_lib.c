#define TER_CAP 5u
#define TER_EPOCH_MOD 16u

typedef struct {
    unsigned epoch;
    unsigned value;
    int live;
} TerEntry;

static TerEntry g_entries[TER_CAP];
static unsigned g_cursor;

void ter_reset(void) {
    unsigned i;
    for (i = 0u; i < TER_CAP; ++i) {
        g_entries[i].epoch = 0u;
        g_entries[i].value = 0u;
        g_entries[i].live = 0;
    }
    g_cursor = 0u;
}

unsigned ter_issue(unsigned token, unsigned* out_epoch) {
    unsigned slot = g_cursor % TER_CAP;
    TerEntry* e = &g_entries[slot];

    e->epoch = (e->epoch + 1u) % TER_EPOCH_MOD;
    e->value = token ^ (slot * 23u + e->epoch * 7u);
    e->live = 1;
    g_cursor += 1u;

    if (out_epoch) {
        *out_epoch = e->epoch;
    }
    return slot;
}

int ter_accept(unsigned slot, unsigned epoch, unsigned* out_value) {
    TerEntry* e = &g_entries[slot % TER_CAP];
    if (e->live == 0 || e->epoch != (epoch % TER_EPOCH_MOD)) {
        return 0;
    }
    if (out_value) {
        *out_value = e->value;
    }
    return 1;
}

void ter_reseed(unsigned slot, unsigned seed) {
    TerEntry* e = &g_entries[slot % TER_CAP];
    e->epoch = (e->epoch + 1u) % TER_EPOCH_MOD;
    e->value = seed * 31u + e->epoch * 13u + slot * 5u;
    e->live = 1;
}

unsigned ter_digest(void) {
    unsigned i;
    unsigned digest = g_cursor * 37u + 7u;
    for (i = 0u; i < TER_CAP; ++i) {
        digest = digest * 163u
            ^ (g_entries[i].epoch * 11u)
            ^ (g_entries[i].value * 17u)
            ^ (unsigned)g_entries[i].live;
    }
    return digest;
}
