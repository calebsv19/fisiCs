#include <stdio.h>
#include <stddef.h>

#define MF15A_LAYERS 4u

typedef struct {
    const char* cmd;
    unsigned a;
    unsigned b;
} Op;

typedef struct {
    unsigned generation;
    unsigned resident;
    unsigned budget;
    int active;
} Layer;

int main(void) {
    static const Op script[] = {
        {"on", 1u, 0u},
        {"touch", 1u, 9u},
        {"on", 2u, 0u},
        {"touch", 2u, 11u},
        {"trim", 1u, 0u},
        {"off", 2u, 0u},
        {"on", 1u, 0u},
        {"touch", 1u, 5u},
        {"trim", 1u, 0u},
        {"trim", 2u, 0u},
    };
    Layer layers[MF15A_LAYERS] = {
        {0u, 0u, 6u, 0},
        {0u, 0u, 7u, 0},
        {0u, 0u, 5u, 0},
        {0u, 0u, 4u, 0},
    };
    unsigned i;
    unsigned trims = 0u;
    unsigned digest = 131u;

    for (i = 0u; i < (unsigned)(sizeof(script) / sizeof(script[0])); ++i) {
        unsigned idx = script[i].a % MF15A_LAYERS;
        Layer* s = &layers[idx];

        if (script[i].cmd[0] == 'o' && script[i].cmd[1] == 'n') {
            if (s->active == 0) {
                s->active = 1;
                s->generation += 1u;
                s->resident += 1u + idx;
            }
        } else if (script[i].cmd[0] == 'o' && script[i].cmd[1] == 'f') {
            if (s->active != 0) {
                s->active = 0;
                if (s->resident > 0u) {
                    s->resident -= 1u;
                }
            }
        } else if (script[i].cmd[0] == 't') {
            if (s->active != 0) {
                s->resident += (script[i].b % 5u) + 1u;
            }
        } else {
            while (s->resident > s->budget) {
                s->resident -= 1u;
                trims += 1u;
            }
        }
    }

    for (i = 0u; i < MF15A_LAYERS; ++i) {
        digest = digest * 181u
            ^ (layers[i].generation * 17u)
            ^ (layers[i].resident * 29u)
            ^ ((unsigned)layers[i].active * 37u);
    }

    printf("%u %u\n", trims, digest);
    return 0;
}
