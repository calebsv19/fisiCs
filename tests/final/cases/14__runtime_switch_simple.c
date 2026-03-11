#include <stdio.h>

int f(int x) {
    switch (x) {
        case 0:
            return 10;
        case 1:
            return 20;
        case 2:
            return 30;
        default:
            return 40;
    }
}

int main(void) {
    printf("%d %d %d %d\n", f(0), f(1), f(2), f(9));
    return 0;
}
