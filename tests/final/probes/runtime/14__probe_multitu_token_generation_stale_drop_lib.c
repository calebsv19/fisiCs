#define TG14_CAP 4u

typedef struct {
    unsigned generation;
    unsigned value;
    int occupied;
} tg14_slot;

static tg14_slot g_slots[TG14_CAP];
static unsigned g_next_slot;

void tg14_init(void) {
    unsigned i;
    for (i = 0u; i < TG14_CAP; ++i) {
        g_slots[i].generation = 0u;
        g_slots[i].value = 0u;
        g_slots[i].occupied = 0;
    }
    g_next_slot = 0u;
}

int tg14_enqueue(unsigned value, unsigned* out_slot, unsigned* out_generation) {
    unsigned slot = g_next_slot % TG14_CAP;
    tg14_slot* s = &g_slots[slot];

    s->generation += 1u;
    s->value = value + (slot * 9u);
    s->occupied = 1;

    g_next_slot += 1u;

    if (out_slot) {
        *out_slot = slot;
    }
    if (out_generation) {
        *out_generation = s->generation;
    }
    return 1;
}

int tg14_accept(unsigned slot, unsigned generation, unsigned* out_value) {
    tg14_slot* s;
    if (slot >= TG14_CAP) {
        return 0;
    }
    s = &g_slots[slot];
    if (!s->occupied || s->generation != generation) {
        return 0;
    }
    if (out_value) {
        *out_value = s->value;
    }
    return 1;
}

void tg14_force_reuse(unsigned slot, unsigned value) {
    tg14_slot* s;
    if (slot >= TG14_CAP) {
        return;
    }
    s = &g_slots[slot];
    s->generation += 1u;
    s->value = value ^ (slot * 5u + 3u);
    s->occupied = 1;
}

unsigned tg14_state_digest(void) {
    unsigned i;
    unsigned digest = g_next_slot * 97u + 11u;
    for (i = 0u; i < TG14_CAP; ++i) {
        digest ^= (g_slots[i].generation * (31u + i * 7u));
        digest = digest * 167u + g_slots[i].value + (unsigned)g_slots[i].occupied;
    }
    return digest;
}
