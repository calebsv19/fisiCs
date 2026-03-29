#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    int* p = NULL;
    int a[3] = {7, 11, 13};

    int is_null = (p == NULL);
    ptrdiff_t d = &a[2] - &a[0];
    uintptr_t z = (uintptr_t)NULL;

    printf("%d %td %llu\n", is_null, d, (unsigned long long)z);
    return 0;
}
