#include <stdio.h>

#define W40_FUNCTION_FN(x) ((x) * 3)
#define W40_FUNCTION_EMIT() 4 + W40_FUNCTION_FN

int main(void) {
    printf("%d\n", W40_FUNCTION_EMIT()(6));
    return 0;
}
