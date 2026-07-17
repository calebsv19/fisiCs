#include <stdio.h>

int main(void) {
    int values[] = {0, 1, 2, 3, 1, 0};
    int i = 0;
    int total = 0;

    do {
        switch (values[i]) {
            case 0: {
                int first = i + 2;
                total += first;
            }
            case 1: {
                int second = i * 3 + 1;
                total += second;
                break;
            }
            case 2: {
                int third = i + 7;
                total += third;
                continue;
            }
            default: {
                int fourth = i * i;
                total += fourth;
            }
        }
        total += 20;
    } while (++i < 6);

    printf("%d\n", total);
    return 0;
}
