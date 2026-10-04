// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <llvm-c/Core.h>
#include "Syntax/literal_utils.h"

/* Parse at the literal's semantic precision. Only trailing type/imaginary
 * suffixes are removed: f/F within a hexadecimal significand are digits.
 * LLVM's length-aware parser avoids host-double rounding and fixed buffers. */
static inline LLVMValueRef cg_float_literal_constant(LLVMTypeRef type,
                                                     const char* spelling) {
    const char* text = spelling ? spelling : "0";
    FloatingLiteralSuffix suffix = parse_floating_literal_suffix(text);
    return LLVMConstRealOfStringAndSize(type, text, (unsigned)suffix.coreLength);
}
