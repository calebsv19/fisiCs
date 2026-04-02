#include <stdio.h>

typedef struct {
    unsigned id;
    unsigned payload[5];
    unsigned scale;
} Lane;

static unsigned fold_lane(const Lane *lane, unsigned bias) {
    unsigned acc = lane->id * 101u + lane->scale * 13u + bias;
    unsigned i = 0u;
    for (i = 0u; i < 5u; ++i) {
        const unsigned *p = lane->payload + i;
        acc ^= (*p + i * lane->scale + bias * 7u);
        acc = (acc << 3) | (acc >> 29);
        acc += (*p * (i + 5u));
    }
    return acc ^ (acc >> 11);
}

int main(void) {
    static const Lane lanes[] = {
        {1u, {3u, 1u, 4u, 1u, 5u}, 2u},
        {2u, {9u, 2u, 6u, 5u, 3u}, 3u},
        {3u, {5u, 8u, 9u, 7u, 9u}, 4u},
        {4u, {3u, 2u, 3u, 8u, 4u}, 5u}
    };
    unsigned a = 0x1234abcdu;
    unsigned b = 0x9e3779b9u;
    unsigned i = 0u;

    for (i = 0u; i < 4u; ++i) {
        a ^= fold_lane(&lanes[i], i + 1u);
        b += fold_lane(&lanes[3u - i], i + 3u);
    }

    printf("%u %u\n", a, a ^ (b + 19u));
    return 0;
}
