#include "target_layout.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>

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
    .longDoubleKind = TL_LONG_DOUBLE_80,
    .supportsStdcall = false,
    .supportsFastcall = false,
    .supportsDllStorage = false
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
    .longDoubleKind = TL_LONG_DOUBLE_128,
    .supportsStdcall = false,
    .supportsFastcall = false,
    .supportsDllStorage = false
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
    .longDoubleKind = TL_LONG_DOUBLE_64,
    .supportsStdcall = true,
    .supportsFastcall = true,
    .supportsDllStorage = true
};

static const TargetLayout layout_lp64_fp64 = {
    .charBits = 8,  .shortBits = 16, .intBits = 32, .longBits = 64, .longLongBits = 64,
    .floatBits = 32, .doubleBits = 64, .longDoubleBits = 64,
    .pointerBits = 64,
    .charAlign = 1, .shortAlign = 2, .intAlign = 4, .longAlign = 8, .longLongAlign = 8,
    .floatAlign = 4, .doubleAlign = 8, .longDoubleAlign = 8,
    .pointerAlign = 8,
    .maxAlign = 16,
    .bitfieldUnitBits = 32,
    .bitfieldUnitAlign = 4,
    .endianness = TL_ENDIAN_LITTLE,
    .longDoubleKind = TL_LONG_DOUBLE_64,
    .supportsStdcall = false,
    .supportsFastcall = false,
    .supportsDllStorage = false
};

const TargetLayout* tl_default(void) {
#if defined(_WIN64) || defined(__WIN64__)
    return &layout_llp64_fp64;
#elif defined(__aarch64__) || defined(__arm64__)
#if defined(__APPLE__)
    /* Apple arm64 uses 64-bit long double (same as double). */
    return &layout_lp64_fp64;
#else
    return &layout_lp64_fp128;
#endif
#elif defined(__x86_64__) || defined(_M_X64)
    return &layout_lp64_fp80;
#elif defined(__riscv) && (__riscv_xlen == 64)
    return &layout_lp64_fp128;
#else
    return &layout_lp64_fp80;
#endif
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
        /* Apple arm64 matches clang's 64-bit long double ABI. */
        if (strstr(triple, "apple") || strstr(triple, "darwin")) {
            return &layout_lp64_fp64;
        }
        /* Most non-Apple aarch64 toolchains use 128-bit long double (quad). */
        return &layout_lp64_fp128;
    }
    if (strstr(triple, "riscv64")) {
        return &layout_lp64_fp128;
    }
    return tl_default();
}

static void append_field(char** cursor, size_t* remaining, const char* fmt, ...) {
    if (!cursor || !*cursor || !remaining || *remaining == 0) return;
    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(*cursor, *remaining, fmt, args);
    va_end(args);
    if (written <= 0) return;
    if ((size_t)written >= *remaining) {
        *cursor += *remaining;
        *remaining = 0;
    } else {
        *cursor += written;
        *remaining -= (size_t)written;
    }
}

char* tl_to_llvm_datalayout(const TargetLayout* tl) {
    if (!tl) return NULL;
    /* Compose a conservative layout string covering endianness, pointer,
     * integer widths, floats, aggregate align, native widths. */
    char buffer[256];
    char* cur = buffer;
    size_t remain = sizeof(buffer);
    append_field(&cur, &remain, "%c", tl->endianness == TL_ENDIAN_BIG ? 'E' : 'e');

    size_t pBits = tl->pointerBits ? tl->pointerBits : 64;
    size_t pAlign = tl->pointerAlign ? tl->pointerAlign * 8 : pBits;
    append_field(&cur, &remain, "-p:%zu:%zu:%zu", pBits, pAlign, pAlign);

    /* Integer alignments */
    size_t i8a  = tl->charAlign   ? tl->charAlign * 8   : 8;
    size_t i16a = tl->shortAlign  ? tl->shortAlign * 8  : 16;
    size_t i32a = tl->intAlign    ? tl->intAlign * 8    : 32;
    size_t i64a = tl->longLongAlign ? tl->longLongAlign * 8 : 64;
    append_field(&cur, &remain, "-i8:%zu", i8a);
    append_field(&cur, &remain, "-i16:%zu", i16a);
    append_field(&cur, &remain, "-i32:%zu", i32a);
    append_field(&cur, &remain, "-i64:%zu", i64a);

    /* Float alignments */
    size_t f32a = tl->floatAlign  ? tl->floatAlign * 8  : 32;
    size_t f64a = tl->doubleAlign ? tl->doubleAlign * 8 : 64;
    append_field(&cur, &remain, "-f32:%zu", f32a);
    append_field(&cur, &remain, "-f64:%zu", f64a);
    if (tl->longDoubleBits) {
        size_t flda = tl->longDoubleAlign ? tl->longDoubleAlign * 8 : tl->longDoubleBits;
        append_field(&cur, &remain, "-f%zu:%zu", tl->longDoubleBits, flda);
    }

    size_t aggAlign = tl->maxAlign ? tl->maxAlign * 8 : 0;
    if (aggAlign) {
        append_field(&cur, &remain, "-a:0:%zu", aggAlign);
    }

    /* Native integer widths */
    append_field(&cur, &remain, "-n8:16:32:64");

    size_t used = (size_t)(cur - buffer);
    char* out = (char*)malloc(used + 1);
    if (!out) return NULL;
    memcpy(out, buffer, used);
    out[used] = '\0';
    return out;
}

void tl_dump(const TargetLayout* tl, FILE* out) {
    if (!tl) tl = tl_default();
    if (!out) out = stdout;
    fprintf(out,
            "Target layout:\n"
            "  endianness: %s\n"
            "  char/short/int/long/long long bits: %zu/%zu/%zu/%zu/%zu\n"
            "  float/double/long double bits: %zu/%zu/%zu (kind=%d)\n"
            "  pointer bits: %zu\n"
            "  align: char=%zu short=%zu int=%zu long=%zu long long=%zu ptr=%zu\n"
            "         float=%zu double=%zu long double=%zu max=%zu\n"
            "  bitfield unit: bits=%zu align=%zu\n"
            "  interop: stdcall=%s fastcall=%s dllstorage=%s\n",
            tl->endianness == TL_ENDIAN_BIG ? "big" : "little",
            tl->charBits, tl->shortBits, tl->intBits, tl->longBits, tl->longLongBits,
            tl->floatBits, tl->doubleBits, tl->longDoubleBits, (int)tl->longDoubleKind,
            tl->pointerBits,
            tl->charAlign, tl->shortAlign, tl->intAlign, tl->longAlign, tl->longLongAlign, tl->pointerAlign,
            tl->floatAlign, tl->doubleAlign, tl->longDoubleAlign, tl->maxAlign,
            tl->bitfieldUnitBits, tl->bitfieldUnitAlign,
            tl->supportsStdcall ? "yes" : "no",
            tl->supportsFastcall ? "yes" : "no",
            tl->supportsDllStorage ? "yes" : "no");
}
