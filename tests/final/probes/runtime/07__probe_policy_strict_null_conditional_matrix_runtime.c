#include <stdio.h>

int main(void) {
    int value = 19;
    int *p = &value;
    int *z = 0;
    int ok0 = ((1 ? p : z) == &value);
    int ok1 = ((0 ? p : z) == 0);
    void *vp = (1 ? (void*)p : (void*)0);
    int ok2 = ((int*)vp == &value);
    void *vz = (0 ? (void*)p : (void*)0);
    int ok3 = (vz == 0);
    printf("%d %d %d %d\n", ok0, ok1, ok2, ok3);
    return 0;
}
