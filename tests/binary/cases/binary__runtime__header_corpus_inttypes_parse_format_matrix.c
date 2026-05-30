#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char *dec_tail = 0;
    char *hex_tail = 0;
    intmax_t dec_value = strtoimax("-4096z", &dec_tail, 10);
    uintmax_t hex_value = strtoumax("7b!", &hex_tail, 16);

    if (!dec_tail || *dec_tail != 'z' || dec_value != INTMAX_C(-4096)) {
        return 1;
    }
    if (!hex_tail || *hex_tail != '!' || hex_value != UINTMAX_C(123)) {
        return 2;
    }

    printf(
        "dec=%" PRIdMAX " tail=%c hex=%" PRIuMAX " htail=%c\n",
        dec_value,
        *dec_tail,
        hex_value,
        *hex_tail);
    return 0;
}
