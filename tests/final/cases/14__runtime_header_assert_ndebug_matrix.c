#define NDEBUG
#include <assert.h>
#include <stdio.h>

int main(void) {
    int value = 3;

    assert(++value == 100);

    printf("assert-ndebug value=%d\n", value);
    return value == 3 ? 0 : 1;
}
