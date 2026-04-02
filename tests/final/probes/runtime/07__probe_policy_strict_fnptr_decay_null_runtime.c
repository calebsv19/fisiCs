#include <stdio.h>

static int inc1(int x) {
    return x + 1;
}

int main(void) {
    int (*fp)(int) = inc1;
    int (*fp_null)(int) = 0;
    int (*fp_alias)(int) = fp;
    int ok_nonnull = (fp != 0);
    int ok_null = (fp_null == 0);
    int call_a = fp(41);
    int call_b = fp_alias(1);
    printf("%d %d %d %d\n", ok_nonnull, ok_null, call_a, call_b);
    return 0;
}
