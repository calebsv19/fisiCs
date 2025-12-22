#ifndef TARGET_LAYOUT_H
#define TARGET_LAYOUT_H

#include <stddef.h>
#include <stdbool.h>

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
} TargetLayout;

/* Returns a pointer to an internal static layout. Do not free. */
const TargetLayout* tl_default(void);
const TargetLayout* tl_from_triple(const char* triple);

#endif /* TARGET_LAYOUT_H */
