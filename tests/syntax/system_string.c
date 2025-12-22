#include <string.h>

int main(void) {
    const char src[] = "abcd";
    char dst[8] = {0};
    size_t n = strlen(src);
    memcpy(dst, src, n + 1);
    return (int)(dst[0] + dst[3]);
}
