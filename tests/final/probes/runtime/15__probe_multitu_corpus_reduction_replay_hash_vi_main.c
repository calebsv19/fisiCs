#include <stdio.h>

void crh6_reset(unsigned seed);
unsigned crh6_step(unsigned lane, unsigned opcode, unsigned arg);
unsigned crh6_digest(void);

static unsigned run_matrix(const unsigned short* ops, unsigned count, unsigned seed) {
    unsigned i;
    unsigned acc = 131u;

    crh6_reset(seed);
    for (i = 0u; i < count; ++i) {
        unsigned lane = (unsigned)(ops[i] & 7u);
        unsigned opcode = (unsigned)((ops[i] >> 3) & 7u);
        unsigned arg = (unsigned)ops[i] * 41u + seed * 59u + i * 61u;
        acc = acc * 223u + crh6_step(lane, opcode, arg);
    }

    return acc ^ crh6_digest();
}

int main(void) {
    static const unsigned short set_a[] = {12u, 45u, 19u, 58u, 26u, 33u, 41u, 60u, 17u, 55u, 29u, 38u, 52u, 24u};
    static const unsigned short set_b[] = {63u, 14u, 36u, 21u, 48u, 30u, 57u, 18u, 34u, 27u, 50u, 39u, 22u, 61u};
    unsigned a1 = run_matrix(set_a, 14u, 173u);
    unsigned a2 = run_matrix(set_a, 14u, 173u);
    unsigned b1 = run_matrix(set_b, 14u, 181u);

    if (a1 != a2) {
        printf("replay6-fail %u %u\n", a1, a2);
        return 61;
    }

    printf("%u %u %u\n", a1, b1, (a1 ^ b1) + 0x11Du);
    return 0;
}
