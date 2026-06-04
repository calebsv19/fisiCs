#include <stddef.h>
#include <stdlib.h>

long wave26_stdlib_conversion_surface(const char *text) {
    char *end = 0;
    long a = strtol(text, &end, 10);
    unsigned long b = strtoul(end, &end, 16);
    double d = strtod(end, &end);
    int i = atoi("7");

    return a + (long)b + (long)d + i + (long)(end != 0);
}

int main(void) {
    return 0;
}
