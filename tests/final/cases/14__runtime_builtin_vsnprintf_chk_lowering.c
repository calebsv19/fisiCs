#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap) __builtin_va_end(ap)

extern int __builtin___vsnprintf_chk(char *dst,
                                     size_t dstlen,
                                     int flags,
                                     size_t objsz,
                                     const char *fmt,
                                     va_list ap);

static int append_checked(char *buffer, size_t capacity, size_t *io_offset, const char *fmt, ...) {
    if (!buffer || !io_offset || !fmt || *io_offset >= capacity) {
        return -1;
    }
    va_list args;
    va_start(args, fmt);
    int written = __builtin___vsnprintf_chk(buffer + *io_offset,
                                            capacity - *io_offset,
                                            0,
                                            (size_t)-1,
                                            fmt,
                                            args);
    va_end(args);
    if (written < 0) {
        return -2;
    }
    if (*io_offset + (size_t)written >= capacity) {
        return -3;
    }
    *io_offset += (size_t)written;
    return written;
}

int main(void) {
    char buffer[64];
    size_t offset = 0u;
    memset(buffer, 0, sizeof(buffer));

    if (append_checked(buffer, sizeof(buffer), &offset, "%s", "map") < 0) {
        puts("append1-failed");
        return 1;
    }
    if (append_checked(buffer, sizeof(buffer), &offset, "-%d", 7) < 0) {
        puts("append2-failed");
        return 1;
    }
    if (strcmp(buffer, "map-7") != 0 || offset != 5u) {
        printf("bad:%s:%zu\n", buffer, offset);
        return 1;
    }

    printf("ok:%s:%zu\n", buffer, offset);
    return 0;
}
