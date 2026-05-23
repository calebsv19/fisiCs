#include <stdio.h>

typedef struct {
    unsigned reseed;
    unsigned handoff;
    unsigned owner;
    unsigned stale;
    unsigned window;
} Node;

static unsigned fold(unsigned acc, unsigned x, unsigned y) {
    return (acc * 89u) ^ (x * 31u) ^ ((y << 5u) | (y >> 27u));
}

static unsigned run(unsigned seed) {
    Node nodes[6] = {
        {1u, 2u, 3u, 0u, 5u},
        {2u, 3u, 5u, 1u, 7u},
        {3u, 4u, 7u, 0u, 11u},
        {4u, 5u, 11u, 1u, 13u},
        {5u, 6u, 13u, 0u, 17u},
        {6u, 7u, 17u, 1u, 19u},
    };
    Node* alias[6] = {&nodes[4], &nodes[1], &nodes[5], &nodes[0], &nodes[3], &nodes[2]};
    unsigned acc = seed ^ 0x8E21u;
    unsigned i;

    for (i = 0u; i < 18u; ++i) {
        Node* a = alias[(seed + i + nodes[i % 6u].owner) % 6u];
        Node* b = &nodes[(i + a->window + a->reseed) % 6u];
        unsigned lane = (a->handoff + b->owner + i) % 8u;
        a->reseed = (a->reseed + lane + b->window) % 23u;
        b->handoff = (b->handoff + a->reseed + lane) % 29u;
        b->stale ^= a->owner + b->handoff + lane;
        a->window = (a->window + b->stale + lane) % 31u;
        acc = fold(acc, a->reseed + b->handoff, a->window + b->stale);
    }

    return acc ^ nodes[0].window ^ nodes[1].reseed ^ nodes[2].handoff ^ nodes[3].owner ^ nodes[4].stale ^ nodes[5].window;
}

int main(void) {
    printf("%u %u\n", run(41u), run(157u));
    return 0;
}
