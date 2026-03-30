#include <stdio.h>

typedef unsigned (*op_fn)(unsigned, unsigned);

static unsigned op_add(unsigned a, unsigned b) {
    return a + b + 0x9e37u;
}

static unsigned op_xor(unsigned a, unsigned b) {
    return (a ^ b) + 0x7f4au;
}

static unsigned op_mix(unsigned a, unsigned b) {
    unsigned x = (a << 5u) | (a >> 27u);
    unsigned y = (b << 11u) | (b >> 21u);
    return (x ^ y) + (a & b);
}

static unsigned step(unsigned v) {
    return v * 1664525u + 1013904223u;
}

int main(void) {
    op_fn table[3] = { op_add, op_xor, op_mix };
    unsigned seed = 0x31415926u;
    unsigned state = 0x27182818u;
    unsigned hash = 2166136261u;

    for (unsigned round = 0; round < 96u; ++round) {
        unsigned lane = (seed >> 29u) & 3u;
        unsigned iter = 0u;

        while (iter < 40u) {
            seed = step(seed ^ round ^ iter);
            unsigned x = seed ^ (state + round * 31u + iter);
            unsigned y = step(x + state + 17u);

            if (lane == 3u) {
                lane = (x >> 3u) % 3u;
            }

            state = table[lane](x, y);
            if ((state & 0x3fu) == 0x21u) {
                lane = (lane + 1u) % 3u;
                iter += 2u;
                continue;
            }

            if ((state & 0x7fu) == 0x4du) {
                break;
            }

            if ((state & 1u) == 0u) {
                lane = (lane + 2u) % 3u;
            } else {
                lane = (lane + 1u) % 3u;
            }
            iter += 1u;
        }

        hash ^= state + round * 131u;
        hash *= 16777619u;
    }

    printf("%u %u\n", state, hash);
    return 0;
}
