#include <stdio.h>

static int update_alias_pair(int *restrict p, int *restrict q) {
    *p = *p + 9;
    *q = *q ^ 3;
    return *p + *q;
}

int main(void) {
    int value = 11;
    int total = update_alias_pair(&value, &value);
    printf("%d %d\n", value, total);
    return 0;
}
