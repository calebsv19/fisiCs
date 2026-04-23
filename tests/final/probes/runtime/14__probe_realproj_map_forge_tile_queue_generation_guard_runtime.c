#include <stdio.h>

#define MF14_QUEUE_CAP 6u

typedef struct {
    unsigned tile_id;
    unsigned generation;
    unsigned payload;
    int occupied;
} QueueEntry;

static QueueEntry g_queue[MF14_QUEUE_CAP];
static unsigned g_next;

static void queue_reset(void) {
    unsigned i;
    for (i = 0u; i < MF14_QUEUE_CAP; ++i) {
        g_queue[i].tile_id = 0u;
        g_queue[i].generation = 0u;
        g_queue[i].payload = 0u;
        g_queue[i].occupied = 0;
    }
    g_next = 0u;
}

static unsigned queue_push(unsigned tile_id, unsigned payload, unsigned* out_gen) {
    unsigned slot = g_next % MF14_QUEUE_CAP;
    QueueEntry* e = &g_queue[slot];

    e->tile_id = tile_id;
    e->generation += 1u;
    e->payload = payload + tile_id * 3u;
    e->occupied = 1;
    g_next += 1u;

    if (out_gen) {
        *out_gen = e->generation;
    }
    return slot;
}

static int queue_collect(unsigned slot, unsigned generation, unsigned* out_payload) {
    QueueEntry* e;
    if (slot >= MF14_QUEUE_CAP) {
        return 0;
    }
    e = &g_queue[slot];
    if (!e->occupied || e->generation != generation) {
        return 0;
    }
    if (out_payload) {
        *out_payload = e->payload;
    }
    return 1;
}

int main(void) {
    unsigned slot_a;
    unsigned slot_b;
    unsigned gen_a;
    unsigned gen_b;
    unsigned gen_b2;
    unsigned payload = 0u;
    unsigned accepted = 0u;
    unsigned dropped = 0u;
    unsigned digest = 41u;

    queue_reset();

    slot_a = queue_push(7u, 19u, &gen_a);
    slot_b = queue_push(3u, 11u, &gen_b);
    (void)queue_push(12u, 5u, 0);

    if (queue_collect(slot_a, gen_a, &payload)) {
        accepted += 1u;
        digest = digest * 131u + payload;
    } else {
        dropped += 1u;
    }

    (void)queue_push(3u, 25u, &gen_b2);

    if (queue_collect(slot_b, gen_b, &payload)) {
        accepted += 1u;
        digest = digest * 131u + payload;
    } else {
        dropped += 1u;
    }

    if (queue_collect(slot_b, gen_b2, &payload)) {
        accepted += 1u;
        digest = digest * 131u + payload;
    } else {
        dropped += 1u;
    }

    printf("%u %u %u\n", accepted, dropped, digest);
    return 0;
}
