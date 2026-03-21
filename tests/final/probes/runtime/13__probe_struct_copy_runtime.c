#include <stdio.h>

typedef struct {
    int x;
    int y;
} Pair;

static Pair make_pair(int x, int y) {
    Pair p = {x, y};
    return p;
}

int main(void) {
    Pair a = make_pair(3, 7);
    Pair b = a;
    b.y += a.x;

    printf("%d %d\n", b.x, b.y);
    return 0;
}
