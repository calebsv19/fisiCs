#include <stdio.h>

int main(void) {
    int value = 23;
    signed long raw = (signed long)(void*)&value;
    int *p = (int*)(void*)raw;
    int ok_ptr = (p == &value);
    int ok_val = (*p == 23);
    printf("%d %d\n", ok_ptr, ok_val);
    return 0;
}
