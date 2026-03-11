#include <stdio.h>

int tri(int n) {
    if (n <= 0) {
        return 0;
    }
    return n + tri(n - 1);
}

int main(void) {
    printf("%d\n", tri(300));
    return 0;
}
