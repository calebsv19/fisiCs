#include <stdio.h>

struct Bits {
    int a : 3;
    int b : 5;
};

int main(void) {
    struct Bits x;

    x.a = -1;
    x.b = 17;

    printf("%d %d %zu\n", x.a, x.b, sizeof(struct Bits));
    return 0;
}
