#include <stdio.h>

#define W40_OBJECT_FN(x) ((x) * 2)
#define W40_OBJECT_EMIT 3 + W40_OBJECT_FN

int main(void) {
    printf("%d\n", W40_OBJECT_EMIT(5));
    return 0;
}
