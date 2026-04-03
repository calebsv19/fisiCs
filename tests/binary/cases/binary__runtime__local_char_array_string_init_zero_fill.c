#include <stdio.h>

int main(void) {
    char s[8] = "abc";
    unsigned a = (unsigned char)s[3];
    unsigned b = (unsigned char)s[4];
    unsigned c = (unsigned char)s[7];
    int ok = (a == 0u && b == 0u && c == 0u) ? 1 : 0;
    printf("%u,%u,%u,%d\n", a, b, c, ok);
    return 0;
}
