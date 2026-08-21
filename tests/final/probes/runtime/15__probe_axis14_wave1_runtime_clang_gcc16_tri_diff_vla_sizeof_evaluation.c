#include <stddef.h>
#include <stdio.h>

static unsigned axis14_measure(unsigned seed) {
    int n = 2 + (int)(seed % 3u);
    size_t first_count = sizeof(int[n++]) / sizeof(int);
    size_t second_count = sizeof(int[n++]) / sizeof(int);
    int values[n];
    unsigned i;
    unsigned sum = 0u;

    for (i = 0u; i < (unsigned)n; ++i) {
        values[i] = (int)(seed + i * 5u);
        sum += (unsigned)values[i];
    }
    return (unsigned)n * 10000u + (unsigned)first_count * 1000u +
           (unsigned)second_count * 100u + sum;
}

int main(void) {
    printf("axis14-vla=%u,%u\n", axis14_measure(4u), axis14_measure(8u));
    return 0;
}
