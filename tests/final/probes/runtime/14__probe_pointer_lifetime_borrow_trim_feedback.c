#include <stdio.h>

typedef struct {
    unsigned owner;
    unsigned borrow;
    unsigned trim;
    unsigned replay;
    unsigned resident;
} Slot;

static unsigned mix(unsigned acc, unsigned x, unsigned y) {
    unsigned shift = (y & 7u) + 1u;
    return (acc ^ (x * 137u) ^ (y * 43u)) + ((x << shift) | (x >> (32u - shift)));
}

static unsigned run(unsigned seed) {
    Slot slots[4] = {
        {3u, 0u, 1u, 0u, 7u},
        {5u, 1u, 2u, 1u, 6u},
        {7u, 2u, 3u, 2u, 5u},
        {11u, 3u, 4u, 3u, 4u},
    };
    Slot* routes[4] = {&slots[1], &slots[3], &slots[0], &slots[2]};
    unsigned acc = seed ^ 0x52C9u;
    unsigned i;

    for (i = 0u; i < 16u; ++i) {
        Slot* a = routes[(seed + i + slots[i & 3u].owner) & 3u];
        Slot* b = &slots[(i + a->owner + a->borrow) & 3u];
        unsigned moved = (a->resident + b->replay + i) % 5u;
        if (a->resident >= moved) {
            a->resident -= moved;
            b->resident += moved;
        }
        a->borrow = (a->borrow + moved + b->owner) % 17u;
        b->trim ^= a->borrow + b->resident + i;
        if (b->resident > (b->trim % 9u)) {
            b->resident -= 1u;
        }
        a->replay = (a->replay + b->trim + moved) % 19u;
        acc = mix(acc, a->borrow + b->trim, a->replay + b->resident);
    }

    return acc ^ slots[0].borrow ^ slots[1].trim ^ slots[2].replay ^ slots[3].resident;
}

int main(void) {
    printf("%u %u %u\n", run(17u), run(79u), run(211u));
    return 0;
}
