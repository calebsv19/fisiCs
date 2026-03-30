#include <stdio.h>

static unsigned step(unsigned x) {
    return x * 1103515245u + 12345u;
}

static unsigned fold(unsigned acc, unsigned v) {
    acc ^= v + 0x9e3779b9u + (acc << 6) + (acc >> 2);
    return acc;
}

int main(void) {
    unsigned seed = 0x13579bdfu;
    unsigned state = 0x2468ace0u;
    unsigned hash = 2166136261u;

    for (unsigned outer = 0; outer < 48u; ++outer) {
        seed = step(seed ^ (outer * 17u + 3u));
        unsigned lane = (seed >> 27u) & 7u;
        unsigned i = 0u;

        while (i < 64u) {
            unsigned x = step(seed ^ i ^ (outer << 3u));
            unsigned y = step(x + state + i * 9u);
            unsigned z = (x ^ (y << 1u)) + (x & y);

            if ((lane & 1u) != 0u) {
                z ^= (x >> 3u) + (y << 2u);
            } else {
                z += (x % 97u) * (y % 89u);
            }

            switch ((z >> 5u) & 3u) {
                case 0u:
                    state = fold(state, z ^ i);
                    break;
                case 1u:
                    state = fold(state + 11u, x ^ outer);
                    break;
                case 2u:
                    state = fold(state ^ 0xa5a5a5a5u, y + lane);
                    break;
                default:
                    state = fold(state + lane + i, z ^ seed);
                    break;
            }

            if ((state & 0x1fu) == 0x11u) {
                i += 2u;
                continue;
            }
            if ((state & 0x3fu) == 0x2du) {
                break;
            }

            i += 1u;
        }

        hash = fold(hash, state ^ seed ^ lane);
    }

    printf("%u %u\n", state, hash);
    return 0;
}
