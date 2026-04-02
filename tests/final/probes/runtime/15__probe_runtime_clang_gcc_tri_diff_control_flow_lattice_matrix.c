#include <stdio.h>

static unsigned step(unsigned x) {
    return x * 1664525u + 1013904223u;
}

static unsigned run_lattice(unsigned seed, unsigned rounds) {
    unsigned state = seed ^ 0x9e3779b9u;
    unsigned acc = 0x811c9dc5u;

    for (unsigned outer = 0u; outer < rounds; ++outer) {
        unsigned i = 0u;
        seed = step(seed + outer * 17u);

        while (i < 48u) {
            unsigned x = step(seed ^ i ^ (outer << 2u));
            unsigned y = step(x + state + i * 13u);
            unsigned lane = (x ^ (y << 1u) ^ (state >> 1u)) + i * 7u;

            switch ((lane >> 3u) & 7u) {
                case 0u:
                    state ^= lane + 3u;
                    break;
                case 1u:
                    state += (lane ^ outer) + 11u;
                    break;
                case 2u:
                    state = (state << 5u) | (state >> 27u);
                    state ^= lane + i;
                    break;
                case 3u:
                    state += (x % 97u) * (y % 89u);
                    break;
                case 4u:
                    state ^= (x >> 3u) + (y << 2u) + outer;
                    break;
                case 5u:
                    state += lane ^ seed ^ 0xa5a5a5a5u;
                    break;
                case 6u:
                    state ^= (lane + 19u) * 33u;
                    break;
                default:
                    state += (lane ^ 0x7f4a7c15u) + i;
                    break;
            }

            acc ^= state + lane + outer * 5u + i * 3u;
            acc *= 16777619u;

            if ((state & 0x1fu) == 0x0du) {
                i += 2u;
                continue;
            }
            if ((state & 0x7fu) == 0x41u) {
                i += 3u;
                continue;
            }
            i += 1u;
        }
    }

    return acc ^ state ^ (acc >> 11u);
}

int main(void) {
    unsigned a = run_lattice(0x13579bdfu, 41u);
    unsigned b = run_lattice(0x2468ace0u, 37u);
    unsigned c = run_lattice(0x00c0ffeeu, 29u);
    printf("%u %u\n", a ^ b, c ^ (a * 3u + b * 5u));
    return 0;
}
