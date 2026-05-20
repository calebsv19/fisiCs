#include <stdio.h>

int helper(void) {
    return 22;
}

int call_local_helper(void);

int main(void) {
    printf("%d %d\n", call_local_helper(), helper());
    return 0;
}
