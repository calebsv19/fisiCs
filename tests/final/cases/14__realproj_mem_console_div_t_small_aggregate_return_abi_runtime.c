#include <stdio.h>
#include <stdlib.h>

static long accumulate_div_pairs(void) {
    static const int inputs[][2] = {
        {7, 3},
        {-7, 3},
        {7, -3},
        {-7, -3},
        {42, 5},
        {-42, 5},
        {42, -5},
        {-42, -5},
        {2147483647, 97},
        {-2147483647, 97}
    };
    long acc = 0;
    size_t i;

    for (i = 0; i < (sizeof(inputs) / sizeof(inputs[0])); ++i) {
        div_t q = div(inputs[i][0], inputs[i][1]);
        acc = (acc * 131L) + ((long)q.quot * 17L) + (long)q.rem;
    }

    return acc;
}

int main(void) {
    printf("%ld\n", accumulate_div_pairs());
    return 0;
}
