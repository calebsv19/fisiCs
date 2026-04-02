#include <stdio.h>

int main(void) {
    int values[4] = {11, 22, 33, 44};
    int *base = &values[0];
    const int *c0 = base;
    const int *c1 = c0;
    const void *cv = (const void*)c1;
    const int *c2 = (const int*)cv;
    const int *selected = (values[2] > values[1]) ? c2 : c0;

    int ok0 = (c0 == &values[0]);
    int ok1 = (c1 == c0);
    int ok2 = (c2 == c1);
    int ok3 = (selected == c2);
    int val = *selected + values[3];

    printf("%d %d %d %d %d\n", ok0, ok1, ok2, ok3, val);
    return 0;
}
