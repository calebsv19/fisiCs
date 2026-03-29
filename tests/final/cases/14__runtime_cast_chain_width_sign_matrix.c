#include <stdint.h>
#include <stdio.h>

int main(void) {
    int32_t s = -1234567;
    uint32_t us = (uint32_t)s;
    uint32_t pos = (uint32_t)(s + 2000000);
    uint16_t low = (uint16_t)us;

    printf("%u %u %u\n", us, pos, (unsigned int)low);
    return 0;
}
