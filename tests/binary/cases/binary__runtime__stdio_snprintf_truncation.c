#include <stdio.h>

int main(void) {
    char buf[5];
    int n = snprintf(buf, sizeof(buf), "%s", "abcdef");
    printf("buf=%s n=%d\n", buf, n);
    return 0;
}
