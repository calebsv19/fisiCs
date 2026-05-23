#include <stdio.h>

typedef struct {
    unsigned generation;
    unsigned resident;
    unsigned budget;
    unsigned epoch;
    unsigned pending;
    unsigned checkpoint;
    unsigned reseed;
    int enabled;
} LayerState;

typedef struct {
    char op;
    unsigned lane;
    unsigned arg;
} Event;

int main(void) {
    static const Event events[] = {
        {'E', 0u, 0u}, {'Q', 0u, 4u}, {'K', 0u, 3u}, {'C', 0u, 0u},
        {'B', 0u, 7u}, {'E', 1u, 0u}, {'Q', 1u, 9u}, {'K', 1u, 5u},
        {'C', 1u, 0u}, {'T', 1u, 6u}, {'B', 1u, 8u}, {'E', 2u, 0u},
        {'Q', 2u, 5u}, {'K', 2u, 4u}, {'C', 2u, 0u}, {'B', 2u, 6u},
        {'D', 1u, 0u}, {'E', 1u, 0u}, {'Q', 1u, 3u}, {'K', 1u, 2u},
        {'C', 1u, 0u}, {'B', 0u, 5u}, {'T', 0u, 7u}, {'C', 0u, 0u},
    };
    LayerState layers[3] = {
        {0u, 0u, 8u, 0u, 0u, 1u, 2u, 0},
        {0u, 0u, 8u, 0u, 0u, 3u, 4u, 0},
        {0u, 0u, 6u, 0u, 0u, 5u, 6u, 0},
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
                l->pending += e->arg + (l->reseed & 1u);
            }
        } else if (e->op == 'K') {
            l->checkpoint = (l->checkpoint + e->arg + l->epoch) % 17u;
            l->reseed = (l->reseed + e->arg + l->generation) % 13u;
        } else if (e->op == 'C') {
            if (l->enabled != 0 && l->pending > 0u) {
                l->resident += l->pending + (l->checkpoint & 1u);
                l->pending = 0u;
                commits += 1u;
            }
        } else if (e->op == 'T') {
            if (l->enabled != 0) {
                l->resident += (e->arg % 5u) + (l->reseed & 3u);
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
            ^ (l->pending * 31u)
            ^ (l->checkpoint * 37u)
            ^ (l->reseed * 41u);
        digest *= 16777619u;
    }

    printf("%u %u %u\n", commits, trims, digest);
    return 0;
}
