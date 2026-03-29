#include <stdio.h>

int main(void) {
    int acc = 17;

    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 7; ++j) {
            int lane = (i * 9 + j * 5 + acc) & 7;
            if (lane == 3) {
                acc += i * 13 - j;
                goto after_inner;
            }
            if ((lane & 1) == 0) {
                acc += lane + i - j;
            } else {
                acc ^= lane * 11 + j;
            }
        }
after_inner:
        acc += i * 3 + 1;
    }

    printf("%d\n", acc);
    return 0;
}
