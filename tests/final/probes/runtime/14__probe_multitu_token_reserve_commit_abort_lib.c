#define RS14_CAP 4u

typedef struct {
    unsigned generation;
    unsigned value;
    int reserved;
    int committed;
} rs14_slot;

static rs14_slot g_slots[RS14_CAP];
static unsigned g_cursor;

void rs14_init(void) {
    unsigned i;
    for (i = 0u; i < RS14_CAP; ++i) {
        g_slots[i].generation = 0u;
        g_slots[i].value = 0u;
        g_slots[i].reserved = 0;
        g_slots[i].committed = 0;
    }
    g_cursor = 0u;
}

int rs14_reserve(unsigned* out_slot, unsigned* out_generation) {
    unsigned slot = g_cursor % RS14_CAP;
    rs14_slot* s = &g_slots[slot];

    s->generation += 1u;
    s->value = 0u;
    s->reserved = 1;
    s->committed = 0;
    g_cursor += 1u;

    if (out_slot) {
        *out_slot = slot;
    }
    if (out_generation) {
        *out_generation = s->generation;
    }
    return 1;
}

int rs14_commit(unsigned slot, unsigned generation, unsigned value) {
    rs14_slot* s;
    if (slot >= RS14_CAP) {
        return 0;
    }
    s = &g_slots[slot];
    if (!s->reserved || s->committed || s->generation != generation) {
        return 0;
    }

    s->value = (value * 41u) ^ (slot * 13u + generation * 7u);
    s->reserved = 0;
    s->committed = 1;
    return 1;
}

int rs14_abort(unsigned slot, unsigned generation) {
    rs14_slot* s;
    if (slot >= RS14_CAP) {
        return 0;
    }
    s = &g_slots[slot];
    if (!s->reserved || s->generation != generation) {
        return 0;
    }
    s->reserved = 0;
    s->committed = 0;
    s->value = 0u;
    return 1;
}

int rs14_read(unsigned slot, unsigned generation, unsigned* out_value) {
    rs14_slot* s;
    if (slot >= RS14_CAP) {
        return 0;
    }
    s = &g_slots[slot];
    if (!s->committed || s->generation != generation) {
        return 0;
    }
    if (out_value) {
        *out_value = s->value;
    }
    return 1;
}

unsigned rs14_digest(void) {
    unsigned i;
    unsigned digest = g_cursor * 59u + 3u;
    for (i = 0u; i < RS14_CAP; ++i) {
        digest = (digest * 173u)
            ^ (g_slots[i].generation * 19u)
            ^ (g_slots[i].value * 11u)
            ^ ((unsigned)g_slots[i].reserved * 5u)
            ^ ((unsigned)g_slots[i].committed * 7u);
    }
    return digest;
}
