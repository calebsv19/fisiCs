#include "target_layout.h"
#include <string.h>

static const TargetLayout layout_lp64_fp80 = {
    .charBits = 8,  .shortBits = 16, .intBits = 32, .longBits = 64, .longLongBits = 64,
    .floatBits = 32, .doubleBits = 64, .longDoubleBits = 80,
    .pointerBits = 64,
    .charAlign = 1, .shortAlign = 2, .intAlign = 4, .longAlign = 8, .longLongAlign = 8,
    .floatAlign = 4, .doubleAlign = 8, .longDoubleAlign = 16, /* typical sysv fp80 */
    .pointerAlign = 8,
    .maxAlign = 16,
    .bitfieldUnitBits = 32,
    .bitfieldUnitAlign = 4,
    .endianness = TL_ENDIAN_LITTLE,
    .longDoubleKind = TL_LONG_DOUBLE_80
};

static const TargetLayout layout_lp64_fp128 = {
    .charBits = 8,  .shortBits = 16, .intBits = 32, .longBits = 64, .longLongBits = 64,
    .floatBits = 32, .doubleBits = 64, .longDoubleBits = 128,
    .pointerBits = 64,
    .charAlign = 1, .shortAlign = 2, .intAlign = 4, .longAlign = 8, .longLongAlign = 8,
    .floatAlign = 4, .doubleAlign = 8, .longDoubleAlign = 16,
    .pointerAlign = 8,
    .maxAlign = 16,
    .bitfieldUnitBits = 32,
    .bitfieldUnitAlign = 4,
    .endianness = TL_ENDIAN_LITTLE,
    .longDoubleKind = TL_LONG_DOUBLE_128
};

static const TargetLayout layout_llp64_fp64 = {
    .charBits = 8,  .shortBits = 16, .intBits = 32, .longBits = 32, .longLongBits = 64,
    .floatBits = 32, .doubleBits = 64, .longDoubleBits = 64,
    .pointerBits = 64,
    .charAlign = 1, .shortAlign = 2, .intAlign = 4, .longAlign = 4, .longLongAlign = 8,
    .floatAlign = 4, .doubleAlign = 8, .longDoubleAlign = 8,
    .pointerAlign = 8,
    .maxAlign = 8,
    .bitfieldUnitBits = 32,
    .bitfieldUnitAlign = 4,
    .endianness = TL_ENDIAN_LITTLE,
    .longDoubleKind = TL_LONG_DOUBLE_64
};

const TargetLayout* tl_default(void) {
    return &layout_lp64_fp80;
}

const TargetLayout* tl_from_triple(const char* triple) {
    if (!triple || !triple[0]) {
        return tl_default();
    }
    if (strstr(triple, "win32") || strstr(triple, "windows") || strstr(triple, "mingw")) {
        return &layout_llp64_fp64;
    }
    if (strstr(triple, "x86_64") || strstr(triple, "amd64")) {
        /* Assume SysV x86_64: fp80 */
        return &layout_lp64_fp80;
    }
    if (strstr(triple, "aarch64") || strstr(triple, "arm64")) {
        /* Most aarch64 toolchains use 128-bit long double (quad) */
        return &layout_lp64_fp128;
    }
    if (strstr(triple, "riscv64")) {
        return &layout_lp64_fp128;
    }
    return tl_default();
}
