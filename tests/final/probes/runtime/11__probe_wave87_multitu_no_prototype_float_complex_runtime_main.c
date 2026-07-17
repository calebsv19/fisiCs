#include <stdio.h>
#include <string.h>

typedef float _Complex Wave87Complex;

int wave87_complex_checksum();

int main(void) {
    float source_lanes[2] = {1.5f, -2.25f};
    Wave87Complex value;
    int checksum;

    memcpy(&value, source_lanes, sizeof(value));
    checksum = wave87_complex_checksum(value);
    printf("%d\n", checksum);
    return checksum == 591 ? 0 : 1;
}
