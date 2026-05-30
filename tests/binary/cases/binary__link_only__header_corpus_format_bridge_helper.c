#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

size_t header_corpus_format_bridge(char* dst, size_t dst_size, const char* label, ...) {
    va_list ap;
    unsigned byte_cap = 0;
    size_t width = 0;
    ptrdiff_t offset = 0;
    int wrote = 0;

    if (!dst || dst_size == 0 || !label) {
        return 0;
    }

    va_start(ap, label);
    byte_cap = va_arg(ap, unsigned);
    width = va_arg(ap, size_t);
    offset = va_arg(ap, ptrdiff_t);
    va_end(ap);

    wrote = snprintf(dst, dst_size, "%s:%u:%zu:%td", label, byte_cap, width, offset);
    if (wrote < 0) {
        return 0;
    }
    return strlen(dst);
}
