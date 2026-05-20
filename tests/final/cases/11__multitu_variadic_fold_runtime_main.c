#include <stdio.h>

int probe_variadic_fold(int base, int count, ...);

int main(void) {
    int total = probe_variadic_fold(10, 4, 1, 2, 3, 4);
    if (total != 20) return 1;
    printf("%d\n", total);
    return 0;
}
