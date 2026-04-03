#include <stdio.h>

char g[8] = "xyz";

int main(void) {
    printf("%s|%u\n", g, (unsigned char)g[4]);
    return 0;
}
