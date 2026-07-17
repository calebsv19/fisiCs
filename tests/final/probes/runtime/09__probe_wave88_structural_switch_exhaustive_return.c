#include <stdio.h>

static int route(int selector) {
    switch (selector) {
        if (selector) {
        case 1:
            return 1;
        }
        default:
            return 2;
    }
}

int main(void) {
    printf("%d %d\n", route(1), route(9));
    return 0;
}
