#include <stdio.h>

typedef struct {
    unsigned slot;
    unsigned generation;
    unsigned owner_mask;
    unsigned score;
} Tile;

typedef unsigned (*StepFn)(Tile* t, unsigned arg);

static unsigned step_claim(Tile* t, unsigned arg) {
    t->owner_mask ^= (1u << (arg & 3u));
    t->generation += 1u;
    t->score += arg * 3u + t->slot;
    return t->score ^ (t->generation * 17u);
}

static unsigned step_refresh(Tile* t, unsigned arg) {
    t->score = t->score * 131u + arg + t->owner_mask;
    return t->score ^ (t->slot * 19u);
}

static unsigned step_trim(Tile* t, unsigned arg) {
    unsigned cut = arg % 5u;
    if (t->score > cut) {
        t->score -= cut;
    } else {
        t->score = 0u;
    }
    if (t->generation > 0u) {
        t->generation -= 1u;
    }
    return t->score + t->generation * 23u;
}

int main(void) {
    Tile tiles[4] = {
        {11u, 1u, 1u, 7u},
        {19u, 2u, 2u, 5u},
        {23u, 0u, 4u, 9u},
        {31u, 1u, 8u, 3u},
    };
    StepFn fns[3] = {step_claim, step_refresh, step_trim};
    unsigned digest = 29u;
    unsigned i;

    for (i = 0u; i < 14u; ++i) {
        Tile* t = &tiles[(i * 3u + 1u) % 4u];
        StepFn fn = fns[(i + t->generation + t->owner_mask) % 3u];
        unsigned lane = fn(t, i * 7u + t->slot);
        digest = digest * 181u + lane;
    }

    printf("%u %u %u\n",
           tiles[0].generation + tiles[1].generation + tiles[2].generation + tiles[3].generation,
           tiles[0].owner_mask ^ tiles[1].owner_mask ^ tiles[2].owner_mask ^ tiles[3].owner_mask,
           digest);
    return 0;
}
