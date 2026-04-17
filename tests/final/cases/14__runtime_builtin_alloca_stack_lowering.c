#include <alloca.h>
#include <stdio.h>

static int fill_and_sum(int n) {
    int *vals = (int *)alloca((size_t)n * sizeof(int));
    int sum = 0;
    for (int i = 0; i < n; ++i) {
        vals[i] = (i + 1) * 3;
        sum += vals[i];
    }
    return sum;
}

int main(void) {
    int got = fill_and_sum(6);
    if (got != 63) {
        printf("bad:%d\n", got);
        return 1;
    }
    puts("ok");
    return 0;
}
