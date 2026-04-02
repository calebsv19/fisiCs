#include <stdio.h>

int main(void) {
    unsigned long raw = 4097ul;
    int *p = (int*)(void*)raw;
    int nonnull = (p != 0);
    printf("%d\n", nonnull);
    return 0;
}
