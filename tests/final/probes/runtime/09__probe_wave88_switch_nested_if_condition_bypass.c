#include <stdio.h>

static int side_effects;

static int route(int selector) {
    switch (selector) {
        if (++side_effects) {
        case 1:
            return side_effects;
        }
    }
    return 7;
}

int main(void) {
    int result = route(1);
    printf("%d %d\n", result, side_effects);
    return 0;
}
