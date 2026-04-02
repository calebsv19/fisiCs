#include <stdint.h>
#include <stdio.h>

int main(void) {
    int value = 123;
    int *p = &value;
    uintptr_t raw = (uintptr_t)p;
    int *q = (int*)raw;

    int ok_ptr = (q == p);
    int ok_val = (*q == 123);
    printf("%d %d\n", ok_ptr, ok_val);
    return 0;
}
