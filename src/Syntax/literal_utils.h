#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    bool ok;         // false when an escape/universal sequence is malformed
    bool overflow;   // true when a code point exceeds the target character width
    size_t length;   // number of code points (not counting the implicit terminator)
} LiteralDecodeResult;

/**
 * Decode a C string literal payload (without surrounding quotes or any wide/UTF
 * prefix). The parser token still carries the raw spelling; this helper walks
 * escapes to recover code points and flag malformed sequences.
 *
 * charBitWidth controls the representable range; values larger than that range
 * flip the overflow flag. When outBytes is non-NULL and charBitWidth <= 8, an
 * allocated buffer containing the decoded bytes plus a '\0' terminator is
 * produced; caller owns the returned buffer.
 */
LiteralDecodeResult decode_c_string_literal(const char* raw,
                                            int charBitWidth,
                                            char** outBytes,
                                            size_t* outByteLen);
