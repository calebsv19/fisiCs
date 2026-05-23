#include <stdio.h>

typedef struct {
    unsigned epoch;
    unsigned owner;
    unsigned stale;
    unsigned guard;
    unsigned budget;
} Cell;

static unsigned rotl32(unsigned x, unsigned s) {
    s &= 31u;
    return s == 0u ? x : ((x << s) | (x >> (32u - s)));
}

static unsigned run(unsigned seed) {
    Cell cells[5] = {
        {1u, 3u, 0u, 11u, 9u},
        {2u, 5u, 1u, 13u, 8u},
        {3u, 7u, 0u, 17u, 7u},
        {4u, 11u, 1u, 19u, 6u},
        {5u, 13u, 0u, 23u, 5u},
    };
    Cell* alias[5] = {&cells[2], &cells[4], &cells[1], &cells[3], &cells[0]};
    unsigned acc = seed ^ 0xA173u;
    unsigned i;

    for (i = 0u; i < 18u; ++i) {
        Cell* a = alias[(seed + i + alias[i % 5u]->owner) % 5u];
        Cell* b = &cells[(i + a->epoch + a->owner) % 5u];
        unsigned lane = (a->budget + b->guard + i) % 6u;
        a->epoch += 1u + (lane & 1u);
        b->stale ^= a->epoch + lane + b->owner;
        if ((b->stale & 1u) != 0u) {
            a->budget = (a->budget + b->stale + lane) % 29u;
        } else if (a->budget > 0u) {
            a->budget -= 1u;
        }
        b->guard ^= a->budget + b->stale + a->owner;
        acc ^= rotl32(a->epoch + b->guard + a->budget, (i & 7u) + 1u);
        acc = acc * 101u + a->owner + b->stale + lane;
    }

    return acc ^ cells[0].epoch ^ cells[1].stale ^ cells[2].guard ^ cells[3].budget ^ cells[4].owner;
}

int main(void) {
    printf("%u %u\n", run(31u), run(149u));
    return 0;
}
