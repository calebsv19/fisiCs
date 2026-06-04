#include <inttypes.h>

uintmax_t wave34_inttypes_base_bridge(const char *text, int base);

int main(void) {
    uintmax_t value = wave34_inttypes_base_bridge("755#", 8);
    return value == UINTMAX_C(493) ? 0 : 1;
}
