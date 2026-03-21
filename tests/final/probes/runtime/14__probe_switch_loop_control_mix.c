#include <stdio.h>

int main(void) {
    int sum = 0;

    for (int i = 0; i < 10; ++i) {
        switch (i % 4) {
            case 0:
                sum += 1;
                continue;
            case 1:
                sum += 10;
                break;
            case 2:
                sum += 100;
                goto tail;
            default:
                sum += 1000;
                break;
        }
tail:
        sum += i;
    }

    printf("%d\n", sum);
    return 0;
}
