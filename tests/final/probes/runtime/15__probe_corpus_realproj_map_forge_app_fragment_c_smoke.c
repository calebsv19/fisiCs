#include <stdio.h>

typedef struct {
    unsigned id;
    unsigned generation;
    int owner;
} Tile;

typedef unsigned (*StepFn)(Tile* tile, unsigned seed);

static unsigned step_touch(Tile* tile, unsigned seed) {
    tile->generation += 1u;
    return seed * 131u + tile->id * 7u + tile->generation;
}

static unsigned step_trim(Tile* tile, unsigned seed) {
    if (tile->generation > 0u) {
        tile->generation -= 1u;
    }
    return seed * 137u + tile->id * 11u + tile->generation;
}

static unsigned step_transfer(Tile* tile, unsigned seed) {
    tile->owner = (tile->owner + 1) & 3;
    return seed * 139u + (unsigned)(tile->owner * 17) + tile->generation;
}

int main(void) {
    Tile tiles[3] = {
        {41u, 2u, 0},
        {77u, 1u, 1},
        {95u, 0u, 2},
    };
    StepFn dispatch[3] = {step_touch, step_trim, step_transfer};
    unsigned i;
    unsigned digest = 19u;

    for (i = 0u; i < 9u; ++i) {
        Tile* tile = &tiles[i % 3u];
        StepFn fn = dispatch[(i + tile->generation) % 3u];
        digest = fn(tile, digest);
    }

    printf("%u %u %u %u\n",
           tiles[0].generation + tiles[1].generation + tiles[2].generation,
           (unsigned)tiles[0].owner,
           (unsigned)tiles[1].owner + (unsigned)tiles[2].owner,
           digest);
    return 0;
}
