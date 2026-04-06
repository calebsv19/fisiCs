// SPDX-License-Identifier: Apache-2.0

#ifndef TARGET_LAYOUT_H
#define TARGET_LAYOUT_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum {
    TL_ENDIAN_LITTLE = 0,
    TL_ENDIAN_BIG    = 1
} TargetEndianness;

typedef enum {
    TL_LONG_DOUBLE_64,
    TL_LONG_DOUBLE_80,
    TL_LONG_DOUBLE_128
} TargetLongDoubleKind;

typedef struct TargetLayout {
    /* sizes and aligns are in bits for size fields, bytes for align fields */
    size_t charBits, shortBits, intBits, longBits, longLongBits;
    size_t floatBits, doubleBits, longDoubleBits;
    size_t pointerBits;

    size_t charAlign, shortAlign, intAlign, longAlign, longLongAlign;
    size_t floatAlign, doubleAlign, longDoubleAlign;
    size_t pointerAlign;

    size_t maxAlign; /* max_align_t alignment in bytes */

    /* bitfield packing defaults */
    size_t bitfieldUnitBits;
    size_t bitfieldUnitAlign;

    TargetEndianness endianness;
    TargetLongDoubleKind longDoubleKind;

    /* Target calling convention / DLL storage support */
    bool supportsStdcall;
    bool supportsFastcall;
    bool supportsDllStorage;
} TargetLayout;

/* Returns a pointer to an internal static layout. Do not free. */
const TargetLayout* tl_default(void);
const TargetLayout* tl_from_triple(const char* triple);
/* Build an LLVM data layout string from a TargetLayout.
 * Caller owns the returned buffer (malloc). Returns NULL on failure. */
char* tl_to_llvm_datalayout(const TargetLayout* tl);
void tl_dump(const TargetLayout* tl, FILE* out);

#endif /* TARGET_LAYOUT_H */
