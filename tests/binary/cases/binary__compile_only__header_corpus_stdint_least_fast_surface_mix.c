#include <stdint.h>

struct Wave20StdintLeastFastSurface {
    int_least8_t least8;
    uint_least16_t least16;
    int_fast32_t fast32;
    uint_fast64_t fast64;
};

uintmax_t wave20_stdint_least_fast_surface(struct Wave20StdintLeastFastSurface *out) {
    out->least8 = INT8_C(-7);
    out->least16 = UINT16_C(4096);
    out->fast32 = INT32_C(123456);
    out->fast64 = UINT64_C(987654321);
    return (uintmax_t)out->least16 + (uintmax_t)out->fast64 + (uintmax_t)(out->fast32 > 0);
}
