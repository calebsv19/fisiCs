#include <stdint.h>

struct Wave20StdintExactSurface {
    int8_t s8;
    uint8_t u8;
    int16_t s16;
    uint16_t u16;
    int32_t s32;
    uint32_t u32;
    int64_t s64;
    uint64_t u64;
};

uint64_t wave20_stdint_exact_surface(struct Wave20StdintExactSurface *out) {
    out->s8 = INT8_C(-12);
    out->u8 = UINT8_C(250);
    out->s16 = INT16_C(-32000);
    out->u16 = UINT16_C(64000);
    out->s32 = INT32_C(-1234567);
    out->u32 = UINT32_C(4000000000);
    out->s64 = INT64_C(-123456789);
    out->u64 = UINT64_C(123456789);
    return out->u64 + (uint64_t)out->u32 + (uint64_t)out->u16 + (uint64_t)out->u8;
}
