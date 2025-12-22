#include <stdlib.h>

struct Pair { int a; int b; };

int main(void) {
    struct Pair p = { .a = 1, .b = 2 };
    int *arr = (int *)malloc(4 * (int)sizeof(int));
    if (!arr) {
        return 1;
    }
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;
    int sum = arr[2] + abs(-5);
    free(arr);
    return sum + p.a + p.b;
}
