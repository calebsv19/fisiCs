#include <stdio.h>

int main(void) {
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "%+08d|%-6u|%.5s", -42, 9u, "abcdef");

    if (n != 21) {
        return 1;
    }
    printf("buf=%s n=%d\n", buf, n);
    return 0;
}
