#include <stdio.h>

static int step(int* value, int delta) {
    *value += delta;
    return *value;
}

int main(void) {
    int x = 0;

    int y = (step(&x, 2), step(&x, 3), x * 2);
    int z = (x ? step(&x, 4) : step(&x, 40));
    int w = (step(&x, -1), x);

    printf("%d %d %d %d\n", y, z, w, x);
    return 0;
}
