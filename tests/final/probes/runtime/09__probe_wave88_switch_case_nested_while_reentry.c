#include <stdio.h>

static int route(int selector) {
    int i = 0;
    int score = 0;

    switch (selector) {
        while (i < 3) {
        case 1:
            score += ++i;
            if (i < 3)
                continue;
            break;
        case 2:
            score += 20;
            ++i;
            if (i < 2)
                continue;
            break;
        }
        score += 100;
    }

    return score;
}

int main(void) {
    printf("%d %d\n", route(1), route(2));
    return 0;
}
