#include <stdio.h>

typedef struct {
    unsigned generation;
    unsigned resident;
    unsigned budget;
    unsigned stale;
    int enabled;
} Layer;

typedef struct {
    char op;
    unsigned lane;
    unsigned arg;
} Step;

int main(void) {
    static const Step script[] = {
        {'E', 0u, 0u},
        {'T', 0u, 5u},
        {'S', 0u, 2u},
        {'E', 1u, 0u},
        {'T', 1u, 6u},
        {'G', 1u, 3u},
        {'D', 0u, 0u},
        {'R', 0u, 4u},
        {'B', 0u, 4u},
        {'R', 1u, 5u},
        {'B', 1u, 5u},
        {'E', 0u, 0u},
        {'T', 0u, 7u},
        {'S', 0u, 1u},
        {'B', 0u, 6u},
    };
    Layer layers[3] = {
        {0u, 0u, 5u, 0u, 0},
        {0u, 0u, 6u, 0u, 0},
        {0u, 0u, 4u, 0u, 0},
    };
    unsigned i;
    unsigned dropped = 0u;
    unsigned trims = 0u;
    unsigned digest = 2166136261u;

    for (i = 0u; i < (unsigned)(sizeof(script) / sizeof(script[0])); ++i) {
        const Step* s = &script[i];
        Layer* l = &layers[s->lane % 3u];

        if (s->op == 'E') {
            if (l->enabled == 0) {
                l->enabled = 1;
                l->generation += 1u;
                l->resident += 1u;
            }
        } else if (s->op == 'D') {
            l->enabled = 0;
            l->stale = l->generation;
        } else if (s->op == 'T') {
            if (l->enabled != 0) {
                l->resident += (s->arg % 4u) + 1u;
            }
        } else if (s->op == 'S') {
            l->stale = l->generation + s->arg;
        } else if (s->op == 'G') {
            l->generation += s->arg;
        } else if (s->op == 'R') {
            if (l->enabled == 0 && l->stale > l->generation) {
                if (l->resident > s->arg) {
                    dropped += s->arg;
                    l->resident -= s->arg;
                } else {
                    dropped += l->resident;
                    l->resident = 0u;
                }
            } else {
                l->resident += (s->arg & 1u);
            }
        } else if (s->op == 'B') {
            l->budget = s->arg;
            while (l->resident > l->budget) {
                l->resident -= 1u;
                trims += 1u;
            }
        }

        digest ^= (l->generation * 17u) ^ (l->resident * 23u) ^ (l->stale * 29u);
        digest *= 16777619u;
    }

    printf("%u %u %u\n", dropped, trims, digest);
    return 0;
}
