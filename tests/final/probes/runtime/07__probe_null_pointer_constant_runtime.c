#include <stdio.h>

int main(void) {
    int x = 7;
    int *p0 = 0;
    int *p1 = (0 ? &x : 0);

    int ok0 = (p0 == 0);
    int ok1 = (p1 == 0);
    p0 = &x;
    int ok2 = (*p0 == 7);

    printf("%d %d %d\n", ok0, ok1, ok2);
    return 0;
}
