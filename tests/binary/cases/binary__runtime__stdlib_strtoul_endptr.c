#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char* raw = "1234xyz";
    char* end = 0;
    unsigned long value = strtoul(raw, &end, 10);
    int consumed = 0;

    if (value != 1234ul) {
        return 1;
    }
    if (!end) {
        return 2;
    }
    consumed = (int)(end - raw);
    if (consumed != 4) {
        return 3;
    }

    printf("value=%lu tail=%s consumed=%d\n", value, end, consumed);
    return 0;
}
