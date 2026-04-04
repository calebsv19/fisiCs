#include <stdio.h>

typedef unsigned char u8_base;
typedef u8_base u8_alias;

int main(void) {
    u8_alias value = 146;
    printf("%u\n", value);
    return 0;
}
