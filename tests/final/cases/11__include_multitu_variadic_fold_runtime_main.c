#include <stdio.h>

#include "11__include_multitu_variadic_fold_runtime.h"

int main(void) {
    int total = probe_variadic_fold_inc(5, 3, 6, 7, 8);
    if (total != 26) return 1;
    printf("%d\n", total);
    return 0;
}
