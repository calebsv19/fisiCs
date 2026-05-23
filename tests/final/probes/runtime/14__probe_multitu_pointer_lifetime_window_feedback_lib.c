typedef struct {
    unsigned window;
    unsigned replay;
    unsigned owner;
    unsigned reclaim;
    unsigned trim;
} WindowNode;

static unsigned mix(unsigned acc, unsigned x, unsigned y) {
    unsigned shift = (y & 7u) + 1u;
    return (acc ^ (x * 149u) ^ (y * 47u)) + ((x << shift) | (x >> (32u - shift)));
}

unsigned pointer_lifetime_window_feedback(unsigned seed) {
    static WindowNode nodes[5] = {
        {5u, 1u, 3u, 0u, 1u},
        {7u, 2u, 5u, 1u, 2u},
        {11u, 3u, 7u, 2u, 3u},
        {13u, 4u, 11u, 3u, 4u},
        {17u, 5u, 13u, 4u, 5u},
    };
    WindowNode* alias[5] = {&nodes[3], &nodes[0], &nodes[4], &nodes[1], &nodes[2]};
    unsigned acc = seed ^ 0x7A43u;
    unsigned i;

    for (i = 0u; i < 17u; ++i) {
        WindowNode* a = alias[(seed + i + nodes[i % 5u].owner) % 5u];
        WindowNode* b = &nodes[(i + a->window + a->replay) % 5u];
        unsigned lane = (a->owner + b->trim + i) % 6u;
        a->replay = (a->replay + lane + b->window) % 23u;
        b->reclaim ^= a->replay + lane + b->owner;
        if ((b->reclaim & 1u) != 0u) {
            a->trim += 1u + (lane & 1u);
        } else if (a->trim > 0u) {
            a->trim -= 1u;
        }
        a->window = (a->window + b->reclaim + a->trim) % 37u;
        acc = mix(acc, a->window + b->reclaim, a->trim + b->owner);
    }

    return acc ^ nodes[0].window ^ nodes[1].replay ^ nodes[2].trim ^ nodes[3].reclaim ^ nodes[4].owner;
}
