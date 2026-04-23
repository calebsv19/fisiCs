#include <stdio.h>

void crh_reset(unsigned seed);
unsigned crh_step(unsigned lane, unsigned opcode, unsigned arg);
unsigned crh_digest(void);

static unsigned run_pass(const unsigned char* stream, unsigned count, unsigned seed) {
    unsigned i;
    unsigned acc = 19u;

    crh_reset(seed);
    for (i = 0u; i < count; ++i) {
        unsigned lane = (unsigned)(stream[i] & 3u);
        unsigned opcode = (unsigned)((stream[i] >> 2) & 3u);
        unsigned arg = (unsigned)stream[i] * 7u + i * 11u + seed;
        acc = acc * 173u + crh_step(lane, opcode, arg);
    }

    return acc ^ crh_digest();
}

int main(void) {
    static const unsigned char pass_a[] = {3u, 9u, 6u, 12u, 1u, 15u, 8u, 4u, 10u, 14u};
    static const unsigned char pass_b[] = {14u, 2u, 11u, 5u, 13u, 7u, 0u, 12u, 9u, 6u};
    unsigned a1 = run_pass(pass_a, 10u, 31u);
    unsigned a2 = run_pass(pass_a, 10u, 31u);
    unsigned b1 = run_pass(pass_b, 10u, 43u);

    if (a1 != a2) {
        printf("replay-fail %u %u\n", a1, a2);
        return 33;
    }

    printf("%u %u %u\n", a1, b1, a1 ^ b1);
    return 0;
}
