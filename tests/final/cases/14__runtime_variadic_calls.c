#include <stdio.h>

int main(void) {
    int n1 = printf("%d %d %d\n", 1, 2, 3);
    int n2 = printf("%s %c %.1f\n", "ok", 'Z', 2.5);
    printf("%d %d\n", n1, n2);
    return 0;
}
