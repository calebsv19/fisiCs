#include <stdio.h>

int probe_variadic_fold_inc(int base, int count, ...);

int main(void) {
    int total = probe_variadic_fold_inc(5, 3, 6, 7, 8);
    printf("%d\n", total);
    return total == 26 ? 0 : 1;
}
