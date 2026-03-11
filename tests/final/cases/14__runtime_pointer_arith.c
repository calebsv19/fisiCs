#include <stdio.h>

int main(void) {
    int arr[5] = {10, 20, 30, 40, 50};
    int *p = arr;
    int *q = arr + 3;
    printf("%d %d\n", (int)(q - p), *(p + 2));
    return 0;
}
