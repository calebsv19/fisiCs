#include <stdio.h>

typedef struct {
    unsigned owner;
    unsigned generation;
    unsigned budget;
    unsigned replay;
    unsigned stale;
} Slot;

static unsigned rotl32(unsigned x, unsigned s) {
    s &= 31u;
    return s == 0u ? x : ((x << s) | (x >> (32u - s)));
}

static unsigned run(unsigned seed) {
    Slot slots[4] = {
        {3u, 1u, 9u, 0u, 1u},
        {5u, 2u, 8u, 1u, 0u},
        {7u, 3u, 7u, 2u, 1u},
        {11u, 4u, 6u, 3u, 0u},
    };
    Slot* windows[4] = {&slots[2], &slots[0], &slots[3], &slots[1]};
    unsigned acc = seed ^ 0x91B5u;
    unsigned i;

    for (i = 0u; i < 12u; ++i) {
        Slot* a = windows[(seed + i + windows[i & 3u]->owner) & 3u];
        Slot* b = &slots[(i + a->owner + a->generation) & 3u];
        unsigned borrow = (a->budget + b->replay + i) % 5u;
        if (a->budget >= borrow) {
            a->budget -= borrow;
            b->budget += borrow;
        }
        a->replay = (a->replay + borrow + b->owner) % 17u;
        b->generation += 1u + (a->replay & 1u);
        b->stale ^= a->generation + borrow + b->budget;
        acc ^= rotl32(a->budget + b->stale + a->replay, (i & 7u) + 1u);
        acc = acc * 97u + a->owner + b->generation + borrow;
    }

    return acc ^ slots[0].budget ^ slots[1].generation ^ slots[2].replay ^ slots[3].stale;
}

int main(void) {
    printf("%u %u\n", run(19u), run(83u));
    return 0;
}
