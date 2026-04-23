#include <stdio.h>

typedef struct {
    unsigned id;
    unsigned generation;
    unsigned touches;
    int owner;
    unsigned ttl;
} TileLane;

typedef struct {
    char op;
    unsigned idx;
    unsigned arg;
} Event;

int main(void) {
    static const Event events[] = {
        {'U', 0u, 4u},
        {'U', 1u, 3u},
        {'X', 0u, 0u},
        {'G', 0u, 2u},
        {'U', 0u, 5u},
        {'R', 1u, 0u},
        {'U', 2u, 7u},
        {'X', 2u, 0u},
        {'G', 2u, 1u},
        {'U', 2u, 6u},
        {'R', 0u, 0u},
        {'U', 1u, 9u},
    };
    TileLane lanes[3] = {
        {41u, 1u, 0u, 0, 0u},
        {77u, 2u, 0u, 1, 0u},
        {95u, 1u, 0u, 2, 0u},
    };
    unsigned i;
    unsigned replay = 0u;
    unsigned digest = 131u;

    for (i = 0u; i < (unsigned)(sizeof(events) / sizeof(events[0])); ++i) {
        const Event* e = &events[i];
        TileLane* t = &lanes[e->idx % 3u];

        if (e->op == 'U') {
            if (t->ttl == 0u || t->ttl > e->arg) {
                t->touches += 1u + (e->arg % 3u);
                t->generation += (e->arg & 1u);
                replay += (unsigned)(t->owner + 1) * (e->arg + 3u);
            } else {
                replay += e->arg ^ 0x55u;
            }
            if (t->ttl > 0u) {
                t->ttl -= 1u;
            }
        } else if (e->op == 'X') {
            t->owner = (t->owner + 1) & 3;
            t->ttl = 2u + (unsigned)t->owner;
        } else if (e->op == 'G') {
            t->generation += e->arg;
            if (t->touches > e->arg) {
                t->touches -= e->arg;
            }
        } else if (e->op == 'R') {
            if (t->touches > 0u) {
                t->touches -= 1u;
            }
            if (t->generation > 0u) {
                t->generation -= 1u;
            }
            t->ttl = 0u;
        }

        digest = digest * 181u
            + t->id * 5u
            + t->generation * 7u
            + t->touches * 11u
            + (unsigned)(t->owner + 1) * 13u
            + t->ttl * 17u;
    }

    printf("%u %u %u %u\n",
           lanes[0].generation + lanes[1].generation + lanes[2].generation,
           lanes[0].touches + lanes[1].touches + lanes[2].touches,
           replay,
           digest);
    return 0;
}
