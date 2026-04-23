#include <stdio.h>

#define MF14Q_CAP 8u

typedef struct {
    unsigned resident[MF14Q_CAP];
    unsigned head;
    unsigned tail;
    unsigned live;
} QueueWindow;

static void queue_seed(QueueWindow* q) {
    unsigned i;
    q->head = 0u;
    q->tail = 0u;
    q->live = 0u;
    for (i = 0u; i < MF14Q_CAP; ++i) {
        q->resident[i] = 0u;
    }
}

static void queue_push(QueueWindow* q, unsigned value) {
    unsigned slot = q->tail % MF14Q_CAP;
    q->resident[slot] = value;
    q->tail += 1u;
    if (q->live < MF14Q_CAP) {
        q->live += 1u;
    } else {
        q->head += 1u;
    }
}

static unsigned queue_trim_to(QueueWindow* q, unsigned keep) {
    unsigned trimmed = 0u;
    while (q->live > keep) {
        unsigned slot = q->head % MF14Q_CAP;
        q->resident[slot] = 0u;
        q->head += 1u;
        q->live -= 1u;
        trimmed += 1u;
    }
    return trimmed;
}

int main(void) {
    QueueWindow q;
    unsigned i;
    unsigned trim_a;
    unsigned trim_b;
    unsigned digest = 43u;

    queue_seed(&q);
    for (i = 0u; i < 11u; ++i) {
        queue_push(&q, i * 19u + 7u);
    }

    trim_a = queue_trim_to(&q, 4u);
    trim_b = queue_trim_to(&q, 4u);

    for (i = 0u; i < MF14Q_CAP; ++i) {
        digest = digest * 157u ^ q.resident[i];
    }
    digest = digest * 157u + q.head;
    digest = digest * 157u + q.tail;
    digest = digest * 157u + q.live;

    printf("%u %u %u\n", trim_a, trim_b, digest);
    return 0;
}
