#include <stdio.h>

typedef union {
    int i;
    long long ll;
} Box;

static Box mutate(Box in) {
    in.i += 5;
    return in;
}

int main(void) {
    Box b;
    b.i = 11;
    b = mutate(b);
    printf("%d %zu\n", b.i, sizeof(Box));
    return 0;
}
