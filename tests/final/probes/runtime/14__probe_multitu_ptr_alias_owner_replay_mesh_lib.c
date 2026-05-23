typedef struct {
    unsigned owner;
    unsigned epoch;
    unsigned replay;
    unsigned weight;
} OwnerNode;

static unsigned rotl32(unsigned x, unsigned shift) {
    shift &= 31u;
    return shift == 0u ? x : ((x << shift) | (x >> (32u - shift)));
}

unsigned ptr_alias_owner_replay_mesh(unsigned seed) {
    static OwnerNode nodes[5] = {
        {3u, 1u, 0u, 11u},
        {5u, 2u, 1u, 13u},
        {7u, 3u, 2u, 17u},
        {9u, 4u, 3u, 19u},
        {11u, 5u, 4u, 23u},
    };
    OwnerNode* windows[5] = {&nodes[2], &nodes[4], &nodes[1], &nodes[3], &nodes[0]};
    unsigned i;
    unsigned acc = seed ^ 0xA531u;

    for (i = 0u; i < 12u; ++i) {
        OwnerNode* left = windows[(seed + i) % 5u];
        OwnerNode* right = &nodes[(i + left->owner + left->epoch) % 5u];
        unsigned lane = (left->owner + right->replay + i) % 7u;
        left->replay = (left->replay + lane + right->epoch) % 19u;
        right->epoch += 1u + (left->replay & 1u);
        right->owner ^= left->weight + lane;
        acc ^= rotl32(left->owner + right->epoch + left->replay, (lane & 7u) + 1u);
        acc = acc * 97u + right->weight + left->weight + lane;
    }

    return acc ^ nodes[0].owner ^ nodes[1].epoch ^ nodes[2].replay ^ nodes[3].weight ^ nodes[4].owner;
}
