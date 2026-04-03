#include <stdarg.h>
#include <stdio.h>

static int wrap(char* dst, int size, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(dst, (size_t)size, fmt, ap);
    va_end(ap);
    return n;
}

int main(void) {
    char buf[16];
    int n = wrap(buf, (int)sizeof(buf), "%s-%d", "tag", 12);
    printf("buf=%s n=%d\n", buf, n);
    return 0;
}
