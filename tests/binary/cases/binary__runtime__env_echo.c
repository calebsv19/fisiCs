#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char* value = getenv("BINARY_SMOKE_ENV");
    if (!value) {
        return 2;
    }
    printf("%s\n", value);
    return 0;
}

