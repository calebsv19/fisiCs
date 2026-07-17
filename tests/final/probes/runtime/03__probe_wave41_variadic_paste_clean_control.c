#include <stdio.h>

#define W41_CAT_RAW(left, right) left ## right
#define W41_CAT(left, right) W41_CAT_RAW(left, right)
#define W41_VARIADIC_PASTE(prefix, ...) W41_CAT(prefix, __VA_ARGS__)

static int w41_clean_value = 41;

int main(void) {
    printf("%d\n", W41_VARIADIC_PASTE(w41_clean_, value));
    return 0;
}
