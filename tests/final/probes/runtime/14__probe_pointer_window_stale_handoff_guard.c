#include <stdio.h>

typedef struct {
    unsigned route;
    unsigned handoff;
    unsigned stale;
    unsigned guard;
    unsigned owner;
} Node;

static unsigned mix(unsigned acc, unsigned x, unsigned y) {
    unsigned shift = (y & 7u) + 1u;
    return (acc ^ (x * 131u) ^ (y * 29u)) + ((x << shift) | (x >> (32u - shift)));
}

static unsigned run(unsigned seed) {
    Node nodes[5] = {
        {5u, 1u, 0u, 11u, 3u},
        {7u, 2u, 1u, 13u, 5u},
        {11u, 3u, 0u, 17u, 7u},
        {13u, 4u, 1u, 19u, 9u},
        {17u, 5u, 0u, 23u, 11u},
    };
    Node* alias[5] = {&nodes[1], &nodes[4], &nodes[0], &nodes[3], &nodes[2]};
    unsigned acc = seed ^ 0x4C77u;
    unsigned i;

    for (i = 0u; i < 15u; ++i) {
        Node* a = alias[(seed + i + nodes[i % 5u].owner) % 5u];
        Node* b = &nodes[(i + a->route + a->stale) % 5u];
        unsigned lane = (a->owner + b->handoff + i) % 6u;
        a->handoff = (a->handoff + lane + b->guard) % 19u;
        b->stale ^= lane + a->route + b->owner;
        if ((b->stale & 1u) != 0u) {
            a->guard ^= b->stale + a->handoff;
        } else {
            b->guard += a->owner + lane;
        }
        a->route += 1u + (a->handoff & 1u);
        acc = mix(acc, a->guard + b->stale, a->route + b->handoff);
    }

    return acc ^ nodes[0].route ^ nodes[1].handoff ^ nodes[2].stale ^ nodes[3].guard ^ nodes[4].owner;
}

int main(void) {
    printf("%u %u %u\n", run(23u), run(71u), run(149u));
    return 0;
}
