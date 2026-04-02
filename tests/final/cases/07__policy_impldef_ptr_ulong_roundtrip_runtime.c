#include <stdio.h>

int main(void) {
    int value = 19;
    unsigned long raw = (unsigned long)(void*)&value;
    int *p = (int*)(void*)raw;
    int ok_ptr = (p == &value);
    int ok_val = (*p == 19);
    printf("%d %d\n", ok_ptr, ok_val);
    return 0;
}
