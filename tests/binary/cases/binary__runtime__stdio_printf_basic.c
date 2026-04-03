#include <stdio.h>

int main(void) {
    int wrote = printf("sum=%d hex=%x str=%s\n", 9, 0x2a, "ok");
    if (wrote <= 0) return 2;
    return 0;
}

