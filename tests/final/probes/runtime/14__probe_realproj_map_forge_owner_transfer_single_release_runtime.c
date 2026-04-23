#include <stdio.h>

#define MF14_OWNER_SLOTS 5u

typedef struct {
    unsigned owner;
    unsigned epoch;
    unsigned credits;
    int held;
} OwnerSlot;

static OwnerSlot g_slots[MF14_OWNER_SLOTS];

static void owner_reset(void) {
    unsigned i;
    for (i = 0u; i < MF14_OWNER_SLOTS; ++i) {
        g_slots[i].owner = 0u;
        g_slots[i].epoch = 0u;
        g_slots[i].credits = 0u;
        g_slots[i].held = 0;
    }
}

static void owner_acquire(unsigned slot, unsigned owner, unsigned credits) {
    OwnerSlot* s = &g_slots[slot % MF14_OWNER_SLOTS];
    s->owner = owner;
    s->credits = credits;
    s->epoch += 1u;
    s->held = 1;
}

static int owner_transfer(unsigned slot, unsigned from, unsigned to) {
    OwnerSlot* s = &g_slots[slot % MF14_OWNER_SLOTS];
    if (!s->held || s->owner != from) {
        return 0;
    }
    s->owner = to;
    s->credits += (to % 5u) + 1u;
    s->epoch += 1u;
    return 1;
}

static int owner_release_once(unsigned slot, unsigned owner, unsigned* out_credits) {
    OwnerSlot* s = &g_slots[slot % MF14_OWNER_SLOTS];
    if (!s->held || s->owner != owner) {
        return 0;
    }
    s->held = 0;
    if (out_credits) {
        *out_credits = s->credits;
    }
    return 1;
}

int main(void) {
    unsigned released = 0u;
    unsigned failed = 0u;
    unsigned credits = 0u;
    unsigned digest = 73u;

    owner_reset();

    owner_acquire(0u, 9u, 14u);
    owner_acquire(1u, 3u, 8u);

    if (owner_transfer(0u, 9u, 12u)) {
        digest = digest * 149u + 1u;
    }
    if (owner_transfer(0u, 9u, 2u)) {
        digest = digest * 149u + 3u;
    } else {
        failed += 1u;
    }

    if (owner_release_once(0u, 12u, &credits)) {
        released += 1u;
        digest = digest * 149u + credits;
    }

    if (owner_release_once(0u, 12u, &credits)) {
        released += 1u;
    } else {
        failed += 1u;
    }

    if (owner_transfer(1u, 3u, 4u)) {
        digest = digest * 149u + 5u;
    }
    if (owner_release_once(1u, 4u, &credits)) {
        released += 1u;
        digest = digest * 149u + credits;
    }

    printf("%u %u %u\n", released, failed, digest);
    return 0;
}
