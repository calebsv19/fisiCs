#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *values = (int *)calloc(5, sizeof(int));
    int *shrunk = 0;
    int *grown = 0;
    int zero_count = 0;
    int sum = 0;
    int i;

    if (!values) {
        return 1;
    }

    for (i = 0; i < 5; ++i) {
        if (values[i] == 0) {
            ++zero_count;
        }
        values[i] = i + 10;
    }

    shrunk = (int *)realloc(values, 3 * sizeof(int));
    if (!shrunk) {
        free(values);
        return 2;
    }

    grown = (int *)realloc(shrunk, 6 * sizeof(int));
    if (!grown) {
        free(shrunk);
        return 3;
    }

    grown[3] = -3;
    grown[4] = 40;
    grown[5] = -5;

    for (i = 0; i < 6; ++i) {
        sum += grown[i];
    }

    free(0);
    free(grown);

    printf("stdlib-realloc-calloc zero=%d first=%d third=%d sum=%d count=%d\n",
           zero_count,
           10,
           12,
           sum,
           6);

    return zero_count == 5 && sum == 65 ? 0 : 1;
}
