#include <stdio.h>

int main(void) {
    printf("%zu %zu\n", sizeof(long double), _Alignof(long double));
    return 0;
}
