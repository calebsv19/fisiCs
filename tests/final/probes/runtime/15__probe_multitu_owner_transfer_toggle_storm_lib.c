#define OTS_SLOTS 3u

typedef struct {
    unsigned owner;
    unsigned epoch;
    unsigned credits;
    int held;
} OtsSlot;

static OtsSlot g_slots[OTS_SLOTS];

void ots_reset(void) {
    unsigned i;
    for (i = 0u; i < OTS_SLOTS; ++i) {
        g_slots[i].owner = i & 3u;
        g_slots[i].epoch = 0u;
        g_slots[i].credits = i * 11u + 5u;
        g_slots[i].held = 0;
    }
}

void ots_toggle(unsigned slot, int enabled) {
    OtsSlot* s = &g_slots[slot % OTS_SLOTS];
    if (enabled) {
        if (s->held == 0) {
            s->held = 1;
            s->epoch += 1u;
            s->owner = (slot + s->epoch) & 3u;
        }
        return;
    }
    if (s->held != 0) {
        s->held = 0;
        s->credits += 1u;
    }
}

int ots_transfer(unsigned slot, unsigned from, unsigned to) {
    OtsSlot* s = &g_slots[slot % OTS_SLOTS];
    if (s->held == 0 || s->owner != (from & 3u)) {
        return 0;
    }
    s->owner = to & 3u;
    s->credits += (to & 3u) + 1u;
    s->epoch += 1u;
    return 1;
}

int ots_release(unsigned slot, unsigned owner, unsigned* out_value) {
    OtsSlot* s = &g_slots[slot % OTS_SLOTS];
    if (s->held == 0 || s->owner != (owner & 3u)) {
        return 0;
    }
    s->held = 0;
    if (out_value) {
        *out_value = s->credits + s->epoch * 3u;
    }
    return 1;
}

unsigned ots_digest(void) {
    unsigned i;
    unsigned digest = 101u;
    for (i = 0u; i < OTS_SLOTS; ++i) {
        digest = digest * 167u
            ^ (g_slots[i].owner * 13u)
            ^ (g_slots[i].epoch * 17u)
            ^ (g_slots[i].credits * 19u)
            ^ (unsigned)g_slots[i].held;
    }
    return digest;
}
