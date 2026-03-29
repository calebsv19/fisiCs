#include <stdio.h>

static int restrict_overlap_update(int *restrict p, int *restrict q) {
    *p = *p + 3;
    *q = *q * 2;
    return *p + *q;
}

int main(void) {
    int value = 5;
    int total = restrict_overlap_update(&value, &value);
    printf("%d %d\n", value, total);
    return 0;
}
