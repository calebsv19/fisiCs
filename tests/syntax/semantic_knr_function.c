#include <stdio.h>

int sum(a, b)
int a;
long b;
{
    return a + (int)b;
}

int main(void) {
    int r = sum(2, 3L);
    printf("sum=%d\n", r);
    return r;
}
