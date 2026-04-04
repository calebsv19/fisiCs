#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char* raw_hex = "0x2Arest";
    const char* raw_oct = "052!";
    const char* raw_dec = "-15z";
    char* end_hex = 0;
    char* end_oct = 0;
    char* end_dec = 0;
    long v_hex = strtol(raw_hex, &end_hex, 0);
    long v_oct = strtol(raw_oct, &end_oct, 0);
    long v_dec = strtol(raw_dec, &end_dec, 10);
    int c_hex;
    int c_oct;
    int c_dec;

    if (v_hex != 42L || v_oct != 42L || v_dec != -15L) {
        return 1;
    }
    if (!end_hex || !end_oct || !end_dec) {
        return 2;
    }

    c_hex = (int)(end_hex - raw_hex);
    c_oct = (int)(end_oct - raw_oct);
    c_dec = (int)(end_dec - raw_dec);
    if (c_hex != 4 || c_oct != 3 || c_dec != 3) {
        return 3;
    }

    printf("strtol_base_matrix_ok hex=%ld oct=%ld dec=%ld consumed=%d/%d/%d\n",
           v_hex, v_oct, v_dec, c_hex, c_oct, c_dec);
    return 0;
}
