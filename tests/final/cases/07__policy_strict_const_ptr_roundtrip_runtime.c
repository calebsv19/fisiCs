#include <stdio.h>

int main(void) {
    const int c = 13;
    const int *cp = &c;
    const void *cvp = (const void*)cp;
    const int *cp2 = (const int*)cvp;
    int ok_ptr = (cp2 == &c);
    int ok_val = (*cp2 == 13);
    printf("%d %d\n", ok_ptr, ok_val);
    return 0;
}
