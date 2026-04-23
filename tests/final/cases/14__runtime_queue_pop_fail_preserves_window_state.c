#include <stdio.h>

#define Q14_CAP 5u

typedef struct {
    unsigned values[Q14_CAP];
    unsigned head;
    unsigned tail;
    unsigned count;
    unsigned epoch;
} q14_state;

static void q14_init(q14_state* q) {
    unsigned i;
    for (i = 0u; i < Q14_CAP; ++i) {
        q->values[i] = 0u;
    }
    q->head = 0u;
    q->tail = 0u;
    q->count = 0u;
    q->epoch = 0u;
}

static int q14_push(q14_state* q, unsigned value) {
    unsigned slot;
    if (q->count >= Q14_CAP) {
        return 0;
    }
    slot = q->head;
    q->values[slot] = (value * 29u) ^ (slot * 7u + q->epoch * 3u);
    q->head = (q->head + 1u) % Q14_CAP;
    q->count += 1u;
    q->epoch += 1u;
    return 1;
}

static int q14_pop(q14_state* q, unsigned* out_value) {
    if (q->count == 0u) {
        return 0;
    }
    *out_value = q->values[q->tail];
    q->tail = (q->tail + 1u) % Q14_CAP;
    q->count -= 1u;
    q->epoch += 1u;
    return 1;
}

int main(void) {
    q14_state q;
    unsigned i;
    unsigned popped = 0u;
    unsigned pop_digest = 0u;
    unsigned fail_empty_initial;
    unsigned fail_push_full;
    unsigned fail_empty_final;
    unsigned stable_after_final_fail;
    unsigned before_head;
    unsigned before_tail;
    unsigned before_count;
    unsigned before_epoch;

    q14_init(&q);

    fail_empty_initial = (unsigned)q14_pop(&q, &popped);

    for (i = 0u; i < Q14_CAP; ++i) {
        (void)q14_push(&q, 100u + i * 13u);
    }
    fail_push_full = (unsigned)q14_push(&q, 999u);

    while (q14_pop(&q, &popped)) {
        pop_digest = (pop_digest * 131u) ^ popped;
    }

    before_head = q.head;
    before_tail = q.tail;
    before_count = q.count;
    before_epoch = q.epoch;

    fail_empty_final = (unsigned)q14_pop(&q, &popped);
    stable_after_final_fail =
        (q.head == before_head) &&
        (q.tail == before_tail) &&
        (q.count == before_count) &&
        (q.epoch == before_epoch);

    printf(
        "%u %u %u %u %u\n",
        fail_empty_initial,
        fail_push_full,
        fail_empty_final,
        stable_after_final_fail,
        pop_digest ^ (q.epoch * 17u)
    );
    return 0;
}
