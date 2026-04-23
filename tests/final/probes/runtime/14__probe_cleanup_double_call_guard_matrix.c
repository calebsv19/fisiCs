#include <stdio.h>

#define MF14C_SLOTS 5u

typedef struct {
    unsigned value;
    int acquired;
    int released;
} Slot;

static Slot g_slots[MF14C_SLOTS];

static void reset_slots(void) {
    unsigned i;
    for (i = 0u; i < MF14C_SLOTS; ++i) {
        g_slots[i].value = i * 7u + 3u;
        g_slots[i].acquired = 0;
        g_slots[i].released = 0;
    }
}

static void acquire(unsigned idx, unsigned seed) {
    Slot* s = &g_slots[idx % MF14C_SLOTS];
    s->acquired = 1;
    s->released = 0;
    s->value ^= seed + idx * 11u;
}

static int cleanup_once(unsigned idx, unsigned* out_value) {
    Slot* s = &g_slots[idx % MF14C_SLOTS];
    if (s->acquired == 0 || s->released != 0) {
        return 0;
    }
    s->released = 1;
    if (out_value) {
        *out_value = s->value;
    }
    return 1;
}

int main(void) {
    unsigned i;
    unsigned accepted = 0u;
    unsigned blocked = 0u;
    unsigned digest = 59u;
    unsigned out = 0u;

    reset_slots();

    for (i = 0u; i < MF14C_SLOTS; ++i) {
        acquire(i, i * 13u + 5u);
    }

    for (i = 0u; i < MF14C_SLOTS; ++i) {
        if (cleanup_once(i, &out)) {
            accepted += 1u;
            digest = digest * 173u + out;
        } else {
            blocked += 1u;
        }
        if (cleanup_once(i, &out)) {
            accepted += 1u;
            digest = digest * 173u + out;
        } else {
            blocked += 1u;
        }
    }

    printf("%u %u %u\n", accepted, blocked, digest);
    return 0;
}
