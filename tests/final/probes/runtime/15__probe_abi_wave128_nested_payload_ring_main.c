#include <stdio.h>

union Wave128Bits {
    unsigned words[4];
    struct {
        unsigned lo;
        unsigned hi;
        unsigned tag;
        unsigned mask;
    } named;
};

struct Wave128Leaf {
    unsigned id;
    long bias;
    union Wave128Bits bits;
};

struct Wave128Payload {
    unsigned epoch;
    struct Wave128Leaf lanes[3][2];
    union {
        struct {
            unsigned route;
            unsigned salt;
        } named;
        unsigned words[2];
    } meta;
};

struct Wave128Payload wave128_make_payload(unsigned seed, long bias);
struct Wave128Payload wave128_mix_payload(struct Wave128Payload payload, unsigned step);
unsigned wave128_fold_payload(struct Wave128Payload payload, unsigned salt);

typedef struct Wave128Payload (*Wave128StepFn)(struct Wave128Payload, unsigned);

static unsigned rotl32(unsigned value, unsigned shift) {
    shift &= 31u;
    if (shift == 0u) {
        return value;
    }
    return (value << shift) | (value >> (32u - shift));
}

int main(void) {
    struct Wave128Payload ring[3];
    Wave128StepFn step = wave128_mix_payload;
    unsigned acc = 0x9e3779b9u;
    unsigned i;

    ring[0] = wave128_make_payload(17u, -5L);
    ring[1] = wave128_make_payload(41u, 9L);
    ring[2] = wave128_make_payload(73u, -11L);

    for (i = 0u; i < 9u; ++i) {
        unsigned src = (i + ring[i % 3u].meta.named.route) % 3u;
        unsigned dst = (src + 1u + (ring[src].lanes[i % 3u][i & 1u].bits.named.tag & 1u)) % 3u;
        struct Wave128Payload next = step(ring[src], i + 23u);
        ring[dst] = next;
        acc ^= wave128_fold_payload(next, rotl32(i * 131u + 19u, i + 5u));
        acc = rotl32(acc + ring[dst].lanes[(i + dst) % 3u][(i >> 1u) & 1u].bits.named.mask, 7u);
    }

    printf("%u %u %u\n",
           acc,
           wave128_fold_payload(ring[0], 0x85ebca6bu),
           wave128_fold_payload(ring[2], 0xc2b2ae35u));
    return 0;
}
