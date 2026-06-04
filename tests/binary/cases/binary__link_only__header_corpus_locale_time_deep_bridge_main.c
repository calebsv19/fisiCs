#include <string.h>

int wave32_locale_time_deep_bridge(char *out, unsigned long cap);

int main(void) {
    char out[32];
    int len = wave32_locale_time_deep_bridge(out, sizeof(out));
    return len == 14 && strcmp(out, "2026-01-04:004") == 0 ? 0 : 1;
}
