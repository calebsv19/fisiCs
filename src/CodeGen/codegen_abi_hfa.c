// SPDX-License-Identifier: Apache-2.0
#include "codegen_private.h"
#include <llvm-c/TargetMachine.h>
#include <string.h>
static bool members(LLVMTypeRef t, LLVMTypeRef *scalar, unsigned *count,
                    unsigned depth) {
  if (depth > 8)
    return false;
  LLVMTypeKind k = LLVMGetTypeKind(t);
  if (k == LLVMFloatTypeKind || k == LLVMDoubleTypeKind) {
    if (*scalar && *scalar != t)
      return false;
    *scalar = t;
    return ++*count <= 4;
  }
  if (k == LLVMArrayTypeKind) {
    unsigned n = LLVMGetArrayLength(t);
    if (!n || n > 4)
      return false;
    for (unsigned i = 0; i < n; i++)
      if (!members(LLVMGetElementType(t), scalar, count, depth + 1))
        return false;
    return true;
  }
  if (k != LLVMStructTypeKind || LLVMIsOpaqueStruct(t) || LLVMIsPackedStruct(t))
    return false;
  unsigned n = LLVMCountStructElementTypes(t);
  if (!n || n > 4)
    return false;
  for (unsigned i = 0; i < n; i++)
    if (!members(LLVMStructGetTypeAtIndex(t, i), scalar, count, depth + 1))
      return false;
  return true;
}
bool cg_aarch64_hfa(CodegenContext *ctx, LLVMTypeRef t) {
  if (!ctx || !ctx->module || !t)
    return false;
  const char *target = LLVMGetTarget(ctx->module);
  /* Native emission supplies the default triple after IR construction. Use
   * that same target when no explicit cross target was configured. */
  char *native = (!target || !*target) ? LLVMGetDefaultTargetTriple() : NULL;
  if (native)
    target = native;
  bool arm = target &&
             (!strncmp(target, "aarch64", 7) || !strncmp(target, "arm64", 5));
  if (native)
    LLVMDisposeMessage(native);
  if (!arm)
    return false;
  LLVMTypeKind k = LLVMGetTypeKind(t);
  if (k != LLVMStructTypeKind && k != LLVMArrayTypeKind)
    return false;
  LLVMTypeRef scalar = NULL;
  unsigned count = 0;
  return members(t, &scalar, &count, 0) && count > 0;
}
