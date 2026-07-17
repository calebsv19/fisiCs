#include <stdio.h>

enum { WAVE33_LOGICAL_AND_COMMA = 0 && (0, 1) };

int main(void) {
    printf("%d\n", WAVE33_LOGICAL_AND_COMMA);
    return 0;
}
