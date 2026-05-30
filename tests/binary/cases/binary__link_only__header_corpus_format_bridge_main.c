#include <limits.h>
#include <stddef.h>
#include <stdint.h>

extern size_t header_corpus_format_bridge(char* dst, size_t dst_size, const char* label, ...);

int main(void) {
    char buffer[64];
    size_t used = header_corpus_format_bridge(
        buffer,
        sizeof(buffer),
        "cap",
        (unsigned)UINT8_MAX,
        (size_t)sizeof(uintptr_t),
        (ptrdiff_t)offsetof(struct {
            uint32_t x;
            char y;
        }, y));
    return used == 0 ? 1 : 0;
}
