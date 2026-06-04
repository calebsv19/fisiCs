#include <stddef.h>
#include <string.h>

struct Wave23StringMemorySurface {
    char buf[16];
    size_t len;
};

int wave23_string_memory_surface(struct Wave23StringMemorySurface *out, const char *src) {
    memset(out->buf, 0, sizeof(out->buf));
    memcpy(out->buf, src, 5);
    out->len = strlen(out->buf);
    return memcmp(out->buf, "alpha", 5) == 0 && out->len == 5;
}
