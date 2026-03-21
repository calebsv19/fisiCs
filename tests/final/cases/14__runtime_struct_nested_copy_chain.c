#include <stdio.h>

typedef struct {
    int x;
    int y;
} Pair;

typedef struct {
    Pair p;
    int z;
} Wrap;

int main(void) {
    Wrap a = {{1, 2}, 3};
    Wrap b = a;
    Wrap c = b;

    c.p.y += c.z;
    c.z += c.p.x;

    printf("%d %d %d\n", c.p.x, c.p.y, c.z);
    return 0;
}
