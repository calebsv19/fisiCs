#include <stdio.h>

int main(void) {
    int *p = (int*)5;
    int nonnull = (p != 0);
    printf("%d\n", nonnull);
    return 0;
}
