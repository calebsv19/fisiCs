#define QB15_CAP 8u

static unsigned g_slots[QB15_CAP];
static unsigned g_head;
static unsigned g_count;

static unsigned qb15_mix(unsigned x, unsigned y) {
    unsigned v = x ^ (y + 0x9e3779b9u + (x << 6) + (x >> 2));
    v ^= v >> 15u;
    v *= 0x85ebca6bu;
    v ^= v >> 13u;
    v *= 0xc2b2ae35u;
    v ^= v >> 16u;
    return v;
}

int qb15_submit(unsigned value, int should_fail) {
    unsigned slot;

    if (should_fail || g_count >= QB15_CAP) {
        return 0;
    }

    slot = g_head % QB15_CAP;
    g_slots[slot] = qb15_mix(value + g_count * 3u, slot * 19u + g_head);
    g_head = (g_head + 1u) % QB15_CAP;
    g_count += 1u;
    return 1;
}

unsigned qb15_digest(void) {
    unsigned i;
    unsigned digest = g_head * 97u + g_count * 13u + 5u;

    for (i = 0u; i < QB15_CAP; ++i) {
        digest = qb15_mix(digest, g_slots[i] + i * 23u);
    }

    return digest;
}

unsigned qb15_state_count(void) {
    return g_count + g_head * 16u;
}
