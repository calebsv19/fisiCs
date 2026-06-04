#include <stdio.h>
#include <string.h>

int main(void) {
    char dst[24];
    int cmp;
    size_t len;

    memset(dst, 'x', sizeof(dst));
    memcpy(dst, "matrix", 6);
    memmove(dst + 6, dst, 6);
    dst[12] = '\0';
    cmp = memcmp(dst, "matrixmatrix", 12);
    len = strlen(dst);

    printf("string-copy-compare len=%lu cmp=%d text=%s\n", (unsigned long)len, cmp, dst);
    return len == 12 && cmp == 0 ? 0 : 1;
}
