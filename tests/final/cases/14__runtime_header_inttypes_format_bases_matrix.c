#include <inttypes.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    char out[96];
    int len = snprintf(out,
                       sizeof(out),
                       "%" PRIdMAX "|%" PRIuMAX "|%" PRIxMAX "|%" PRIoMAX "|%" PRIuPTR,
                       INTMAX_C(-8192),
                       UINTMAX_C(65535),
                       UINTMAX_C(0x2a),
                       UINTMAX_C(0755),
                       (uintptr_t)52u);

    printf("inttypes-format %s len=%d\n", out, len);
    return len == 21 && strcmp(out, "-8192|65535|2a|755|52") == 0 ? 0 : 1;
}
