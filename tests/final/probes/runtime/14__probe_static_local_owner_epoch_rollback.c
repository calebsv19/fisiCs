#include <stdio.h>

typedef struct {
    unsigned epoch;
    unsigned owner;
    unsigned budget;
    unsigned resident;
    unsigned replay;
    unsigned guard;
} Slot;

static unsigned mix(unsigned x, unsigned y) {
    unsigned shift = (y & 7u) + 1u;
    return (x * 131u) ^ (y * 17u) ^ ((x << shift) | (x >> (32u - shift)));
}

static unsigned step(unsigned seed) {
    static Slot slots[3] = {
        {1u, 3u, 9u, 4u, 0u, 5u},
        {2u, 5u, 8u, 3u, 1u, 7u},
        {3u, 7u, 7u, 2u, 2u, 11u},
    };
    static unsigned epoch_log[6] = {0u, 1u, 2u, 1u, 3u, 2u};
    unsigned i;
    unsigned acc = seed;

    for (i = 0u; i < 6u; ++i) {
        Slot* s = &slots[(seed + i + slots[i % 3u].owner) % 3u];
        unsigned prev_epoch = epoch_log[i];
        s->replay = (s->replay + s->owner + i + prev_epoch) % 13u;
        s->epoch += 1u + (s->replay & 1u);
        s->resident += (s->owner ^ s->guard ^ prev_epoch) & 3u;
        if (s->resident > s->budget) {
            s->resident -= (s->resident - s->budget + 1u) / 2u;
        }
        s->guard ^= s->epoch + s->resident + s->replay + prev_epoch;
        acc = mix(acc ^ s->guard, s->resident + s->owner + prev_epoch);
        epoch_log[i] = s->epoch ^ s->replay;
    }

    return acc ^ slots[0].guard ^ slots[1].epoch ^ slots[2].resident;
}

int main(void) {
    printf("%u %u\n", step(41u), step(97u));
    return 0;
}
