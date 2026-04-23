#include <stdio.h>

void crh4_reset(unsigned seed);
unsigned crh4_step(unsigned lane, unsigned opcode, unsigned arg);
unsigned crh4_digest(void);

static unsigned run_matrix(const unsigned short* ops, unsigned count, unsigned seed) {
    unsigned i;
    unsigned acc = 83u;

    crh4_reset(seed);
    for (i = 0u; i < count; ++i) {
        unsigned lane = (unsigned)(ops[i] & 7u);
        unsigned opcode = (unsigned)((ops[i] >> 3) & 7u);
        unsigned arg = (unsigned)ops[i] * 31u + seed * 37u + i * 41u;
        acc = acc * 197u + crh4_step(lane, opcode, arg);
    }

    return acc ^ crh4_digest();
}

int main(void) {
    static const unsigned short set_a[] = {21u, 37u, 59u, 13u, 45u, 62u, 29u, 35u, 47u, 53u, 24u, 15u, 49u, 31u};
    static const unsigned short set_b[] = {61u, 18u, 27u, 38u, 56u, 14u, 48u, 33u, 25u, 57u, 26u, 43u, 16u, 63u};
    unsigned a1 = run_matrix(set_a, 14u, 127u);
    unsigned a2 = run_matrix(set_a, 14u, 127u);
    unsigned b1 = run_matrix(set_b, 14u, 139u);

    if (a1 != a2) {
        printf("replay4-fail %u %u\n", a1, a2);
        return 51;
    }

    printf("%u %u %u\n", a1, b1, (a1 + b1) ^ 0x5Au);
    return 0;
}
