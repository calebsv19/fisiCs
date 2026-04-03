#include <stdio.h>
#include <string.h>

int main(void) {
    const char* src = "abcDEF";
    char buf[8];

    memset(buf, 'x', sizeof(buf));
    memcpy(buf, src, 7);
    memmove(buf + 1, buf, 6);
    buf[7] = '\0';

    printf("%s\n", buf);
    return 0;
}
