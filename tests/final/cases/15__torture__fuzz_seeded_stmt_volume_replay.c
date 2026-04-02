#include <stdio.h>

static unsigned step(unsigned x) {
    return x * 1103515245u + 12345u;
}

static unsigned fold(unsigned acc, unsigned v) {
    acc ^= v + 0x9e3779b9u + (acc << 6u) + (acc >> 2u);
    return acc;
}

static unsigned run_lane(unsigned seed, unsigned lane) {
    unsigned state = seed ^ (lane * 0x45d9f3bu + 0x7f4a7c15u);
    unsigned hash = 2166136261u ^ lane;

    for (unsigned outer = 0u; outer < 80u; ++outer) {
        unsigned xseed = step(seed ^ (outer * 17u + lane * 13u));
        unsigned i = 0u;

        while (i < 96u) {
            unsigned x = step(xseed ^ i ^ (outer << 3u));
            unsigned y = step(x + state + i * 9u);
            unsigned z = (x ^ (y << 1u)) + (x & y) + lane * 31u;

            if ((lane & 1u) != 0u) {
                z ^= (x >> 3u) + (y << 2u);
            } else {
                z += (x % 97u) * (y % 89u);
            }

            switch ((z >> 5u) & 7u) {
                case 0u:
                    state = fold(state, z ^ i);
                    break;
                case 1u:
                    state = fold(state + 11u, x ^ outer);
                    break;
                case 2u:
                    state = fold(state ^ 0xa5a5a5a5u, y + lane);
                    break;
                case 3u:
                    state = fold(state + lane + i, z ^ seed);
                    break;
                case 4u:
                    state = fold(state ^ xseed, (x + y) ^ z);
                    break;
                case 5u:
                    state = fold(state + (outer << 1u), z + i + lane);
                    break;
                case 6u:
                    state = fold(state ^ (i * 37u), x ^ y ^ z);
                    break;
                default:
                    state = fold(state + 3u, z ^ (xseed >> 1u));
                    break;
            }

            if ((state & 0x1fu) == 0x11u) {
                i += 2u;
                continue;
            }
            if ((state & 0x7fu) == 0x2du) {
                i += 3u;
                continue;
            }

            i += 1u;
        }

        hash = fold(hash, state ^ xseed ^ outer ^ lane);
        seed = step(seed ^ hash ^ state);
    }

    return hash ^ state;
}

int main(void) {
    unsigned seed = 0x13579bdfu;
    unsigned out0 = run_lane(seed, 0u);
    unsigned out1 = run_lane(seed ^ 0x2468ace0u, 1u);
    unsigned out2 = run_lane(seed ^ 0x0badf00du, 2u);
    unsigned out3 = run_lane(seed ^ 0x00c0ffeeu, 3u);

    printf("%u %u\n", out0 ^ out2, out1 ^ out3);
    return 0;
}
