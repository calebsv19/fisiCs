#include <stdio.h>

#define MF14_LAYERS 4u

typedef struct {
    unsigned generation;
    unsigned resident;
    unsigned budget;
    int active;
} LayerState;

static void layer_toggle(LayerState* s, int enabled) {
    if (enabled) {
        if (!s->active) {
            s->active = 1;
            s->generation += 1u;
            s->resident += 2u;
        }
        return;
    }
    if (s->active) {
        s->active = 0;
        if (s->resident > 0u) {
            s->resident -= 1u;
        }
    }
}

static void layer_touch(LayerState* s, unsigned tiles) {
    if (s->active) {
        s->resident += (tiles % 7u) + 1u;
    } else if (s->resident > 0u) {
        s->resident -= 1u;
    }
}

static unsigned layer_trim(LayerState* s) {
    unsigned trimmed = 0u;
    while (s->resident > s->budget) {
        s->resident -= 1u;
        trimmed += 1u;
    }
    return trimmed;
}

int main(void) {
    LayerState layers[MF14_LAYERS] = {
        {0u, 0u, 6u, 0},
        {0u, 0u, 5u, 0},
        {0u, 0u, 7u, 0},
        {0u, 0u, 4u, 0},
    };
    unsigned i;
    unsigned trimmed_total = 0u;
    unsigned digest = 97u;

    layer_toggle(&layers[1], 1);
    layer_touch(&layers[1], 8u);
    trimmed_total += layer_trim(&layers[1]);

    layer_toggle(&layers[2], 1);
    layer_touch(&layers[2], 9u);
    layer_touch(&layers[2], 3u);
    trimmed_total += layer_trim(&layers[2]);

    layer_toggle(&layers[1], 0);
    layer_toggle(&layers[1], 1);
    layer_touch(&layers[1], 11u);
    trimmed_total += layer_trim(&layers[1]);

    layer_toggle(&layers[3], 1);
    layer_touch(&layers[3], 6u);
    layer_toggle(&layers[3], 0);
    trimmed_total += layer_trim(&layers[3]);

    for (i = 0u; i < MF14_LAYERS; ++i) {
        digest = digest * 193u
            ^ (layers[i].generation * 17u)
            ^ (layers[i].resident * 31u)
            ^ (layers[i].budget * 13u)
            ^ ((unsigned)layers[i].active * 7u);
    }

    printf("%u %u\n", trimmed_total, digest);
    return 0;
}
