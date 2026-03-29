#include <stdbool.h>
#include <stdio.h>

int main(void) {
    int value = 7;
    int *nonnull = &value;
    int *nullp = (int *)0;

    bool b0 = (bool)0;
    bool b1 = (bool)-11;
    bool b2 = (bool)0.0;
    bool b3 = (bool)-0.0;
    bool b4 = (bool)0.25;
    bool b5 = (bool)nonnull;
    bool b6 = (bool)nullp;

    unsigned mix = 0u;
    mix += (unsigned)b0;
    mix += (unsigned)b1 * 2u;
    mix += (unsigned)b2 * 4u;
    mix += (unsigned)b3 * 8u;
    mix += (unsigned)b4 * 16u;
    mix += (unsigned)b5 * 32u;
    mix += (unsigned)b6 * 64u;

    printf("%d %d %d %d %d %d %d %u\n",
           (int)b0, (int)b1, (int)b2, (int)b3, (int)b4, (int)b5, (int)b6, mix);
    return 0;
}
