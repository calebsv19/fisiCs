#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char *hex_text = "  -0x2Aq";
    const char *oct_text = "077tail";
    const char *bad_octal_text = "09tail";
    const char *empty_text = "xyz";
    char *end_hex = 0;
    char *end_oct = 0;
    char *end_bad_octal = 0;
    char *end_empty = 0;
    long hex_value = 0;
    long oct_value = 0;
    long bad_octal_value = 0;
    unsigned long empty_value = 0;
    long off_hex = 0;
    long off_oct = 0;
    long off_bad_octal = 0;
    long off_empty = 0;
    int empty_errno = 0;
    long summary = 0;

    errno = 0;
    hex_value = strtol(hex_text, &end_hex, 0);
    errno = 0;
    oct_value = strtol(oct_text, &end_oct, 0);
    errno = 0;
    bad_octal_value = strtol(bad_octal_text, &end_bad_octal, 0);
    errno = 0;
    empty_value = strtoul(empty_text, &end_empty, 10);
    empty_errno = errno;

    off_hex = (long)(end_hex - hex_text);
    off_oct = (long)(end_oct - oct_text);
    off_bad_octal = (long)(end_bad_octal - bad_octal_text);
    off_empty = (long)(end_empty - empty_text);
    summary = hex_value + oct_value + bad_octal_value + (long)empty_value +
              off_hex + off_oct + off_bad_octal + off_empty + empty_errno;

    printf("stdlib-base-auto hex=%ld off=%ld oct=%ld off=%ld bad=%ld off=%ld empty=%lu off=%ld errno=%d summary=%ld\n",
           hex_value,
           off_hex,
           oct_value,
           off_oct,
           bad_octal_value,
           off_bad_octal,
           empty_value,
           off_empty,
           empty_errno,
           summary);

    return hex_value == -42L && off_hex == 7L && oct_value == 63L &&
                   off_oct == 3L && bad_octal_value == 0L &&
                   off_bad_octal == 1L && empty_value == 0UL &&
                   off_empty == 0L && empty_errno == EINVAL && summary == 54L
               ? 0
               : 1;
}
