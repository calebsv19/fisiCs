// SPDX-License-Identifier: Apache-2.0

#include "codegen_initializers_aggregate.h"

#include "codegen_types.h"

#include "Compiler/compiler_context.h"

#include "Syntax/layout.h"

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

const CCTagFieldLayout* cg_init_lookup_field_layout(CodegenContext* ctx,
                                                           LLVMTypeRef aggregateType,
                                                           const ParsedType* aggregateParsed,
                                                           const char* fieldName) {
    if (!ctx || !fieldName) return NULL;
    const ParsedType* resolved = aggregateParsed;
    size_t guard = 0;
    while (resolved &&
           resolved->tag != TAG_STRUCT &&
           resolved->tag != TAG_UNION &&
           resolved->kind == TYPE_NAMED &&
           resolved->userTypeName &&
           guard++ < 8) {
        const ParsedType* next = NULL;
        if (ctx->typeCache) {
            CGNamedLLVMType* info = cg_type_cache_get_typedef_info(ctx->typeCache, resolved->userTypeName);
            if (info && info->parsedType.kind != TYPE_INVALID) {
                next = &info->parsedType;
            }
        }
        if (!next) {
            const SemanticModel* model = cg_context_get_semantic_model(ctx);
            if (model) {
                const Symbol* sym = semanticModelLookupGlobal(model, resolved->userTypeName);
                if (sym && sym->kind == SYMBOL_TYPEDEF) {
                    next = &sym->type;
                }
            }
        }
        if (!next || next == resolved) break;
        resolved = next;
    }
    const char* tagName = NULL;
    CCTagKind kind = CC_TAG_STRUCT;
    const char* llvmStructName = NULL;
    if (aggregateType && LLVMGetTypeKind(aggregateType) == LLVMStructTypeKind) {
        llvmStructName = LLVMGetStructName(aggregateType);
    }

    bool isStructLike =
        resolved && (resolved->tag == TAG_STRUCT || resolved->kind == TYPE_STRUCT);
    bool isUnionLike =
        resolved && (resolved->tag == TAG_UNION || resolved->kind == TYPE_UNION);
    if (resolved &&
        (isStructLike || isUnionLike) &&
        resolved->userTypeName) {
        tagName = resolved->userTypeName;
        kind = isUnionLike ? CC_TAG_UNION : CC_TAG_STRUCT;
    } else if (ctx->typeCache && aggregateType) {
        CGStructLLVMInfo* info = cg_type_cache_find_struct_by_llvm(ctx->typeCache, aggregateType);
        if (info && info->name) {
            tagName = info->name;
            kind = info->isUnion ? CC_TAG_UNION : CC_TAG_STRUCT;
        }
    }
    if (!tagName && ctx->typeCache && llvmStructName && llvmStructName[0]) {
        CGStructLLVMInfo* info = cg_type_cache_get_struct_info(ctx->typeCache, llvmStructName);
        if (info && info->name) {
            tagName = info->name;
            kind = info->isUnion ? CC_TAG_UNION : CC_TAG_STRUCT;
        }
    }
    if (!tagName && llvmStructName && llvmStructName[0]) {
        tagName = llvmStructName;
        kind = CC_TAG_STRUCT;
    }
    if (!tagName) return NULL;

    CompilerContext* cctx = ctx->semanticModel ? semanticModelGetContext(ctx->semanticModel) : NULL;
    if (!cctx) return NULL;
    Scope* globalScope = semanticModelGetGlobalScope(ctx->semanticModel);
    const CCTagFieldLayout* layouts = NULL;
    size_t count = 0;
    if (!cc_get_tag_field_layouts(cctx, kind, tagName, &layouts, &count) || !layouts) {
        (void)layout_struct_union(cctx, globalScope, kind, tagName, NULL, NULL);
        cc_get_tag_field_layouts(cctx, kind, tagName, &layouts, &count);
    }
    if (!layouts) return NULL;
    for (size_t i = 0; i < count; ++i) {
        if (layouts[i].name && strcmp(layouts[i].name, fieldName) == 0) {
            return &layouts[i];
        }
    }
    return NULL;
}

bool cg_init_field_by_index(CodegenContext* ctx,
                                   const ParsedType* aggregateParsed,
                                   unsigned targetIndex,
                                   const char** outFieldName,
                                   const ParsedType** outParsed) {
    if (outFieldName) *outFieldName = NULL;
    if (outParsed) *outParsed = NULL;
    if (!ctx || !aggregateParsed) return false;

    const ParsedType* resolved = aggregateParsed;
    size_t guard = 0;
    while (resolved &&
           resolved->tag != TAG_STRUCT &&
           resolved->tag != TAG_UNION &&
           resolved->kind == TYPE_NAMED &&
           resolved->userTypeName &&
           guard++ < 8) {
        const ParsedType* next = NULL;
        if (ctx->typeCache) {
            CGNamedLLVMType* info = cg_type_cache_get_typedef_info(ctx->typeCache, resolved->userTypeName);
            if (info && info->parsedType.kind != TYPE_INVALID) {
                next = &info->parsedType;
            }
        }
        if (!next) {
            const SemanticModel* model = cg_context_get_semantic_model(ctx);
            if (model) {
                const Symbol* sym = semanticModelLookupGlobal(model, resolved->userTypeName);
                if (sym && sym->kind == SYMBOL_TYPEDEF) {
                    next = &sym->type;
                }
            }
        }
        if (!next || next == resolved) break;
        resolved = next;
    }
    bool isStructLike =
        resolved && (resolved->tag == TAG_STRUCT || resolved->kind == TYPE_STRUCT);
    bool isUnionLike =
        resolved && (resolved->tag == TAG_UNION || resolved->kind == TYPE_UNION);
    if (!resolved || (!isStructLike && !isUnionLike) || !resolved->userTypeName) {
        return false;
    }

    CompilerContext* cctx = ctx->semanticModel ? semanticModelGetContext(ctx->semanticModel) : NULL;
    if (!cctx) return false;
    CCTagKind kind = isUnionLike ? CC_TAG_UNION : CC_TAG_STRUCT;
    ASTNode* def = cc_tag_definition(cctx, kind, resolved->userTypeName);
    if (!def || (def->type != AST_STRUCT_DEFINITION && def->type != AST_UNION_DEFINITION)) {
        return false;
    }

    unsigned running = 0;
    bool isUnion = (def->type == AST_UNION_DEFINITION);
    for (size_t f = 0; f < def->structDef.fieldCount; ++f) {
        ASTNode* fieldDecl = def->structDef.fields[f];
        if (!fieldDecl || fieldDecl->type != AST_VARIABLE_DECLARATION) continue;
        for (size_t v = 0; v < fieldDecl->varDecl.varCount; ++v) {
            unsigned idx = isUnion ? 0u : running;
            if (idx == targetIndex) {
                ASTNode* nameNode = fieldDecl->varDecl.varNames[v];
                const char* name = (nameNode && nameNode->type == AST_IDENTIFIER) ? nameNode->valueNode.value : NULL;
                if (outFieldName) *outFieldName = name;
                if (outParsed) {
                    const ParsedType* parsed = astVarDeclTypeAt(fieldDecl, v);
                    *outParsed = parsed ? parsed : &fieldDecl->varDecl.declaredType;
                }
                return name != NULL;
            }
            if (!isUnion) running++;
        }
    }
    return false;
}

const StructInfo* cg_init_lookup_legacy_struct_info(CodegenContext* ctx,
                                                           const char* structName,
                                                           LLVMTypeRef aggregateType) {
    if (!ctx) {
        return NULL;
    }
    for (size_t i = 0; i < ctx->structInfoCount; ++i) {
        if (aggregateType &&
            ctx->structInfos[i].llvmType &&
            ctx->structInfos[i].llvmType == aggregateType) {
            return &ctx->structInfos[i];
        }
        if (structName &&
            ctx->structInfos[i].name &&
            strcmp(ctx->structInfos[i].name, structName) == 0) {
            return &ctx->structInfos[i];
        }
    }
    return NULL;
}

CGStructLLVMInfo* cg_init_find_struct_info_for_aggregate(CodegenContext* ctx,
                                                                LLVMTypeRef aggregateType,
                                                                const ParsedType* parsedHint) {
    if (!ctx || !ctx->typeCache) {
        return NULL;
    }

    const ParsedType* resolvedHint = parsedHint;
    size_t aliasGuard = 0;
    while (resolvedHint &&
           resolvedHint->kind == TYPE_NAMED &&
           resolvedHint->userTypeName &&
           aliasGuard++ < 16) {
        const ParsedType* next = NULL;
        CGNamedLLVMType* info =
            cg_type_cache_get_typedef_info(ctx->typeCache, resolvedHint->userTypeName);
        if (info && info->parsedType.kind != TYPE_INVALID) {
            next = &info->parsedType;
        }
        if (!next && ctx->semanticModel) {
            const Symbol* sym = semanticModelLookupGlobal(ctx->semanticModel,
                                                          resolvedHint->userTypeName);
            if (sym && sym->kind == SYMBOL_TYPEDEF) {
                next = &sym->type;
            }
        }
        if (!next || next == resolvedHint) {
            break;
        }
        resolvedHint = next;
    }

    if (resolvedHint && resolvedHint->inlineStructOrUnionDef) {
        CGStructLLVMInfo* info =
            cg_type_cache_get_struct_by_definition(ctx->typeCache,
                                                   resolvedHint->inlineStructOrUnionDef);
        if (info) {
            return info;
        }
    }

    if (aggregateType) {
        CGStructLLVMInfo* info = cg_type_cache_find_struct_by_llvm(ctx->typeCache, aggregateType);
        if (info) {
            return info;
        }
        if (LLVMGetTypeKind(aggregateType) == LLVMStructTypeKind) {
            const char* llvmStructName = LLVMGetStructName(aggregateType);
            if (llvmStructName && llvmStructName[0] != '\0') {
                info = cg_type_cache_get_struct_info(ctx->typeCache, llvmStructName);
                if (info) {
                    return info;
                }
            }
        }
    }

    if (resolvedHint && resolvedHint->userTypeName) {
        return cg_type_cache_get_struct_info(ctx->typeCache, resolvedHint->userTypeName);
    }
    return NULL;
}

static LLVMValueRef cg_init_bitfield_mask(LLVMTypeRef storageTy, unsigned width) {
    unsigned storageBits = LLVMGetIntTypeWidth(storageTy);
    if (width == 0) {
        return LLVMConstInt(storageTy, 0, 0);
    }
    if (width >= storageBits) {
        return LLVMConstAllOnes(storageTy);
    }
    uint64_t mask = (width >= 64) ? ~0ULL : ((1ULL << width) - 1ULL);
    return LLVMConstInt(storageTy, mask, 0);
}

bool cg_init_store_bitfield(CodegenContext* ctx,
                                   LLVMValueRef basePtr,
                                   const CCTagFieldLayout* lay,
                                   LLVMValueRef value,
                                   const ParsedType* fromParsed,
                                   const ParsedType* toParsed) {
    if (!ctx || !basePtr || !lay || !value) return false;
    unsigned storageBits = (unsigned)(lay->storageUnitBytes ? lay->storageUnitBytes * 8 : 32);
    LLVMTypeRef storageTy = LLVMIntTypeInContext(ctx->llvmContext, storageBits);
    LLVMTypeRef i8Ty = LLVMInt8TypeInContext(ctx->llvmContext);
    LLVMValueRef baseI8 = LLVMBuildBitCast(ctx->builder, basePtr, LLVMPointerType(i8Ty, 0), "init.bf.base");
    LLVMValueRef offsetVal = LLVMConstInt(LLVMInt64TypeInContext(ctx->llvmContext), lay->byteOffset, 0);
    LLVMValueRef ptrI8 = LLVMBuildGEP2(ctx->builder, i8Ty, baseI8, &offsetVal, 1, "init.bf.gep");
    LLVMValueRef storagePtr = LLVMBuildBitCast(ctx->builder, ptrI8, LLVMPointerType(storageTy, 0), "init.bf.ptr");

    LLVMValueRef casted = cg_cast_value(ctx, value, storageTy, fromParsed, toParsed, "init.bf.cast");
    unsigned bitOffset = (unsigned)lay->bitOffset;
    LLVMValueRef mask = cg_init_bitfield_mask(storageTy, (unsigned)lay->widthBits);
    if (lay->widthBits < LLVMGetIntTypeWidth(storageTy)) {
        casted = LLVMBuildAnd(ctx->builder, casted, mask, "init.bf.truncmask");
    }
    LLVMValueRef shifted = casted;
    if (bitOffset > 0) {
        LLVMValueRef sh = LLVMConstInt(storageTy, bitOffset, 0);
        shifted = LLVMBuildShl(ctx->builder, casted, sh, "init.bf.shl");
    }
    LLVMValueRef shiftedMask = mask;
    if (bitOffset > 0) {
        LLVMValueRef sh = LLVMConstInt(storageTy, bitOffset, 0);
        shiftedMask = LLVMBuildShl(ctx->builder, mask, sh, "init.bf.mask.shl");
    }
    LLVMValueRef oldVal = LLVMBuildLoad2(ctx->builder, storageTy, storagePtr, "init.bf.old");
    LLVMValueRef notMask = LLVMBuildNot(ctx->builder, shiftedMask, "init.bf.notmask");
    LLVMValueRef cleared = LLVMBuildAnd(ctx->builder, oldVal, notMask, "init.bf.cleared");
    LLVMValueRef combined = LLVMBuildOr(ctx->builder, cleared, shifted, "init.bf.combined");
    LLVMBuildStore(ctx->builder, combined, storagePtr);
    return true;
}

unsigned long long cg_eval_initializer_index(ASTNode* expr, bool* outSuccess) {
    if (outSuccess) *outSuccess = false;
    if (!expr) return 0;
    unsigned long long result = 0;
    switch (expr->type) {
        case AST_NUMBER_LITERAL:
            result = strtoull(expr->valueNode.value, NULL, 0);
            if (outSuccess) *outSuccess = true;
            return result;
        case AST_CHAR_LITERAL:
            if (expr->valueNode.value && expr->valueNode.value[0] != '\0') {
                result = (unsigned char)expr->valueNode.value[0];
                if (outSuccess) *outSuccess = true;
                return result;
            }
            break;
        default:
            break;
    }
    fprintf(stderr, "Error: Initializer designator index must be an integer constant\n");
    return 0;
}

bool cg_entries_flat_scalars(DesignatedInit** entries, size_t entryCount) {
    if (!entries || entryCount == 0) return false;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;
        if (entry->fieldName || entry->indexExpr) {
            return false;
        }
        if (entry->expression->type == AST_COMPOUND_LITERAL) {
            return false;
        }
    }
    return true;
}

bool cg_expr_initializes_whole_aggregate(CodegenContext* ctx,
                                                ASTNode* expr,
                                                LLVMTypeRef destType) {
    if (!ctx || !expr || !destType) return false;
    LLVMTypeKind destKind = LLVMGetTypeKind(destType);
    if (destKind != LLVMStructTypeKind && destKind != LLVMArrayTypeKind) {
        return false;
    }
    if (expr->type == AST_COMPOUND_LITERAL) {
        return true;
    }

    const ParsedType* exprParsed = cg_resolve_expression_type(ctx, expr);
    if (!exprParsed || exprParsed->kind == TYPE_INVALID) {
        return false;
    }

    LLVMTypeRef exprType = cg_type_from_parsed(ctx, exprParsed);
    return exprType && exprType == destType;
}

bool cg_entries_have_designators(DesignatedInit** entries, size_t entryCount) {
    if (!entries || entryCount == 0) return false;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry) continue;
        if (entry->fieldName || entry->indexExpr) {
            return true;
        }
    }
    return false;
}
