#include <stdio.h>

void crh7_reset(unsigned seed);
unsigned crh7_step(unsigned lane, unsigned opcode, unsigned arg);
unsigned crh7_digest(void);

static unsigned run_matrix(const unsigned short* ops, unsigned count, unsigned seed) {
    unsigned i;
    unsigned acc = 149u;

    crh7_reset(seed);
    for (i = 0u; i < count; ++i) {
        unsigned lane = (unsigned)(ops[i] & 7u);
        unsigned opcode = (unsigned)((ops[i] >> 3) & 7u);
        unsigned arg = (unsigned)ops[i] * 43u + seed * 47u + i * 71u;
        acc = acc * 197u + crh7_step(lane, opcode, arg);
    }

    return acc ^ crh7_digest();
}

int main(void) {
    static const unsigned short set_a[] = {11u, 53u, 28u, 44u, 35u, 62u, 25u, 39u, 18u, 57u, 20u, 46u, 31u, 59u};
    static const unsigned short set_b[] = {60u, 16u, 42u, 23u, 49u, 27u, 54u, 19u, 36u, 21u, 58u, 24u, 47u, 30u};
    unsigned a1 = run_matrix(set_a, 14u, 191u);
    unsigned a2 = run_matrix(set_a, 14u, 191u);
    unsigned b1 = run_matrix(set_b, 14u, 199u);

    if (a1 != a2) {
        printf("replay7-fail %u %u\n", a1, a2);
        return 67;
    }

    printf("%u %u %u\n", a1, b1, (a1 + b1) ^ 0x2D3u);
    return 0;
}
