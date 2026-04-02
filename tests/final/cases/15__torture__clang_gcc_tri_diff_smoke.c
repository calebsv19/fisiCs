#include <stdio.h>

static unsigned op_addmul(unsigned acc, unsigned value) {
    return acc + (value * 7u) + 3u;
}

static unsigned op_xorshift(unsigned acc, unsigned value) {
    unsigned mixed = acc ^ (value + 13u);
    mixed ^= (mixed << 5);
    mixed ^= (mixed >> 3);
    return mixed + (value * 11u);
}

static unsigned run_lane(const unsigned *data, unsigned count) {
    unsigned (*ops[2])(unsigned, unsigned) = {op_addmul, op_xorshift};
    unsigned acc = 0x1234u;

    for (unsigned i = 0; i < count; ++i) {
        unsigned lane = (data[i] + i) & 1u;
        acc = ops[lane](acc, data[i]);

        switch ((i + data[i]) % 3u) {
            case 0u:
                acc += (i * 9u) ^ data[i];
                break;
            case 1u:
                acc ^= (data[i] << 2) + i;
                break;
            default:
                acc += (acc >> 4) + 17u;
                break;
        }
    }

    return acc ^ (acc >> 7);
}

int main(void) {
    static const unsigned data[10] = {3u, 1u, 4u, 1u, 5u, 9u, 2u, 6u, 5u, 3u};
    unsigned result = run_lane(data, 10u);
    unsigned checksum = result + (run_lane(data + 2, 6u) * 3u) + (data[0] * data[9]);
    printf("%u %u\n", result, checksum);
    return 0;
}
