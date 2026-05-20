#include <stdio.h>

int shared_count;

int bump_shared(void);

int main(void) {
    shared_count = 2;
    printf("%d %d\n", bump_shared(), shared_count);
    return 0;
}
