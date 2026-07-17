#include <stdio.h>

static int route(int selector) {
    int i = 0;
    int score = 0;

    switch (selector) {
        for (; i < 3; ++i) {
        case 1:
            score += 10;
            continue;
        case 2:
            score += 20;
            break;
        }
        score += 1;
    }

    return score;
}

int main(void) {
    printf("%d %d\n", route(1), route(2));
    return 0;
}
