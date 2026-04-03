#include <stdio.h>
#include <string.h>

int main(void) {
    char buf[16];
    memset(buf, 0, sizeof(buf));
    strncpy(buf, "toolong", 4);
    buf[4] = '\0';
    strncat(buf, "XYZ", 2);
    printf("%s\n", buf);
    return 0;
}
