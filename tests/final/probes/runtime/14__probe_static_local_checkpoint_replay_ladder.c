#include <stdio.h>

static unsigned rotl32(unsigned value, unsigned shift) {
    shift &= 31u;
    return shift ? ((value << shift) | (value >> (32u - shift))) : value;
}

static unsigned ladder_step(unsigned lane, unsigned weight) {
    static unsigned state = 19u;
    static unsigned checkpoint = 7u;
    static unsigned replay = 0u;
    unsigned local = state ^ (checkpoint * 13u) ^ (replay * 17u);

    local = rotl32(local + lane * 29u + weight * 11u + 0x5Bu, (lane + replay) & 7u);
    if (((local >> ((lane & 3u) + 1u)) & 1u) != 0u) {
        checkpoint = (checkpoint + weight + (local & 7u)) % 257u;
    } else {
        replay = (replay + lane + (weight & 3u) + (local & 1u)) % 11u;
    }

    state = local ^ (checkpoint * 31u) ^ (replay * 43u);
    return state;
}

int main(void) {
    static const unsigned lanes[] = {1u, 3u, 0u, 4u, 2u, 5u, 1u, 6u};
    static const unsigned weights[] = {7u, 11u, 13u, 17u, 19u, 23u, 29u, 31u};
    unsigned a = 2166136261u;
    unsigned b = 2166136261u;
    unsigned i;

    for (i = 0u; i < 8u; ++i) {
        a = (a * 16777619u) ^ ladder_step(lanes[i], weights[i]);
    }
    for (i = 0u; i < 8u; ++i) {
        b = (b * 16777619u) ^ ladder_step(lanes[7u - i], weights[(i + 3u) % 8u]);
    }

    printf("%u %u\n", a, b);
    return 0;
}
