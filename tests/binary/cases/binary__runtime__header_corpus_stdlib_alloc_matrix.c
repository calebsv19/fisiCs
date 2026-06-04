#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *values = (int *)calloc(4, sizeof(int));
    int *grown;
    int i;
    int sum = 0;

    if (!values) {
        return 2;
    }

    values[0] = 3;
    values[1] = 5;
    values[2] = 7;
    values[3] = 11;

    grown = (int *)realloc(values, 6 * sizeof(int));
    if (!grown) {
        free(values);
        return 3;
    }

    grown[4] = -2;
    grown[5] = 13;

    for (i = 0; i < 6; ++i) {
        sum += grown[i];
    }

    free(grown);
    printf("stdlib-alloc sum=%d count=%d\n", sum, 6);
    return sum == 37 ? 0 : 1;
}
