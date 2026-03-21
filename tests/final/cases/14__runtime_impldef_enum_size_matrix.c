#include <stdio.h>

enum Small { S0 = 0, S1 = 1 };
enum Wide { W0 = 0, W1 = 0x7fffffff };

int main(void) {
    enum Small s = S1;
    enum Wide w = W1;

    printf("%zu %zu %d %d\n", sizeof(enum Small), sizeof(enum Wide), (int)s, (w > 0));
    return 0;
}
