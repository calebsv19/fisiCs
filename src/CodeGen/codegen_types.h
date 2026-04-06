// SPDX-License-Identifier: Apache-2.0

#ifndef CODEGEN_TYPES_H
#define CODEGEN_TYPES_H

#include "code_gen.h"
#include "Parser/Helpers/parsed_type.h"

#include <llvm-c/Core.h>

#ifdef __cplusplus
extern "C" {
#endif

LLVMTypeRef cg_type_from_parsed(CodegenContext* ctx, const ParsedType* type);

#ifdef __cplusplus
}
#endif

#endif // CODEGEN_TYPES_H
