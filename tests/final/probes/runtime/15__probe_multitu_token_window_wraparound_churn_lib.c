#define TW62_CAP 8u

typedef struct {
    unsigned generation;
    unsigned value;
} tw62_slot;

static tw62_slot g_tw[TW62_CAP];

void tw62_reset(unsigned seed) {
    unsigned i;
    for (i = 0u; i < TW62_CAP; ++i) {
        g_tw[i].generation = seed + i * 3u;
        g_tw[i].value = seed ^ (i * 19u + 5u);
    }
}

unsigned tw62_issue(unsigned slot) {
    return g_tw[slot & 7u].generation;
}

int tw62_write(unsigned slot, unsigned token, unsigned value) {
    tw62_slot* s = &g_tw[slot & 7u];
    if (s->generation != token) {
        return 0;
    }
    s->value = (value * 31u) ^ (token * 13u + slot * 7u);
    return 1;
}

int tw62_read(unsigned slot, unsigned token, unsigned* out_value) {
    tw62_slot* s = &g_tw[slot & 7u];
    if (s->generation != token) {
        return 0;
    }
    if (out_value) {
        *out_value = s->value;
    }
    return 1;
}

void tw62_rotate(unsigned slot, unsigned salt) {
    tw62_slot* s = &g_tw[slot & 7u];
    s->generation += 1u;
    s->value ^= (salt * 97u) + s->generation * 11u;
}

unsigned tw62_digest(void) {
    unsigned i;
    unsigned digest = 0x9e3779b9u;
    for (i = 0u; i < TW62_CAP; ++i) {
        digest = (digest * 131u) ^ (g_tw[i].generation * 17u + g_tw[i].value * 3u + i);
    }
    return digest;
}
