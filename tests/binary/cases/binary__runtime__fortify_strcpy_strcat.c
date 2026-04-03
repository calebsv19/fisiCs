#include <stdio.h>
#include <string.h>

int main(void) {
    char buf[32];
    strcpy(buf, "base");
    strcat(buf, "-ok");
    printf("%s\n", buf);
    return 0;
}
