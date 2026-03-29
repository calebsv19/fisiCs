#include <stdio.h>

int main(void) {
    int i = 0;
    int acc = 0;

    do {
        switch (i & 3) {
            case 0:
                acc += 1;
                ++i;
                continue;
            case 1:
                acc += 10;
                break;
            case 2:
                acc += 100;
                goto after_switch;
            default:
                acc += 1000;
                break;
        }

        acc += 5;
after_switch:
        acc += i;
        ++i;
    } while (i < 10);

    printf("%d\n", acc);
    return 0;
}
