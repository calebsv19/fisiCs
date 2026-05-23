typedef struct {
    unsigned owner;
    unsigned handoff;
    unsigned stale;
    unsigned reclaim;
} ReclaimNode;

static unsigned mix(unsigned acc, unsigned x, unsigned y) {
    unsigned shift = (y & 7u) + 1u;
    return (acc ^ (x * 107u) ^ (y * 41u)) + ((x << shift) | (x >> (32u - shift)));
}

unsigned owner_window_stale_handoff_reclaim(unsigned seed) {
    static ReclaimNode nodes[5] = {
        {5u, 1u, 0u, 2u},
        {7u, 2u, 1u, 3u},
        {11u, 3u, 0u, 4u},
        {13u, 4u, 1u, 5u},
        {17u, 5u, 0u, 6u},
    };
    ReclaimNode* alias[5] = {&nodes[3], &nodes[0], &nodes[4], &nodes[1], &nodes[2]};
    unsigned acc = seed ^ 0x8731u;
    unsigned i;

    for (i = 0u; i < 14u; ++i) {
        ReclaimNode* a = alias[(seed + i + nodes[i % 5u].owner) % 5u];
        ReclaimNode* b = &nodes[(i + a->owner + a->stale) % 5u];
        unsigned lane = (a->handoff + b->reclaim + i) % 7u;
        a->handoff = (a->handoff + lane + b->owner) % 19u;
        b->stale ^= lane + a->owner + b->handoff;
        if ((b->stale & 1u) != 0u) {
            a->reclaim += lane + 1u;
        } else if (a->reclaim > 0u) {
            a->reclaim -= 1u;
        }
        acc = mix(acc, a->reclaim + b->stale, a->handoff + b->owner);
    }

    return acc ^ nodes[0].handoff ^ nodes[1].stale ^ nodes[2].reclaim ^ nodes[3].owner ^ nodes[4].handoff;
}
