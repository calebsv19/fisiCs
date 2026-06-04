#include <stdio.h>
#include <string.h>

int wave25_stdio_bridge_score(const char *label, int value) {
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "%s:%d", label, value);

    return n + (int)(unsigned char)buf[0] + (int)(unsigned char)buf[4];
}
