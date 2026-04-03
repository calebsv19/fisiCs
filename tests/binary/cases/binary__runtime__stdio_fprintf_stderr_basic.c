#include <stdio.h>

int main(void) {
    int a = fprintf(stderr, "err:%d\n", 7);
    int b = fprintf(stdout, "out:%d\n", 11);
    if (a <= 0 || b <= 0) return 2;
    return 0;
}

