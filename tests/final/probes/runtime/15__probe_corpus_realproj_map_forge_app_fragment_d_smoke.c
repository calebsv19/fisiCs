#include <stdio.h>

typedef struct {
    unsigned generation;
    unsigned resident;
    unsigned budget;
    unsigned pending;
    int active;
} LayerState;

typedef struct {
    char op;
    unsigned layer;
    unsigned value;
} Step;

int main(void) {
    static const Step script[] = {
        {'N', 0u, 0u},
        {'T', 0u, 5u},
        {'R', 0u, 3u},
        {'C', 0u, 0u},
        {'N', 1u, 0u},
        {'T', 1u, 6u},
        {'R', 1u, 2u},
        {'A', 1u, 0u},
        {'T', 0u, 4u},
        {'B', 0u, 5u},
        {'B', 1u, 4u},
        {'F', 0u, 0u},
        {'N', 1u, 0u},
        {'T', 1u, 7u},
        {'R', 1u, 4u},
        {'C', 1u, 0u},
        {'B', 1u, 6u},
    };
    LayerState layers[3] = {
        {0u, 0u, 6u, 0u, 0},
        {0u, 0u, 5u, 0u, 0},
        {0u, 0u, 4u, 0u, 0},
    };
    unsigned i;
    unsigned commits = 0u;
    unsigned aborts = 0u;
    unsigned trims = 0u;
    unsigned digest = 2166136261u;

    for (i = 0u; i < (unsigned)(sizeof(script) / sizeof(script[0])); ++i) {
        const Step* s = &script[i];
        LayerState* l = &layers[s->layer % 3u];

        if (s->op == 'N') {
            if (l->active == 0) {
                l->active = 1;
                l->generation += 1u;
                l->resident += 1u + (s->layer % 3u);
            }
        } else if (s->op == 'F') {
            l->active = 0;
            l->pending = 0u;
            if (l->resident > 0u) {
                l->resident -= 1u;
            }
        } else if (s->op == 'T') {
            if (l->active != 0) {
                l->resident += (s->value % 5u) + 1u;
            }
        } else if (s->op == 'R') {
            if (l->active != 0) {
                l->pending += s->value + (l->generation & 1u);
            }
        } else if (s->op == 'C') {
            if (l->active != 0 && l->pending > 0u) {
                l->resident += l->pending;
                l->pending = 0u;
                commits += 1u;
            }
        } else if (s->op == 'A') {
            if (l->pending > 0u) {
                l->pending = 0u;
                aborts += 1u;
            }
        } else if (s->op == 'B') {
            l->budget = s->value;
            while (l->resident > l->budget) {
                l->resident -= 1u;
                trims += 1u;
            }
        }

        digest ^= (l->generation * 17u) ^ (l->resident * 29u) ^ (l->pending * 31u);
        digest *= 16777619u;
    }

    printf("%u %u %u %u\n", commits, aborts, trims, digest);
    return 0;
}
