#define QGR_CAP 6u

typedef struct {
    unsigned generation;
    unsigned value;
    int live;
} QgrEntry;

static QgrEntry g_entries[QGR_CAP];
static unsigned g_head;
static unsigned g_tail;
static unsigned g_live;

void qgr_reset(void) {
    unsigned i;
    for (i = 0u; i < QGR_CAP; ++i) {
        g_entries[i].generation = 0u;
        g_entries[i].value = 0u;
        g_entries[i].live = 0;
    }
    g_head = 0u;
    g_tail = 0u;
    g_live = 0u;
}

unsigned qgr_push(unsigned value, unsigned* out_generation) {
    unsigned slot = g_tail % QGR_CAP;
    QgrEntry* e = &g_entries[slot];

    e->generation += 1u;
    e->value = value + slot * 13u;
    e->live = 1;

    g_tail += 1u;
    if (g_live < QGR_CAP) {
        g_live += 1u;
    } else {
        g_head += 1u;
    }

    if (out_generation) {
        *out_generation = e->generation;
    }
    return slot;
}

unsigned qgr_reclaim(unsigned keep) {
    unsigned reclaimed = 0u;
    while (g_live > keep) {
        unsigned slot = g_head % QGR_CAP;
        g_entries[slot].live = 0;
        g_entries[slot].value = 0u;
        g_head += 1u;
        g_live -= 1u;
        reclaimed += 1u;
    }
    return reclaimed;
}

int qgr_collect(unsigned slot, unsigned generation, unsigned* out_value) {
    QgrEntry* e = &g_entries[slot % QGR_CAP];
    if (e->live == 0 || e->generation != generation) {
        return 0;
    }
    if (out_value) {
        *out_value = e->value;
    }
    return 1;
}

unsigned qgr_digest(void) {
    unsigned i;
    unsigned digest = 41u + g_head * 7u + g_tail * 11u + g_live * 13u;
    for (i = 0u; i < QGR_CAP; ++i) {
        digest = digest * 173u
            ^ (g_entries[i].generation * 17u)
            ^ (g_entries[i].value * 19u)
            ^ (unsigned)g_entries[i].live;
    }
    return digest;
}
