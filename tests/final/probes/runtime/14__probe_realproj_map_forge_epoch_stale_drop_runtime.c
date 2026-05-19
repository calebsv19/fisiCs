#include <stdio.h>

typedef struct {
    unsigned generation;
    unsigned resident;
    unsigned budget;
    unsigned stale;
    unsigned epoch;
    unsigned pending;
    int enabled;
} LayerState;

typedef struct {
    char op;
    unsigned lane;
    unsigned arg;
} Event;

int main(void) {
    static const Event events[] = {
        {'E', 0u, 0u},
        {'T', 0u, 6u},
        {'Q', 0u, 3u},
        {'C', 0u, 0u},
        {'E', 1u, 0u},
        {'T', 1u, 4u},
        {'Q', 1u, 5u},
        {'A', 1u, 0u},
        {'G', 1u, 2u},
        {'Q', 1u, 7u},
        {'C', 1u, 0u},
        {'D', 0u, 0u},
        {'R', 0u, 4u},
        {'B', 0u, 5u},
        {'E', 0u, 0u},
        {'Q', 0u, 2u},
        {'C', 0u, 0u},
        {'B', 1u, 6u},
    };
    LayerState layers[3] = {
        {0u, 0u, 6u, 0u, 0u, 0},
        {0u, 0u, 6u, 0u, 0u, 0},
        {0u, 0u, 4u, 0u, 0u, 0},
    };
    unsigned i;
    unsigned commits = 0u;
    unsigned aborts = 0u;
    unsigned trims = 0u;
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
            l->stale = l->generation + (l->epoch & 1u);
            l->pending = 0u;
        } else if (e->op == 'T') {
            if (l->enabled != 0) {
                l->resident += (e->arg % 5u) + 1u;
            }
        } else if (e->op == 'Q') {
            if (l->enabled != 0) {
                l->pending += e->arg + (l->epoch & 1u);
            }
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
                if (l->resident > e->arg) {
                    l->resident -= e->arg;
                    dropped += e->arg;
                } else {
                    dropped += l->resident;
                    l->resident = 0u;
                }
            }
        } else if (e->op == 'B') {
            l->budget = e->arg;
            while (l->resident > l->budget) {
                l->resident -= 1u;
                trims += 1u;
            }
        }

        digest ^= (l->generation * 17u)
            ^ (l->resident * 23u)
            ^ (l->epoch * 29u)
            ^ (l->stale * 31u)
            ^ (l->pending * 37u);
        digest *= 16777619u;
    }

    printf("%u %u %u %u %u\n", commits, aborts, trims, dropped, digest);
    return 0;
}
