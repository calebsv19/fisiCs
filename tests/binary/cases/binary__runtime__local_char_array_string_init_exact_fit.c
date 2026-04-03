#include <stdio.h>

int main(void) {
    char s[4] = "abc";
    printf("%s|%d\n", s, (int)sizeof(s));
    return 0;
}
