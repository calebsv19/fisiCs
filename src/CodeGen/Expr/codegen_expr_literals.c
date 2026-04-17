// SPDX-License-Identifier: Apache-2.0

#include "codegen_expr_internal.h"

#include "literal_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LLVMValueRef codegenNumberLiteral(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_NUMBER_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenNumberLiteral\n");
        return NULL;
    }

    const ParsedType* parsed = cg_resolve_expression_type(ctx, node);
    if (parsed && parsed->kind == TYPE_PRIMITIVE &&
        (parsed->primitiveType == TOKEN_FLOAT || parsed->primitiveType == TOKEN_DOUBLE)) {
        const char* text = node->valueNode.value ? node->valueNode.value : "0";
        char buf[128];
        size_t len = strlen(text);
        size_t outLen = 0;
        for (size_t i = 0; i < len && outLen + 1 < sizeof(buf); ++i) {
            char c = text[i];
            if (c == 'i' || c == 'I' || c == 'j' || c == 'J' ||
                c == 'f' || c == 'F' || c == 'l' || c == 'L') {
                continue;
            }
            buf[outLen++] = c;
        }
        buf[outLen] = '\0';
        double value = strtod(buf, NULL);
        LLVMTypeRef floatTy = cg_type_from_parsed(ctx, parsed);
        if (floatTy && LLVMGetTypeKind(floatTy) == LLVMStructTypeKind) {
            floatTy = LLVMDoubleTypeInContext(ctx->llvmContext);
        }
        if (!floatTy || LLVMGetTypeKind(floatTy) == LLVMVoidTypeKind) {
            floatTy = LLVMDoubleTypeInContext(ctx->llvmContext);
        }
        return LLVMConstReal(floatTy, value);
    }

    const ParsedType* parsedInt = cg_resolve_expression_type(ctx, node);
    LLVMTypeRef ty = parsedInt ? cg_type_from_parsed(ctx, parsedInt) : NULL;
    if (!ty || LLVMGetTypeKind(ty) == LLVMVoidTypeKind) {
        ty = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    IntegerLiteralInfo info = {0};
    unsigned long long value = 0;
    if (parse_integer_literal_info(node->valueNode.value ? node->valueNode.value : "0",
                                   cg_context_get_target_layout(ctx),
                                   &info) && info.ok) {
        value = info.value;
    }

    return LLVMConstInt(ty, value, (info.ok && !info.isUnsigned) ? 1 : 0);
}

LLVMValueRef codegenCharLiteral(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_CHAR_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenCharLiteral\n");
        return NULL;
    }

    const char* payload = NULL;
    ast_literal_encoding(node->valueNode.value, &payload);
    long long value = 0;
    if (payload) {
        char* endptr = NULL;
        value = strtoll(payload, &endptr, 0);
        if (endptr == payload) {
            value = (unsigned char)payload[0];
        }
    }
    LLVMTypeRef ty = LLVMInt32TypeInContext(ctx->llvmContext);
    return LLVMConstInt(ty, (unsigned long long)value, 0);
}

static LLVMValueRef cg_build_wide_string_global(CodegenContext* ctx,
                                                const char* rawPayload) {
    if (!ctx) return NULL;
    uint32_t* codepoints = NULL;
    size_t count = 0;
    LiteralDecodeResult res = decode_c_string_literal_wide(rawPayload ? rawPayload : "",
                                                           32,
                                                           &codepoints,
                                                           &count);
    (void)res;
    LLVMTypeRef elemTy = LLVMInt32TypeInContext(ctx->llvmContext);
    LLVMTypeRef arrayTy = LLVMArrayType(elemTy, (unsigned)(count + 1));
    LLVMValueRef* values = (LLVMValueRef*)calloc(count + 1, sizeof(LLVMValueRef));
    if (!values) {
        free(codepoints);
        return NULL;
    }
    for (size_t i = 0; i < count; ++i) {
        values[i] = LLVMConstInt(elemTy, codepoints ? codepoints[i] : 0, 0);
    }
    values[count] = LLVMConstInt(elemTy, 0, 0);

    static unsigned counter = 0;
    char name[32];
    snprintf(name, sizeof(name), ".wstr.%u", counter++);
    LLVMValueRef global = LLVMAddGlobal(ctx->module, arrayTy, name);
    LLVMSetLinkage(global, LLVMPrivateLinkage);
    LLVMSetInitializer(global, LLVMConstArray(elemTy, values, (unsigned)(count + 1)));
    LLVMSetGlobalConstant(global, true);
    LLVMSetUnnamedAddr(global, LLVMGlobalUnnamedAddr);

    LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
    LLVMValueRef indices[2] = { zero, zero };
    LLVMValueRef gep = LLVMConstGEP2(arrayTy, global, indices, 2);

    free(values);
    free(codepoints);
    return gep;
}

LLVMValueRef codegenStringLiteral(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_STRING_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenStringLiteral\n");
        return NULL;
    }

    const char* payload = NULL;
    LiteralEncoding enc = ast_literal_encoding(node->valueNode.value, &payload);
    if (enc == LIT_ENC_WIDE) {
        return cg_build_wide_string_global(ctx, payload ? payload : "");
    }

    char* decoded = NULL;
    size_t decodedLen = 0;
    LiteralDecodeResult res = decode_c_string_literal(payload ? payload : "", 8, &decoded, &decodedLen);
    (void)res;
    if (decoded) {
        LLVMValueRef strConst = LLVMConstStringInContext(ctx->llvmContext, decoded, (unsigned)decodedLen, false);
        LLVMTypeRef arrTy = LLVMTypeOf(strConst);
        static unsigned counter = 0;
        char name[32];
        snprintf(name, sizeof(name), ".str.%u", counter++);
        LLVMValueRef global = LLVMAddGlobal(ctx->module, arrTy, name);
        LLVMSetLinkage(global, LLVMPrivateLinkage);
        LLVMSetInitializer(global, strConst);
        LLVMSetGlobalConstant(global, true);
        LLVMSetUnnamedAddr(global, LLVMGlobalUnnamedAddr);
        LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
        LLVMValueRef indices[2] = { zero, zero };
        LLVMValueRef gep = LLVMConstGEP2(arrTy, global, indices, 2);
        free(decoded);
        return gep;
    }

    const char* text = payload ? payload : node->valueNode.value;
    return LLVMBuildGlobalStringPtr(ctx->builder, text ? text : "", "str");
}

LLVMValueRef codegenSizeof(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_SIZEOF) {
        fprintf(stderr, "Error: Invalid node type for codegenSizeof\n");
        return NULL;
    }

    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
    const ParsedType* parsed = NULL;
    if (node->expr.left) {
        ASTNode* operand = node->expr.left;
        if (operand->type == AST_STRING_LITERAL) {
            const char* payload = NULL;
            LiteralEncoding enc = ast_literal_encoding(operand->valueNode.value, &payload);
            const TargetLayout* tl = cg_context_get_target_layout(ctx);
            uint64_t elemSize = 1;
            unsigned charBits = (unsigned)((tl && tl->charBits) ? tl->charBits : 8);
            if (enc == LIT_ENC_WIDE) {
                unsigned wcharBits = charBits;
                const SemanticModel* model = cg_context_get_semantic_model(ctx);
                const Symbol* sym = model ? semanticModelLookupGlobal(model, "wchar_t") : NULL;
                if (sym && sym->kind == SYMBOL_TYPEDEF) {
                    LLVMTypeRef wcharTy = cg_type_from_parsed(ctx, &sym->type);
                    uint64_t sz = 0;
                    if (cg_size_align_for_type(ctx, &sym->type, wcharTy, &sz, NULL) && sz > 0) {
                        elemSize = sz;
                        wcharBits = (unsigned)(elemSize * 8);
                    }
                } else {
                    elemSize = (uint64_t)((charBits + 7) / 8);
                    wcharBits = (unsigned)(elemSize * 8);
                }

                LiteralDecodeResult res = decode_c_string_literal_wide(payload ? payload : "",
                                                                       (int)wcharBits,
                                                                       NULL,
                                                                       NULL);
                if (!res.ok) return NULL;
                return LLVMConstInt(intptrTy, (res.length + 1) * elemSize, 0);
            }

            LiteralDecodeResult res = decode_c_string_literal(payload ? payload : "",
                                                              (int)charBits,
                                                              NULL,
                                                              NULL);
            if (!res.ok) return NULL;
            elemSize = (uint64_t)((charBits + 7) / 8);
            if (!elemSize) elemSize = 1;
            return LLVMConstInt(intptrTy, (res.length + 1) * elemSize, 0);
        }
        if (operand->type == AST_PARSED_TYPE) {
            parsed = &operand->parsedTypeNode.parsed;
        } else {
            parsed = cg_resolve_expression_type(ctx, operand);
        }
    }

    if (parsed) {
        if (parsedTypeHasVLA(parsed)) {
            LLVMValueRef elementCount = NULL;
            if (node->expr.left && node->expr.left->type == AST_IDENTIFIER && ctx->currentScope) {
                NamedValue* named = cg_scope_lookup(ctx->currentScope, node->expr.left->valueNode.value);
                if (named && named->vlaElementCount) {
                    elementCount = named->vlaElementCount;
                }
            }
            if (!elementCount) {
                elementCount = computeVLAElementCount(ctx, parsed);
            }
            if (!elementCount) return NULL;
            elementCount = ensureIntegerLike(ctx, elementCount);
            if (!elementCount) return NULL;
            LLVMTypeRef elemType = vlaInnermostElementLLVM(ctx, parsed);
            unsigned long long elemSize = LLVMABISizeOfType(LLVMGetModuleDataLayout(ctx->module), elemType);
            LLVMValueRef elemSizeVal = LLVMConstInt(intptrTy, elemSize, 0);
            return LLVMBuildMul(ctx->builder, elementCount, elemSizeVal, "sizeof.vla");
        }

        LLVMTypeRef ty = cg_type_from_parsed(ctx, parsed);
        if (!ty || LLVMGetTypeKind(ty) == LLVMVoidTypeKind) {
            ty = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        uint64_t semanticSize = 0;
        if (cg_size_align_for_type(ctx, parsed, ty, &semanticSize, NULL) && semanticSize > 0) {
            return LLVMConstInt(intptrTy, semanticSize, 0);
        }
        unsigned long long abiSize = LLVMABISizeOfType(LLVMGetModuleDataLayout(ctx->module), ty);
        return LLVMConstInt(intptrTy, abiSize, 0);
    }

    LLVMTypeRef type = LLVMInt32TypeInContext(ctx->llvmContext);
    if (node->expr.left) {
        ASTNode* operand = node->expr.left;
        LLVMValueRef value = codegenNode(ctx, operand);
        if (value) {
            type = LLVMTypeOf(value);
        }
    }

    return LLVMSizeOf(type);
}

LLVMValueRef codegenAlignof(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_ALIGNOF) {
        fprintf(stderr, "Error: Invalid node type for codegenAlignof\n");
        return NULL;
    }
    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
    const ParsedType* parsed = NULL;
    if (node->expr.left) {
        ASTNode* operand = node->expr.left;
        if (operand->type == AST_PARSED_TYPE) {
            parsed = &operand->parsedTypeNode.parsed;
        } else {
            parsed = cg_resolve_expression_type(ctx, operand);
        }
    }
    if (parsed) {
        LLVMTypeRef ty = NULL;
        if (parsedTypeHasVLA(parsed)) {
            ty = vlaInnermostElementLLVM(ctx, parsed);
        } else {
            ty = cg_type_from_parsed(ctx, parsed);
        }
        if (!ty || LLVMGetTypeKind(ty) == LLVMVoidTypeKind) {
            ty = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        unsigned align = LLVMABIAlignmentOfType(LLVMGetModuleDataLayout(ctx->module), ty);
        return LLVMConstInt(intptrTy, align, 0);
    }
    LLVMTypeRef type = LLVMInt32TypeInContext(ctx->llvmContext);
    if (node->expr.left) {
        LLVMValueRef value = codegenNode(ctx, node->expr.left);
        if (value) type = LLVMTypeOf(value);
    }
    unsigned align = LLVMABIAlignmentOfType(LLVMGetModuleDataLayout(ctx->module), type);
    return LLVMConstInt(intptrTy, align, 0);
}

LLVMValueRef codegenHeapAllocation(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_HEAP_ALLOCATION) {
        fprintf(stderr, "Error: Invalid node type for codegenHeapAllocation\n");
        return NULL;
    }

    LLVMTypeRef allocType = LLVMStructType(NULL, 0, 0);
    LLVMValueRef size = LLVMSizeOf(allocType);
    LLVMValueRef mallocFunc = LLVMGetNamedFunction(ctx->module, "malloc");

    if (!mallocFunc) {
        fprintf(stderr, "Error: malloc function not found\n");
        return NULL;
    }

    LLVMValueRef args[] = {size};
    return LLVMBuildCall2(ctx->builder, LLVMTypeOf(mallocFunc), mallocFunc, args, 1, "mallocCall");
}
