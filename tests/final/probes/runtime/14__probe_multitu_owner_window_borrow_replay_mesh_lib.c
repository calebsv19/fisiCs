typedef struct {
    unsigned owner;
    unsigned credits;
    unsigned replay;
    unsigned epoch;
} OwnerWindow;

static unsigned rotl32(unsigned x, unsigned s) {
    s &= 31u;
    return s == 0u ? x : ((x << s) | (x >> (32u - s)));
}

unsigned owner_window_borrow_replay_mesh(unsigned seed) {
    static OwnerWindow windows[6] = {
        {3u, 8u, 0u, 1u},
        {5u, 7u, 1u, 2u},
        {7u, 6u, 2u, 3u},
        {9u, 5u, 3u, 4u},
        {11u, 4u, 4u, 5u},
        {13u, 3u, 5u, 6u},
    };
    OwnerWindow* routes[6] = {
        &windows[2], &windows[5], &windows[1], &windows[4], &windows[0], &windows[3],
    };
    unsigned acc = seed ^ 0x6A51u;
    unsigned i;

    for (i = 0u; i < 16u; ++i) {
        OwnerWindow* left = routes[(seed + i + windows[i % 6u].owner) % 6u];
        OwnerWindow* right = &windows[(i + left->owner + left->epoch) % 6u];
        unsigned moved = (left->credits + right->replay + i) % 4u;
        if (left->credits >= moved) {
            left->credits -= moved;
            right->credits += moved;
        }
        left->replay = (left->replay + moved + right->owner) % 23u;
        right->epoch += 1u + (left->replay & 1u);
        acc ^= rotl32(left->credits + right->epoch + left->replay, (i & 7u) + 1u);
        acc = acc * 89u + left->owner + right->credits + moved;
    }

    return acc ^ windows[0].credits ^ windows[1].epoch ^ windows[2].replay ^ windows[3].owner ^ windows[4].credits ^ windows[5].epoch;
}
