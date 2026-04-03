#include <stdio.h>

typedef struct {
    int a;
    int b;
} Pair;

static Pair make_pair(int a, int b) {
    Pair value = {a, b};
    return value;
}

int main(void) {
    Pair pair = make_pair(20, 22);
    printf("%d\n", pair.a + pair.b);
    return 0;
}

