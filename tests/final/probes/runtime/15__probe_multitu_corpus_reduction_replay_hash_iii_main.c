#include <stdio.h>

void crh3_reset(unsigned seed);
unsigned crh3_step(unsigned lane, unsigned opcode, unsigned arg);
unsigned crh3_digest(void);

static unsigned run_matrix(const unsigned short* ops, unsigned count, unsigned seed) {
    unsigned i;
    unsigned acc = 67u;

    crh3_reset(seed);
    for (i = 0u; i < count; ++i) {
        unsigned lane = (unsigned)(ops[i] & 7u);
        unsigned opcode = (unsigned)((ops[i] >> 3) & 7u);
        unsigned arg = (unsigned)ops[i] * 23u + seed * 29u + i * 31u;
        acc = acc * 173u + crh3_step(lane, opcode, arg);
    }

    return acc ^ crh3_digest();
}

int main(void) {
    static const unsigned short set_a[] = {19u, 34u, 58u, 11u, 42u, 63u, 27u, 36u, 44u, 51u, 22u, 13u, 47u, 30u};
    static const unsigned short set_b[] = {60u, 17u, 25u, 39u, 54u, 12u, 46u, 31u, 23u, 55u, 28u, 41u, 14u, 62u};
    unsigned a1 = run_matrix(set_a, 14u, 101u);
    unsigned a2 = run_matrix(set_a, 14u, 101u);
    unsigned b1 = run_matrix(set_b, 14u, 113u);

    if (a1 != a2) {
        printf("replay3-fail %u %u\n", a1, a2);
        return 45;
    }

    printf("%u %u %u\n", a1, b1, (a1 ^ b1) + 0x33u);
    return 0;
}
