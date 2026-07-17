#include <stdio.h>

enum { WAVE33_CONDITIONAL_COMMA = 1 ? 7 : (0, 1) };

int main(void) {
    printf("%d\n", WAVE33_CONDITIONAL_COMMA);
    return 0;
}
