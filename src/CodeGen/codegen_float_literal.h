// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <llvm-c/Core.h>
#include <string.h>

/* Parse at the literal's semantic precision. Only trailing type/imaginary
 * suffixes are removed: f/F within a hexadecimal significand are digits.
 * LLVM's length-aware parser avoids host-double rounding and fixed buffers. */
static inline LLVMValueRef cg_float_literal_constant(LLVMTypeRef type,
                                                     const char* spelling) {
    const char* text = spelling ? spelling : "0";
    size_t length = strlen(text);
    if (length && strchr("iIjJ", text[length - 1])) --length;
    if (length && strchr("fFlL", text[length - 1])) --length;
    return LLVMConstRealOfStringAndSize(type, text, (unsigned)length);
}
