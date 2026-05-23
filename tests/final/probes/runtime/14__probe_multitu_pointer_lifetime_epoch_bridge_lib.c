typedef struct {
    unsigned epoch;
    unsigned owner;
    unsigned guard;
    unsigned borrow;
    unsigned stale;
} LifetimeNode;

static unsigned rotl32(unsigned x, unsigned s) {
    s &= 31u;
    return s == 0u ? x : ((x << s) | (x >> (32u - s)));
}

unsigned pointer_lifetime_epoch_bridge(unsigned seed) {
    static LifetimeNode nodes[6] = {
        {1u, 3u, 11u, 0u, 1u},
        {2u, 5u, 13u, 1u, 0u},
        {3u, 7u, 17u, 2u, 1u},
        {4u, 11u, 19u, 3u, 0u},
        {5u, 13u, 23u, 4u, 1u},
        {6u, 17u, 29u, 5u, 0u},
    };
    LifetimeNode* routes[6] = {
        &nodes[2], &nodes[5], &nodes[1], &nodes[4], &nodes[0], &nodes[3],
    };
    unsigned acc = seed ^ 0xCB91u;
    unsigned i;

    for (i = 0u; i < 20u; ++i) {
        LifetimeNode* a = routes[(seed + i + nodes[i % 6u].owner) % 6u];
        LifetimeNode* b = &nodes[(i + a->epoch + a->borrow) % 6u];
        unsigned lane = (a->guard + b->stale + i) % 7u;
        a->epoch += 1u + (lane & 1u);
        a->borrow = (a->borrow + lane + b->owner) % 31u;
        b->stale ^= a->epoch + a->borrow + lane;
        b->guard ^= a->owner + b->stale + lane;
        acc ^= rotl32(a->epoch + b->guard + a->borrow, (i & 7u) + 1u);
        acc = acc * 109u + a->owner + b->stale + lane;
    }

    return acc ^ nodes[0].epoch ^ nodes[1].borrow ^ nodes[2].guard ^ nodes[3].stale ^ nodes[4].owner ^ nodes[5].epoch;
}
