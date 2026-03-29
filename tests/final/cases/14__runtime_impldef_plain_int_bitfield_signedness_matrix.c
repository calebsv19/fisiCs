#include <stdio.h>

typedef struct PlainBits {
    int a : 5;
    int b : 5;
} PlainBits;

int main(void) {
    PlainBits x = {0, 0};
    x.a = -1;
    x.b = 17;

    printf("%d %d\n", x.a, x.b);
    return 0;
}
