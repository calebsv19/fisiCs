#include <stdio.h>

typedef struct {
    unsigned generation;
    unsigned resident;
    unsigned budget;
    unsigned epoch;
    unsigned pending;
    unsigned guard;
    unsigned replay;
    unsigned handoff;
    int enabled;
} LayerState;

typedef struct {
    char op;
    unsigned lane;
    unsigned arg;
} Event;

int main(void) {
    static const Event events[] = {
        {'E', 0u, 0u}, {'Q', 0u, 5u}, {'P', 0u, 3u}, {'H', 0u, 2u},
        {'C', 0u, 0u}, {'E', 1u, 0u}, {'Q', 1u, 4u}, {'H', 1u, 5u},
        {'P', 1u, 4u}, {'C', 1u, 0u}, {'G', 1u, 2u}, {'B', 1u, 7u},
        {'E', 2u, 0u}, {'Q', 2u, 8u}, {'P', 2u, 6u}, {'H', 2u, 1u},
        {'C', 2u, 0u}, {'B', 2u, 6u}, {'D', 0u, 0u}, {'E', 0u, 0u},
        {'Q', 0u, 7u}, {'P', 0u, 2u}, {'H', 0u, 4u}, {'C', 0u, 0u},
        {'B', 0u, 5u}, {'G', 0u, 3u}, {'Q', 1u, 2u}, {'C', 1u, 0u},
    };
    LayerState layers[3] = {
        {0u, 0u, 8u, 0u, 0u, 3u, 0u, 1u, 0},
        {0u, 0u, 7u, 0u, 0u, 5u, 0u, 2u, 0},
        {0u, 0u, 6u, 0u, 0u, 7u, 0u, 3u, 0},
    };
    unsigned i;
    unsigned commits = 0u;
    unsigned trims = 0u;
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
            l->pending = 0u;
        } else if (e->op == 'Q') {
            if (l->enabled != 0) {
                l->pending += e->arg + l->replay + (l->handoff & 1u);
            }
        } else if (e->op == 'P') {
            l->replay = (l->replay + e->arg + (l->guard & 1u)) % 11u;
        } else if (e->op == 'H') {
            l->handoff = (l->handoff + e->arg + l->epoch) % 13u;
        } else if (e->op == 'C') {
            if (l->enabled != 0 && l->pending > 0u) {
                l->resident += l->pending + (l->handoff & 3u);
                l->pending = 0u;
                commits += 1u;
            }
        } else if (e->op == 'G') {
            l->generation += e->arg;
            l->epoch += 1u;
            l->guard ^= e->arg + l->replay + l->handoff;
        } else if (e->op == 'B') {
            l->budget = e->arg;
            while (l->resident > l->budget) {
                l->resident -= 1u;
                trims += 1u;
            }
        }

        l->guard ^= (l->generation * 11u) + (l->resident * 13u) + (l->replay * 17u) + (l->handoff * 19u);
        digest ^= (l->generation * 17u)
            ^ (l->resident * 23u)
            ^ (l->epoch * 29u)
            ^ (l->pending * 31u)
            ^ (l->guard * 37u)
            ^ (l->replay * 41u)
            ^ (l->handoff * 43u);
        digest *= 16777619u;
    }

    printf("%u %u %u\n", commits, trims, digest);
    return 0;
}
