#include <stdio.h>

typedef struct {
    unsigned a;
    unsigned b;
    unsigned c;
    unsigned d;
} Quad;

static unsigned mix8(
    unsigned a, unsigned b, unsigned c, unsigned d,
    unsigned e, unsigned f, unsigned g, unsigned h,
    unsigned salt
) {
    unsigned x = 2166136261u ^ salt;
    x ^= a + 3u * b + 5u * c + 7u * d;
    x *= 16777619u;
    x ^= e + 11u * f + 13u * g + 17u * h;
    x *= 16777619u;
    x ^= x >> 15u;
    return x;
}

int main(void) {
    unsigned d00 = 1u,  d01 = 2u,  d02 = 3u,  d03 = 4u,  d04 = 5u,  d05 = 6u,  d06 = 7u,  d07 = 8u;
    unsigned d08 = 9u,  d09 = 10u, d10 = 11u, d11 = 12u, d12 = 13u, d13 = 14u, d14 = 15u, d15 = 16u;
    unsigned d16 = 17u, d17 = 18u, d18 = 19u, d19 = 20u, d20 = 21u, d21 = 22u, d22 = 23u, d23 = 24u;
    unsigned d24 = 25u, d25 = 26u, d26 = 27u, d27 = 28u, d28 = 29u, d29 = 30u, d30 = 31u, d31 = 32u;
    unsigned d32 = 33u, d33 = 34u, d34 = 35u, d35 = 36u, d36 = 37u, d37 = 38u, d38 = 39u, d39 = 40u;
    unsigned d40 = 41u, d41 = 42u, d42 = 43u, d43 = 44u, d44 = 45u, d45 = 46u, d46 = 47u, d47 = 48u;
    unsigned d48 = 49u, d49 = 50u, d50 = 51u, d51 = 52u, d52 = 53u, d53 = 54u, d54 = 55u, d55 = 56u;
    unsigned d56 = 57u, d57 = 58u, d58 = 59u, d59 = 60u, d60 = 61u, d61 = 62u, d62 = 63u, d63 = 64u;

    Quad q0 = {3u, 5u, 7u, 11u};
    Quad q1 = {13u, 17u, 19u, 23u};
    Quad q2 = {29u, 31u, 37u, 41u};
    Quad q3 = {43u, 47u, 53u, 59u};
    Quad q4 = {61u, 67u, 71u, 73u};
    Quad q5 = {79u, 83u, 89u, 97u};
    Quad q6 = {101u, 103u, 107u, 109u};
    Quad q7 = {113u, 127u, 131u, 137u};

    unsigned acc = 0x811c9dc5u;
    acc ^= mix8(d00, d01, d02, d03, d04, d05, d06, d07, 3u);
    acc ^= mix8(d08, d09, d10, d11, d12, d13, d14, d15, 5u);
    acc ^= mix8(d16, d17, d18, d19, d20, d21, d22, d23, 7u);
    acc ^= mix8(d24, d25, d26, d27, d28, d29, d30, d31, 11u);
    acc ^= mix8(d32, d33, d34, d35, d36, d37, d38, d39, 13u);
    acc ^= mix8(d40, d41, d42, d43, d44, d45, d46, d47, 17u);
    acc ^= mix8(d48, d49, d50, d51, d52, d53, d54, d55, 19u);
    acc ^= mix8(d56, d57, d58, d59, d60, d61, d62, d63, 23u);

    acc ^= mix8(q0.a, q0.b, q0.c, q0.d, q1.a, q1.b, q1.c, q1.d, 29u);
    acc ^= mix8(q2.a, q2.b, q2.c, q2.d, q3.a, q3.b, q3.c, q3.d, 31u);
    acc ^= mix8(q4.a, q4.b, q4.c, q4.d, q5.a, q5.b, q5.c, q5.d, 37u);
    acc ^= mix8(q6.a, q6.b, q6.c, q6.d, q7.a, q7.b, q7.c, q7.d, 41u);

    printf("%u\n", acc);
    return 0;
}
