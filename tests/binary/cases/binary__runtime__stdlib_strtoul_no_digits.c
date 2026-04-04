#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char* raw = "xyz";
    char* end = 0;
    unsigned long value;
    int consumed;

    value = strtoul(raw, &end, 10);
    if (!end) {
        return 1;
    }
    consumed = (int)(end - raw);
    if (value != 0ul || consumed != 0) {
        return 2;
    }

    printf("strtoul_no_digits_ok value=%lu consumed=%d\n", value, consumed);
    return 0;
}
