#include <stdio.h>

enum {
    A = 2,
    B = 5
};

static int score(int v) {
    switch (v) {
        case A + B:
            return 11;
        case (A << B) - 1:
            return 17;
        case (3 * 3) + 1:
            return 23;
        default:
            return 0;
    }
}

int main(void) {
    int total = 0;
    total += score(7);
    total += score(63);
    total += score(10);
    total += score(4);
    printf("%d\n", total);
    return 0;
}
