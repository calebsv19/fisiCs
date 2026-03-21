#include <stdio.h>

int main(void) {
    int sum = 0;

    for (int i = 0; i < 12; ++i) {
        if ((i % 3) == 0) {
            continue;
        }
        sum += i;
        if (sum > 30) {
            break;
        }
    }

    printf("%d\n", sum);
    return 0;
}
