#include <stdio.h>

void crh2_reset(unsigned seed);
unsigned crh2_step(unsigned lane, unsigned opcode, unsigned arg);
unsigned crh2_digest(void);

static unsigned run_stream(const unsigned short* stream, unsigned count, unsigned seed) {
    unsigned i;
    unsigned acc = 41u;

    crh2_reset(seed);
    for (i = 0u; i < count; ++i) {
        unsigned lane = (unsigned)(stream[i] & 7u);
        unsigned opcode = (unsigned)((stream[i] >> 3) & 7u);
        unsigned arg = (unsigned)stream[i] * 13u + seed * 17u + i * 19u;
        acc = acc * 167u + crh2_step(lane, opcode, arg);
    }

    return acc ^ crh2_digest();
}

int main(void) {
    static const unsigned short stream_a[] = {17u, 44u, 29u, 51u, 62u, 33u, 28u, 47u, 12u, 59u, 38u, 26u};
    static const unsigned short stream_b[] = {63u, 14u, 55u, 21u, 48u, 31u, 42u, 9u, 57u, 36u, 25u, 18u};
    unsigned a1 = run_stream(stream_a, 12u, 71u);
    unsigned a2 = run_stream(stream_a, 12u, 71u);
    unsigned b1 = run_stream(stream_b, 12u, 83u);

    if (a1 != a2) {
        printf("replay2-fail %u %u\n", a1, a2);
        return 39;
    }

    printf("%u %u %u\n", a1, b1, a1 + (b1 ^ 0xA5A5A5A5u));
    return 0;
}
