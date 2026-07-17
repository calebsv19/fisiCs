#include <stdio.h>

typedef unsigned int (*Transform)(unsigned int);

static unsigned int add_bias(unsigned int value) { return value + 17u; }
static unsigned int mix_bits(unsigned int value) { return (value << 3u) ^ (value >> 2u) ^ 29u; }
static unsigned int scale(unsigned int value) { return value * 9u + 5u; }

static unsigned int direct_dispatch(unsigned int selector, unsigned int value) {
    if (selector == 0u) return add_bias(value);
    if (selector == 1u) return mix_bits(value);
    return scale(value);
}

static unsigned int table_dispatch(unsigned int selector, unsigned int value) {
    static Transform const transforms[] = {add_bias, mix_bits, scale};
    return transforms[selector](value);
}

int main(void) {
    const unsigned int inputs[] = {5u, 18u, 77u, 123u, 9u, 41u};
    unsigned int direct = 0u;
    unsigned int table = 0u;
    for (unsigned int i = 0u; i < 6u; ++i) {
        unsigned int selector = i % 3u;
        direct += direct_dispatch(selector, inputs[i]) * (i + 1u);
        table += table_dispatch(selector, inputs[i]) * (i + 1u);
    }
    printf("%u %u %u\n", direct, table, direct == table);
    return 0;
}
