#include <stdio.h>

static int route(int selector) {
    switch (selector)
        case 3:
            return 30;
    return 7;
}

int main(void) {
    printf("%d %d\n", route(3), route(4));
    return 0;
}
