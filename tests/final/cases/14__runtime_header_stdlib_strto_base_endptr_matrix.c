#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char *auto_text = "  -0x2a!";
    const char *oct_text = "0755/";
    const char *bin_like = "0b101";
    char *tail_a = 0;
    char *tail_b = 0;
    char *tail_c = 0;
    long auto_value = strtol(auto_text, &tail_a, 0);
    unsigned long oct_value = strtoul(oct_text, &tail_b, 0);
    long bin_value = strtol(bin_like, &tail_c, 0);
    long off_a = (long)(tail_a - auto_text);
    long off_b = (long)(tail_b - oct_text);
    long off_c = (long)(tail_c - bin_like);
    long summary = labs(auto_value) + (long)oct_value + bin_value + off_a + off_b + off_c;

    printf("stdlib-strto-base auto=%ld off=%ld oct=%lu off=%ld bin=%ld off=%ld summary=%ld\n",
           auto_value,
           off_a,
           oct_value,
           off_b,
           bin_value,
           off_c,
           summary);

    return auto_value == -42L && off_a == 7L && oct_value == 493UL && off_b == 4L &&
                   bin_value == 0L && off_c == 1L && summary == 547L
               ? 0
               : 1;
}
