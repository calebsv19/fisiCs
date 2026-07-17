#include <stdio.h>

static int wave85_descendant_scope(void) {
    int retained = 9;

    goto wave85_inside;

    {
        int bypassed = 44;

wave85_inside:
        return retained + 7;
    }
}

int main(void) {
    int value = wave85_descendant_scope();
    printf("%d\n", value);
    return value != 16;
}
