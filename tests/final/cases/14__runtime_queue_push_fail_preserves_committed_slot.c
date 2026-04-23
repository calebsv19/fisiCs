#include <stdio.h>

#define QCAP 4u

typedef struct {
    unsigned values[QCAP];
    unsigned generation[QCAP];
    unsigned head;
    unsigned tail;
    unsigned count;
} queue_state;

static void queue_init(queue_state* q) {
    unsigned i;
    for (i = 0; i < QCAP; ++i) {
        q->values[i] = 0u;
        q->generation[i] = 0u;
    }
    q->head = 0u;
    q->tail = 0u;
    q->count = 0u;
}

static int queue_push(queue_state* q, unsigned value) {
    if (q->count >= QCAP) {
        return 0;
    }

    {
        unsigned slot = q->head;
        q->values[slot] = value ^ ((slot + 1u) * 17u);
        q->generation[slot] += 1u;
    }

    q->head = (q->head + 1u) % QCAP;
    q->count += 1u;
    return 1;
}

static int queue_pop(queue_state* q, unsigned* out_value) {
    if (q->count == 0u) {
        return 0;
    }

    *out_value = q->values[q->tail];
    q->tail = (q->tail + 1u) % QCAP;
    q->count -= 1u;
    return 1;
}

int main(void) {
    queue_state q;
    unsigned before_values[QCAP];
    unsigned before_generation[QCAP];
    unsigned pushed[QCAP] = {11u, 22u, 33u, 44u};
    unsigned fail_push;
    unsigned stable;
    unsigned i;
    unsigned pop_value;
    unsigned pop_digest = 0u;
    unsigned gen_digest = 0u;

    queue_init(&q);
    for (i = 0u; i < QCAP; ++i) {
        (void)queue_push(&q, pushed[i]);
        before_values[i] = q.values[i];
        before_generation[i] = q.generation[i];
    }

    fail_push = (unsigned)queue_push(&q, 55u);

    stable = 1u;
    for (i = 0u; i < QCAP; ++i) {
        if (q.values[i] != before_values[i] || q.generation[i] != before_generation[i]) {
            stable = 0u;
        }
        gen_digest = (gen_digest * 131u) ^ q.generation[i];
    }

    while (queue_pop(&q, &pop_value)) {
        pop_digest = (pop_digest * 167u) ^ pop_value;
    }

    printf("%u %u %u %u\n", fail_push, stable, pop_digest, gen_digest);
    return 0;
}
