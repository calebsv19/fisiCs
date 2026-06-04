#include <inttypes.h>
#include <stdlib.h>

uintmax_t wave34_inttypes_base_bridge(const char *text, int base) {
    char *tail = 0;
    uintmax_t value = strtoumax(text, &tail, base);
    if (!tail || *tail != '#') {
        return UINTMAX_C(0);
    }
    return value;
}
