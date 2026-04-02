#include <stdio.h>

static unsigned lane_a(unsigned acc, unsigned x, unsigned i) {
    acc += (x * 9u) + (i * 17u) + 5u;
    acc ^= (acc >> 7);
    return acc;
}

static unsigned lane_b(unsigned acc, unsigned x, unsigned i) {
    unsigned rot = (x + i) & 15u;
    unsigned left = acc << rot;
    unsigned right = acc >> ((16u - rot) & 15u);
    acc ^= (left | right) + x * 13u + i * 3u;
    acc *= 2654435761u;
    return acc;
}

static unsigned run_matrix(const unsigned *data, unsigned n) {
    unsigned acc = 0x51f15e5u;
    for (unsigned i = 0; i < n; ++i) {
        switch ((data[i] + i) % 4u) {
            case 0u:
                acc = lane_a(acc, data[i], i);
                break;
            case 1u:
                acc = lane_b(acc, data[i], i);
                break;
            case 2u:
                acc = lane_a(lane_b(acc, data[i], i), data[i] ^ 19u, i + 1u);
                break;
            default:
                acc = lane_b(lane_a(acc, data[i], i), data[i] + 7u, i + 3u);
                break;
        }
    }
    return acc ^ (acc >> 11);
}

int main(void) {
    static const unsigned data[] = {
        8u, 6u, 7u, 5u, 3u, 0u, 9u, 1u, 4u, 2u, 6u, 5u
    };
    unsigned a = run_matrix(data, 12u);
    unsigned b = run_matrix(data + 2, 8u);
    unsigned c = run_matrix(data + 1, 6u);
    printf("%u %u\n", a, a ^ (b * 3u + c * 5u + 17u));
    return 0;
}
