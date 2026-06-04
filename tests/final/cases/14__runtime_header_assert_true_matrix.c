#include <assert.h>
#include <stdio.h>

int main(void) {
    int value = 3;

    assert(++value == 4);
    assert(value < 10);

    printf("assert-true value=%d\n", value);
    return value == 4 ? 0 : 1;
}
