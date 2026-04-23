#include <stdio.h>

void crh5_reset(unsigned seed);
unsigned crh5_step(unsigned lane, unsigned opcode, unsigned arg);
unsigned crh5_digest(void);

static unsigned run_matrix(const unsigned short* ops, unsigned count, unsigned seed) {
    unsigned i;
    unsigned acc = 97u;

    crh5_reset(seed);
    for (i = 0u; i < count; ++i) {
        unsigned lane = (unsigned)(ops[i] & 7u);
        unsigned opcode = (unsigned)((ops[i] >> 3) & 7u);
        unsigned arg = (unsigned)ops[i] * 37u + seed * 43u + i * 47u;
        acc = acc * 211u + crh5_step(lane, opcode, arg);
    }

    return acc ^ crh5_digest();
}

int main(void) {
    static const unsigned short set_a[] = {23u, 39u, 61u, 15u, 47u, 62u, 31u, 37u, 49u, 55u, 26u, 17u, 51u, 33u};
    static const unsigned short set_b[] = {63u, 20u, 29u, 40u, 58u, 16u, 50u, 35u, 27u, 59u, 28u, 45u, 18u, 60u};
    unsigned a1 = run_matrix(set_a, 14u, 149u);
    unsigned a2 = run_matrix(set_a, 14u, 149u);
    unsigned b1 = run_matrix(set_b, 14u, 163u);

    if (a1 != a2) {
        printf("replay5-fail %u %u\n", a1, a2);
        return 57;
    }

    printf("%u %u %u\n", a1, b1, (a1 ^ b1) + 0x7Bu);
    return 0;
}
