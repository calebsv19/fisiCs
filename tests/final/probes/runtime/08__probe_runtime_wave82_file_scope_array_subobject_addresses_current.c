#include <stdio.h>

static int seed = 3;
static int values[3] = { 5, 7, 11 };

static int *const pointers[3] = {
    &seed,
    &values[1],
    &values[2],
};

int main(void) {
    *pointers[1] += *pointers[0];
    *pointers[2] += *pointers[1];
    printf("%d %d %d\n", values[1], values[2], pointers[2] == &values[2]);
    return 0;
}
