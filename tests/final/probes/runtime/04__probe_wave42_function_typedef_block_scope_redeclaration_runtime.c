#include <stdio.h>

typedef int Wave42BlockFunction(int value);

static int wave42_block_target(int value) {
    return value + 5;
}

int main(void) {
    Wave42BlockFunction wave42_block_target;
    printf("%d\n", wave42_block_target(7));
    return 0;
}
