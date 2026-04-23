#include <stddef.h>
#include <stdio.h>

static unsigned rebase_sum(unsigned* restrict dst,
                           const unsigned* restrict src,
                           size_t n,
                           unsigned bias) {
    unsigned fold = bias * 29u + 7u;
    size_t i;

    for (i = 0u; i < n; ++i) {
        dst[i] = src[i] + bias + (unsigned)i * 3u;
        fold = fold * 131u + dst[i];
    }
    return fold;
}

int main(void) {
    unsigned src[8];
    unsigned dst[8];
    unsigned fold;
    unsigned check = 0u;
    unsigned char* bytes;
    size_t i;

    for (i = 0u; i < 8u; ++i) {
        src[i] = 100u + (unsigned)i * 17u;
    }

    fold = rebase_sum(dst, src, 8u, 9u);

    bytes = (unsigned char*)&dst[2];
    bytes[0] = (unsigned char)(bytes[0] ^ 0x3Cu);
    bytes = (unsigned char*)&dst[5];
    bytes[0] = (unsigned char)(bytes[0] + 7u);

    for (i = 0u; i < 8u; ++i) {
        check = check * 167u + dst[i];
    }

    printf("%u %u %u %u\n", fold, check, dst[2], dst[5]);
    return 0;
}
