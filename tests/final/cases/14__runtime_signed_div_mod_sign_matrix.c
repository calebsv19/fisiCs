#include <stdio.h>

int main(void) {
    int q1 = 7 / 3;
    int r1 = 7 % 3;
    int q2 = -7 / 3;
    int r2 = -7 % 3;
    int q3 = 7 / -3;
    int r3 = 7 % -3;
    int q4 = -7 / -3;
    int r4 = -7 % -3;

    int ok = (q1 == 2 && r1 == 1 && q2 == -2 && r2 == -1 &&
              q3 == -2 && r3 == 1 && q4 == 2 && r4 == -1);

    printf("%d %d %d %d %d %d %d %d %d\n", ok, q1, r1, q2, r2, q3, r3, q4, r4);
    return 0;
}
