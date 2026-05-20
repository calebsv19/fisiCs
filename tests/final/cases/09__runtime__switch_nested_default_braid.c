#include <stdio.h>

static int fold(int outer, int inner) {
    int out = 0;

    switch (outer) {
        case 0:
            out += 1;
            switch (inner) {
                case 0:
                    out += 10;
                    break;
                default:
                    out += 20;
                    break;
            }
            break;
        case 1:
            out += 3;
            switch (inner) {
                case 1:
                    out += 30;
                    break;
                default:
                    out += 40;
                    break;
            }
            break;
        default:
            out += 5;
            switch (inner) {
                case 2:
                    out += 50;
                    break;
                default:
                    out += 60;
                    break;
            }
            break;
    }

    return out;
}

int main(void) {
    int total = 0;

    total += fold(0, 0);
    total += fold(0, 9);
    total += fold(1, 1);
    total += fold(1, 0);
    total += fold(2, 2);
    total += fold(2, 0);

    printf("%d\n", total);
    return 0;
}
