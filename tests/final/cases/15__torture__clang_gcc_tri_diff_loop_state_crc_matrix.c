#include <stdio.h>

static unsigned mix_lane(unsigned state, unsigned x, unsigned i, unsigned pass) {
    unsigned lane = (x + i + pass) & 3u;
    if (lane == 0u) {
        state ^= x * 131u + i * 17u + pass * 5u;
    } else if (lane == 1u) {
        state += x * 97u + (i + 3u) * 11u;
    } else if (lane == 2u) {
        state = (state << 5) | (state >> 27);
        state ^= x * 29u + i * 7u;
    } else {
        state = state * 2654435761u + x + i + pass;
    }
    return state ^ (state >> 13);
}

int main(void) {
    static const unsigned data[] = {
        4u, 8u, 15u, 16u, 23u, 42u, 7u, 11u, 19u, 31u, 5u, 13u
    };
    unsigned s = 0x7f4a7c15u;
    unsigned t = 0x13c6ef37u;
    unsigned pass = 0u;
    unsigned i = 0u;

    for (pass = 0u; pass < 3u; ++pass) {
        for (i = 0u; i < 12u; ++i) {
            s = mix_lane(s, data[i], i, pass);
            t ^= mix_lane(t + s, data[11u - i], i + 1u, pass + 2u);
        }
    }

    printf("%u %u\n", s, s ^ (t * 3u + 17u));
    return 0;
}
