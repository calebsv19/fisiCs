#include <stdio.h>

int main(void) {
    int values[4] = {4, 8, 12, 16};
    int *b = &values[0];
    int *m = &values[2];
    int *e = &values[4];
    const int *ce = e;

    int ok0 = (b < m);
    int ok1 = (m < e);
    int ok2 = (e > b);
    int ok3 = (ce == (const int*)e);
    int d0 = (int)(m - b);
    int d1 = (int)(e - b);

    printf("%d %d %d %d %d %d\n", ok0, ok1, ok2, ok3, d0, d1);
    return 0;
}
