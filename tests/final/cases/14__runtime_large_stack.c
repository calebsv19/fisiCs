#include <stdio.h>

int main(void) {
    int arr[256];
    for (int i = 0; i < 256; i++) {
        arr[i] = i;
    }
    int sum = 0;
    for (int i = 0; i < 256; i++) {
        sum += arr[i];
    }
    printf("%d\n", sum);
    return 0;
}
