#include <stdio.h>

#define MF14G_CAP 6u

typedef struct {
    unsigned generation;
    unsigned value;
    int live;
} Entry;

static Entry g_entries[MF14G_CAP];

static void table_reset(void) {
    unsigned i;
    for (i = 0u; i < MF14G_CAP; ++i) {
        g_entries[i].generation = 0u;
        g_entries[i].value = 0u;
        g_entries[i].live = 0;
    }
}

static unsigned publish(unsigned slot, unsigned value, unsigned* out_gen) {
    Entry* e = &g_entries[slot % MF14G_CAP];
    e->generation += 1u;
    e->value = value + slot * 5u;
    e->live = 1;
    if (out_gen) {
        *out_gen = e->generation;
    }
    return slot % MF14G_CAP;
}

static int collect(unsigned slot, unsigned generation, unsigned* out_value) {
    Entry* e = &g_entries[slot % MF14G_CAP];
    if (e->live == 0 || e->generation != generation) {
        return 0;
    }
    if (out_value) {
        *out_value = e->value;
    }
    return 1;
}

int main(void) {
    unsigned s0;
    unsigned s1;
    unsigned g0;
    unsigned g1;
    unsigned g1_new;
    unsigned value = 0u;
    unsigned accepted = 0u;
    unsigned dropped = 0u;
    unsigned digest = 83u;

    table_reset();

    s0 = publish(1u, 17u, &g0);
    s1 = publish(4u, 29u, &g1);

    if (collect(s0, g0, &value)) {
        accepted += 1u;
        digest = digest * 167u + value;
    } else {
        dropped += 1u;
    }

    (void)publish(4u, 53u, &g1_new);

    if (collect(s1, g1, &value)) {
        accepted += 1u;
        digest = digest * 167u + value;
    } else {
        dropped += 1u;
    }

    if (collect(s1, g1_new, &value)) {
        accepted += 1u;
        digest = digest * 167u + value;
    } else {
        dropped += 1u;
    }

    printf("%u %u %u\n", accepted, dropped, digest);
    return 0;
}
