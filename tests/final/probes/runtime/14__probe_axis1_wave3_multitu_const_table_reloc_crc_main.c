#include <stdio.h>

extern const unsigned axis1_wave3_table[4];
extern unsigned axis1_wave3_fold(unsigned seed);

int main(void) {
    unsigned crc = axis1_wave3_fold(17u);
    printf("%u %u %u\n", crc, axis1_wave3_table[0], axis1_wave3_table[3]);
    return 0;
}
