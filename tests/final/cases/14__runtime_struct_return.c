#include <stdio.h>

struct Pair {
    int a;
    int b;
};

static struct Pair make_pair(int a, int b) {
    struct Pair p = {a, b};
    return p;
}

int main(void) {
    struct Pair p = make_pair(3, 9);
    printf("%d\n", p.a + p.b);
    return 0;
}
