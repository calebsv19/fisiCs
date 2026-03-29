#include <stdio.h>

int main(void) {
    int data[40];
    for (int i = 0; i < 40; ++i) {
        data[i] = i * 7 - 9;
    }

    int *left = &data[5];
    int *right = &data[29];
    int span = (int)(right - left);
    int acc = 0;

    for (int i = 0; i < 10; ++i) {
        int a = left[i];
        int b = right[-i];
        if ((i & 1) == 0) {
            acc += a * 3 + b;
        } else {
            acc += a - b * 2;
        }
    }

    printf("%d %d %d\n", span, left[3], acc);
    return 0;
}
