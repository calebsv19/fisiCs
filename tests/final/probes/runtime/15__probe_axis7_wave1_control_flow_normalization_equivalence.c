#include <stdio.h>

static unsigned int branch_form(const unsigned int *values, unsigned int count) {
    unsigned int total = 0u;
    for (unsigned int i = 0u; i < count; ++i) {
        if ((values[i] & 1u) == 0u) total += values[i] * 3u;
        else total += values[i] * 5u + 1u;
    }
    return total;
}

static unsigned int normalized_form(const unsigned int *values, unsigned int count) {
    unsigned int total = 0u;
    for (unsigned int i = 0u; i < count; ++i) {
        unsigned int factor = ((values[i] & 1u) == 0u) ? 3u : 5u;
        total += values[i] * factor + ((values[i] & 1u) ? 1u : 0u);
    }
    return total;
}

int main(void) {
    const unsigned int values[] = {4u, 9u, 2u, 7u, 12u, 1u, 8u};
    unsigned int a = branch_form(values, 7u);
    unsigned int b = normalized_form(values, 7u);
    printf("%u %u %u\n", a, b, a == b);
    return 0;
}
