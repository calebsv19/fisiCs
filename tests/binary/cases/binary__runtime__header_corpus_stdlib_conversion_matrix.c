#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char *signed_text = " -17x";
    const char *unsigned_text = "255z";
    char *end_a = 0;
    char *end_b = 0;
    long a = strtol(signed_text, &end_a, 10);
    unsigned long b = strtoul(unsigned_text, &end_b, 10);
    long off_a = (long)(end_a - signed_text);
    long off_b = (long)(end_b - unsigned_text);
    long mix = labs(a) + labs(-42L) + (long)(b & 255UL);

    printf("stdlib-conv a=%ld off=%ld b=%lu off=%ld mix=%ld\n",
           a,
           off_a,
           b,
           off_b,
           mix);
    return a == -17L && off_a == 4L && b == 255UL && off_b == 3L && mix == 314L ? 0 : 1;
}
