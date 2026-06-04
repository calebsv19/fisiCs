#include <stddef.h>
#include <stdio.h>

int wave25_stdio_format_surface(char *dst, size_t cap, int value) {
    int (*puts_ptr)(const char *) = puts;
    int (*printf_ptr)(const char *, ...) = printf;
    int n = snprintf(dst, cap, "v=%d", value);

    return n + (puts_ptr != 0) + (printf_ptr != 0) + EOF + BUFSIZ + FILENAME_MAX;
}

int main(void) {
    char buf[16];
    return wave25_stdio_format_surface(buf, sizeof(buf), 7) < 0;
}
