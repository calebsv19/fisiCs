typedef struct {
    unsigned route;
    unsigned shadow;
    unsigned resident;
    unsigned reclaim;
} RouteNode;

static unsigned mix_route(unsigned acc, unsigned x, unsigned y) {
    unsigned shift = (y & 7u) + 1u;
    return (acc ^ (x * 109u) ^ (y * 29u)) + ((x << shift) | (x >> (32u - shift)));
}

unsigned ptr_alias_route_shadow_reclaim(unsigned seed) {
    static RouteNode nodes[4] = {
        {5u, 1u, 12u, 0u},
        {7u, 3u, 11u, 1u},
        {11u, 5u, 10u, 2u},
        {13u, 7u, 9u, 3u},
    };
    RouteNode* aliases[4] = {&nodes[1], &nodes[3], &nodes[0], &nodes[2]};
    unsigned i;
    unsigned acc = seed ^ 0x51C3u;

    for (i = 0u; i < 14u; ++i) {
        RouteNode* a = aliases[(seed + i + nodes[i & 3u].shadow) & 3u];
        RouteNode* b = &nodes[(i + a->route + a->reclaim) & 3u];
        unsigned spend = (a->route + b->shadow + i) % 6u;
        if (b->resident > spend) {
            b->resident -= spend;
            a->reclaim += spend + 1u;
        } else {
            a->shadow ^= b->resident + spend;
            b->resident = (b->resident + a->route + 3u) % 17u;
        }
        a->route += 1u + (a->reclaim & 1u);
        acc = mix_route(acc, a->route + b->resident, a->shadow + b->reclaim);
    }

    return acc ^ nodes[0].route ^ nodes[1].shadow ^ nodes[2].resident ^ nodes[3].reclaim;
}
