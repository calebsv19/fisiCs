#define QE62_CAP 6u

typedef struct {
    unsigned generation;
    unsigned value;
    int owned;
} qe62_slot;

static qe62_slot g_slots[QE62_CAP];
static unsigned g_head;
static unsigned g_epoch;

static unsigned qe62_mix(unsigned x, unsigned y) {
    unsigned v = x ^ (y + 0x9e3779b9u + (x << 6) + (x >> 2));
    v ^= v >> 15u;
    v *= 0x85ebca6bu;
    v ^= v >> 13u;
    v *= 0xc2b2ae35u;
    v ^= v >> 16u;
    return v;
}

int qe62_submit(unsigned value, int should_fail) {
    unsigned slot;
    qe62_slot* s;

    if (should_fail) {
        return 0;
    }

    slot = g_head % QE62_CAP;
    s = &g_slots[slot];

    s->generation = ++g_epoch;
    s->value = qe62_mix(value + slot * 13u, s->generation * 19u + g_head);
    s->owned = 1;
    g_head = (g_head + 1u) % QE62_CAP;
    return 1;
}

unsigned qe62_reclaim(unsigned min_epoch) {
    unsigned i;
    unsigned reclaimed = 0u;
    for (i = 0u; i < QE62_CAP; ++i) {
        if (g_slots[i].owned && g_slots[i].generation < min_epoch) {
            g_slots[i].owned = 0;
            g_slots[i].value = qe62_mix(g_slots[i].value, i * 97u + min_epoch);
            reclaimed += 1u;
        }
    }
    return reclaimed;
}

unsigned qe62_epoch(void) {
    return g_epoch;
}

unsigned qe62_digest(void) {
    unsigned i;
    unsigned digest = g_epoch * 131u + g_head * 17u + 5u;
    for (i = 0u; i < QE62_CAP; ++i) {
        digest = qe62_mix(
            digest,
            g_slots[i].value + g_slots[i].generation * 11u + (unsigned)g_slots[i].owned * 3u + i
        );
    }
    return digest;
}
