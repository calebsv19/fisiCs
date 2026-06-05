#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char *signed_text = " -9223372036854775807:";
    const char *unsigned_text = "0xffffffffffffffff!";
    const char *float_text = " +12.75xyz";
    char *tail_signed = 0;
    char *tail_unsigned = 0;
    char *tail_float = 0;
    long long signed_value = strtoll(signed_text, &tail_signed, 10);
    unsigned long long unsigned_value = strtoull(unsigned_text, &tail_unsigned, 0);
    double float_value = strtod(float_text, &tail_float);
    long off_signed = (long)(tail_signed - signed_text);
    long off_unsigned = (long)(tail_unsigned - unsigned_text);
    long off_float = (long)(tail_float - float_text);
    long scaled_float = (long)(float_value * 100.0 + 0.5);
    unsigned long long low_mask = unsigned_value & 255ULL;

    printf("stdlib-wide signed_tail=%ld signed_mod=%lld unsigned_tail=%ld low=%llu float_tail=%ld scaled=%ld\n",
           off_signed,
           signed_value % 1000LL,
           off_unsigned,
           low_mask,
           off_float,
           scaled_float);

    return off_signed == 21L && signed_value % 1000LL == -807LL &&
                   off_unsigned == 18L && low_mask == 255ULL && off_float == 7L &&
                   scaled_float == 1275L
               ? 0
               : 1;
}
