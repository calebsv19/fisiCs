#include <stdio.h>

enum Wide {
    WIDE_ZERO = 0,
    WIDE_MAX = 0x7fffffff
};

int main(void) {
    enum Wide e = (enum Wide)-1;
    int is_negative = (e < 0);
    printf("%zu %d\n", sizeof(enum Wide), is_negative);
    return 0;
}
