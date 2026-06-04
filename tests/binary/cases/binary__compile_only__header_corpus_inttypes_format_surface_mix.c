#include <inttypes.h>
#include <stdio.h>

int main(void) {
    char out[96];
    intmax_t signed_value = INTMAX_C(-8192);
    uintmax_t hex_value = UINTMAX_C(0x2a);
    uintptr_t ptr_value = (uintptr_t)0x34u;
    int len = snprintf(out, sizeof(out), "%" PRIdMAX ":%" PRIxMAX ":%" PRIuPTR, signed_value, hex_value, ptr_value);
    return len > 0 ? 0 : 1;
}
