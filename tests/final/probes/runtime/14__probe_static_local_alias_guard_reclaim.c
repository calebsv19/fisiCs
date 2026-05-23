#include <stdio.h>

typedef struct {
    unsigned generation;
    unsigned owner;
    unsigned credit;
    unsigned guard;
} Cell;

static unsigned fold(unsigned x, unsigned y, unsigned z) {
    return ((x + 0x9E3779B9u) ^ (y * 73u)) + ((z << 5u) | (z >> 27u));
}

static unsigned run(unsigned seed) {
    static Cell cells[4] = {
        {1u, 4u, 11u, 7u},
        {2u, 6u, 9u, 13u},
        {3u, 8u, 7u, 17u},
        {4u, 10u, 5u, 19u},
    };
    Cell* alias[4] = {&cells[2], &cells[0], &cells[3], &cells[1]};
    unsigned i;
    unsigned acc = seed;

    for (i = 0u; i < 10u; ++i) {
        Cell* a = alias[(seed + i) & 3u];
        Cell* b = &cells[(i + a->owner + a->generation) & 3u];
        unsigned moved = (a->credit + b->owner + i) % 5u;
        if (a->credit >= moved) {
            a->credit -= moved;
            b->credit += moved;
        }
        a->generation += 1u;
        b->guard ^= a->generation + moved + a->owner;
        acc = fold(acc ^ a->credit, b->guard + a->generation, a->owner + b->owner);
    }

    return acc ^ cells[0].credit ^ cells[1].guard ^ cells[2].generation ^ cells[3].owner;
}

int main(void) {
    printf("%u %u %u\n", run(13u), run(29u), run(61u));
    return 0;
}
