#include <stdio.h>

int probe_variadic_fold(int base, int count, ...);

int main(void) {
    int total = probe_variadic_fold(10, 4, 1, 2, 3, 4);
    printf("%d\n", total);
    return total == 20 ? 0 : 1;
}
