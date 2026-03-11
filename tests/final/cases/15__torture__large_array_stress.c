#include <stdio.h>

int main(void) {
    int a[512];
    int i;
    int sum = 0;

    for (i = 0; i < 512; ++i) {
        a[i] = i % 13;
    }
    for (i = 0; i < 512; ++i) {
        sum += a[i];
    }

    printf("%d\n", sum);
    return 0;
}
