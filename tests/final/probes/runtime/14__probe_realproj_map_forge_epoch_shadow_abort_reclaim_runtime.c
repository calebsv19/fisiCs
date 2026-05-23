#include <stdio.h>

typedef struct {
    unsigned generation;
    unsigned resident;
    unsigned budget;
    unsigned stale;
    unsigned epoch;
    unsigned pending;
    unsigned shadow;
    int enabled;
} LayerState;

typedef struct {
    char op;
    unsigned lane;
    unsigned arg;
} Event;

int main(void) {
    static const Event events[] = {
        {'E', 0u, 0u}, {'Q', 0u, 6u}, {'S', 0u, 3u}, {'A', 0u, 0u},
        {'Q', 0u, 5u}, {'C', 0u, 0u}, {'E', 1u, 0u}, {'Q', 1u, 8u},
        {'S', 1u, 2u}, {'G', 1u, 3u}, {'Q', 1u, 4u}, {'C', 1u, 0u},
        {'D', 1u, 0u}, {'R', 1u, 5u}, {'E', 2u, 0u}, {'Q', 2u, 7u},
        {'S', 2u, 4u}, {'A', 2u, 0u}, {'Q', 2u, 6u}, {'C', 2u, 0u},
        {'B', 2u, 6u}, {'D', 0u, 0u}, {'R', 0u, 4u}, {'E', 1u, 0u},
        {'Q', 1u, 3u}, {'S', 1u, 5u}, {'C', 1u, 0u}, {'B', 1u, 7u},
    };
    LayerState layers[3] = {
        {0u, 0u, 7u, 0u, 0u, 0u, 1u, 0},
        {0u, 0u, 8u, 0u, 0u, 0u, 3u, 0},
        {0u, 0u, 5u, 0u, 0u, 0u, 5u, 0},
    };
    unsigned i;
    unsigned commits = 0u;
    unsigned aborts = 0u;
    unsigned dropped = 0u;
    unsigned digest = 2166136261u;

    for (i = 0u; i < (unsigned)(sizeof(events) / sizeof(events[0])); ++i) {
        const Event* e = &events[i];
        LayerState* l = &layers[e->lane % 3u];

        if (e->op == 'E') {
            if (l->enabled == 0) {
                l->enabled = 1;
                l->generation += 1u;
                l->epoch += 1u;
                l->resident += 1u;
            }
        } else if (e->op == 'D') {
            l->enabled = 0;
            l->stale = l->generation + (l->shadow & 1u);
            l->pending = 0u;
        } else if (e->op == 'Q') {
            if (l->enabled != 0) {
                l->pending += e->arg + (l->epoch & 1u) + (l->shadow & 1u);
            }
        } else if (e->op == 'S') {
            l->shadow = (l->shadow + e->arg + l->generation) % 11u;
        } else if (e->op == 'C') {
            if (l->enabled != 0 && l->pending > 0u) {
                l->resident += l->pending;
                l->pending = 0u;
                commits += 1u;
            }
        } else if (e->op == 'A') {
            if (l->pending > 0u) {
                l->pending = 0u;
                aborts += 1u;
            }
        } else if (e->op == 'G') {
            l->generation += e->arg;
            l->epoch += 1u;
        } else if (e->op == 'R') {
            if (l->enabled == 0 && l->stale > 0u) {
                unsigned want = e->arg + (l->shadow & 3u);
                if (l->resident > want) {
                    l->resident -= want;
                    dropped += want;
                } else {
                    dropped += l->resident;
                    l->resident = 0u;
                }
            }
        } else if (e->op == 'B') {
            l->budget = e->arg;
            while (l->resident > l->budget) {
                l->resident -= 1u;
            }
        }

        digest ^= (l->generation * 17u)
            ^ (l->resident * 23u)
            ^ (l->epoch * 29u)
            ^ (l->stale * 31u)
            ^ (l->pending * 37u)
            ^ (l->shadow * 41u);
        digest *= 16777619u;
    }

    printf("%u %u %u %u\n", commits, aborts, dropped, digest);
    return 0;
}
