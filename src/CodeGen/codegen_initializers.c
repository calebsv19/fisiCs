#include "codegen_private.h"

#include "codegen_types.h"
#include "Compiler/compiler_context.h"
#include "Syntax/layout.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const CCTagFieldLayout* cg_init_lookup_field_layout(CodegenContext* ctx,
                                                           const ParsedType* aggregateParsed,
                                                           const char* fieldName) {
    if (!ctx || !aggregateParsed || !fieldName) return NULL;
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
    if (!resolved || (resolved->tag != TAG_STRUCT && resolved->tag != TAG_UNION) || !resolved->userTypeName) {
        return NULL;
    }

    CompilerContext* cctx = ctx->semanticModel ? semanticModelGetContext(ctx->semanticModel) : NULL;
    if (!cctx) return NULL;
    Scope* globalScope = semanticModelGetGlobalScope(ctx->semanticModel);
    CCTagKind kind = (resolved->tag == TAG_STRUCT) ? CC_TAG_STRUCT : CC_TAG_UNION;
    const CCTagFieldLayout* layouts = NULL;
    size_t count = 0;
    if (!cc_get_tag_field_layouts(cctx, kind, resolved->userTypeName, &layouts, &count) || !layouts) {
        (void)layout_struct_union(cctx, globalScope, kind, resolved->userTypeName, NULL, NULL);
        cc_get_tag_field_layouts(cctx, kind, resolved->userTypeName, &layouts, &count);
    }
    if (!layouts) return NULL;
    for (size_t i = 0; i < count; ++i) {
        if (layouts[i].name && strcmp(layouts[i].name, fieldName) == 0) {
            return &layouts[i];
        }
    }
    return NULL;
}

static bool cg_init_field_by_index(CodegenContext* ctx,
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
    if (!resolved || (resolved->tag != TAG_STRUCT && resolved->tag != TAG_UNION) || !resolved->userTypeName) {
        return false;
    }

    CompilerContext* cctx = ctx->semanticModel ? semanticModelGetContext(ctx->semanticModel) : NULL;
    if (!cctx) return false;
    CCTagKind kind = (resolved->tag == TAG_STRUCT) ? CC_TAG_STRUCT : CC_TAG_UNION;
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

static bool cg_init_store_bitfield(CodegenContext* ctx,
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

static unsigned long long cg_eval_initializer_index(ASTNode* expr, bool* outSuccess) {
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

bool cg_store_initializer_expression(CodegenContext* ctx,
                                     LLVMValueRef destPtr,
                                     LLVMTypeRef destType,
                                     const ParsedType* destParsed,
                                     ASTNode* expr) {
    if (!ctx || !destPtr || !expr) return false;
    LLVMValueRef value = codegenNode(ctx, expr);
    if (!value) {
        fprintf(stderr, "Error: Failed to evaluate initializer expression\n");
        return false;
    }

    LLVMTypeRef storeType = destType;
    if (!storeType || LLVMGetTypeKind(storeType) == LLVMVoidTypeKind) {
        storeType = LLVMTypeOf(value);
    }
    if (!storeType || LLVMGetTypeKind(storeType) == LLVMVoidTypeKind) {
        fprintf(stderr, "Error: Invalid initializer destination type\n");
        return false;
    }

    // Zero initializer to memset if possible
    if (expr->type == AST_NUMBER_LITERAL && expr->valueNode.value && strcmp(expr->valueNode.value, "0") == 0) {
        uint64_t bytes = 0;
        uint32_t align = 0;
        cg_size_align_for_type(ctx, destParsed, storeType, &bytes, &align);
        LLVMTypeRef i8Ptr = LLVMPointerType(LLVMInt8TypeInContext(ctx->llvmContext), 0);
        LLVMValueRef dstCast = LLVMBuildBitCast(ctx->builder, destPtr, i8Ptr, "init.zero.dst");
        LLVMValueRef sizeVal = LLVMConstInt(LLVMInt64TypeInContext(ctx->llvmContext), bytes, 0);
        LLVMBuildMemSet(ctx->builder,
                        dstCast,
                        LLVMConstInt(LLVMInt8TypeInContext(ctx->llvmContext), 0, 0),
                        sizeVal,
                        align ? align : 1);
        return true;
    }

    LLVMValueRef casted = cg_cast_value(ctx, value, storeType, cg_resolve_expression_type(ctx, expr), destParsed, "init.cast");
    LLVMBuildStore(ctx->builder, casted, destPtr);
    return true;
}

static bool cg_store_struct_entries(CodegenContext* ctx,
                                    LLVMValueRef destPtr,
                                    LLVMTypeRef destType,
                                    const ParsedType* destParsed,
                                    DesignatedInit** entries,
                                    size_t entryCount);
static bool cg_store_array_entries(CodegenContext* ctx,
                                   LLVMValueRef destPtr,
                                   LLVMTypeRef destType,
                                   const ParsedType* destParsed,
                                   DesignatedInit** entries,
                                   size_t entryCount);

bool cg_store_designated_entries(CodegenContext* ctx,
                                 LLVMValueRef destPtr,
                                 LLVMTypeRef destType,
                                 const ParsedType* destParsed,
                                 DesignatedInit** entries,
                                 size_t entryCount) {
    if (!ctx || !destPtr || !destType || LLVMGetTypeKind(destType) == LLVMVoidTypeKind) {
        return false;
    }

    LLVMTypeKind kind = LLVMGetTypeKind(destType);
    if (kind == LLVMStructTypeKind || kind == LLVMArrayTypeKind) {
        /*
         * C aggregate initialization is fail-closed: any field/element not explicitly
         * initialized must become zero-initialized. Seed with a full zeroinitializer
         * before applying explicit designators/entries.
         */
        LLVMBuildStore(ctx->builder, LLVMConstNull(destType), destPtr);

        if (kind == LLVMStructTypeKind) {
            return cg_store_struct_entries(ctx, destPtr, destType, destParsed, entries, entryCount);
        }
        return cg_store_array_entries(ctx, destPtr, destType, destParsed, entries, entryCount);
    }

    if (entryCount != 1 || !entries[0] || !entries[0]->expression) {
        fprintf(stderr, "Error: scalar initializer requires single expression\n");
        return false;
    }
    return cg_store_initializer_expression(ctx, destPtr, destType, destParsed, entries[0]->expression);
}

bool cg_store_compound_literal_into_ptr(CodegenContext* ctx,
                                        LLVMValueRef destPtr,
                                        LLVMTypeRef destType,
                                        const ParsedType* destParsed,
                                        ASTNode* literalNode) {
    if (!ctx || !destPtr || !literalNode || literalNode->type != AST_COMPOUND_LITERAL) {
        return false;
    }

    const ParsedType* literalParsed = &literalNode->compoundLiteral.literalType;
    if (literalParsed && literalParsed->kind != TYPE_INVALID) {
        destParsed = literalParsed;
        LLVMTypeRef literalLLVM = cg_type_from_parsed(ctx, literalParsed);
        if (literalLLVM && LLVMGetTypeKind(literalLLVM) != LLVMVoidTypeKind) {
            destType = literalLLVM;
        }
    }

    if (!destType || LLVMGetTypeKind(destType) == LLVMVoidTypeKind) {
        fprintf(stderr, "Error: Unable to resolve compound literal type\n");
        return false;
    }

    return cg_store_designated_entries(ctx,
                                       destPtr,
                                       destType,
                                       destParsed,
                                       literalNode->compoundLiteral.entries,
                                       literalNode->compoundLiteral.entryCount);
}

static bool cg_store_struct_entries(CodegenContext* ctx,
                                    LLVMValueRef destPtr,
                                    LLVMTypeRef destType,
                                    const ParsedType* destParsed,
                                    DesignatedInit** entries,
                                    size_t entryCount) {
    if (!ctx || !destPtr || LLVMGetTypeKind(destType) != LLVMStructTypeKind) {
        return false;
    }

    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    const char* structName = destParsed ? destParsed->userTypeName : NULL;
    CGStructLLVMInfo* structInfo = NULL;
    if (cache && structName) {
        structInfo = cg_type_cache_get_struct_info(cache, structName);
    }

    unsigned implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned targetIndex = implicitIndex;
        const ParsedType* fieldParsed = NULL;
        const char* targetFieldName = entry->fieldName;

        bool matchedField = false;
        if (entry->fieldName && structInfo) {
            for (size_t f = 0; f < structInfo->fieldCount; ++f) {
                if (structInfo->fields[f].name &&
                    strcmp(structInfo->fields[f].name, entry->fieldName) == 0) {
                    targetIndex = structInfo->fields[f].index;
                    fieldParsed = &structInfo->fields[f].parsedType;
                    matchedField = true;
                    break;
                }
            }
        }
        if (!entry->fieldName && structInfo && targetIndex < structInfo->fieldCount) {
            targetFieldName = structInfo->fields[targetIndex].name;
            fieldParsed = &structInfo->fields[targetIndex].parsedType;
        }
        if (!targetFieldName) {
            (void)cg_init_field_by_index(ctx, destParsed, targetIndex, &targetFieldName, &fieldParsed);
        }
        (void)matchedField;
        implicitIndex = targetIndex + 1;

        if (targetIndex >= LLVMCountStructElementTypes(destType) && !targetFieldName) {
            continue;
        }

        const CCTagFieldLayout* lay =
            cg_init_lookup_field_layout(ctx, destParsed, targetFieldName);
        ASTNode* valueExpr = entry->expression;
        const ParsedType* valueParsed = cg_resolve_expression_type(ctx, valueExpr);

        if (lay && lay->isBitfield && lay->widthBits > 0) {
            LLVMValueRef value = codegenNode(ctx, valueExpr);
            if (!value) {
                return false;
            }
            if (!cg_init_store_bitfield(ctx, destPtr, lay, value, valueParsed, fieldParsed)) {
                return false;
            }
            continue;
        }

        LLVMTypeRef fieldType = NULL;
        LLVMValueRef fieldPtr = NULL;
        if (targetFieldName) {
            fieldPtr = buildStructFieldPointer(ctx,
                                               destPtr,
                                               destType,
                                               structName,
                                               targetFieldName,
                                               destParsed,
                                               &fieldType,
                                               &fieldParsed);
        }
        if (!fieldPtr) {
            if (targetIndex >= LLVMCountStructElementTypes(destType)) {
                continue;
            }
            fieldPtr = LLVMBuildStructGEP2(ctx->builder, destType, destPtr, targetIndex, "init.field");
            if (!fieldPtr) {
                fprintf(stderr, "Error: Unable to access struct field for initializer\n");
                return false;
            }
            fieldType = LLVMStructGetTypeAtIndex(destType, targetIndex);
        }

        if (valueExpr->type == AST_COMPOUND_LITERAL) {
            if (!cg_store_compound_literal_into_ptr(ctx, fieldPtr, fieldType, fieldParsed, valueExpr)) {
                return false;
            }
        } else {
            if (!cg_store_initializer_expression(ctx, fieldPtr, fieldType, fieldParsed, valueExpr)) {
                return false;
            }
        }
    }
    return true;
}

static bool cg_store_array_entries(CodegenContext* ctx,
                                   LLVMValueRef destPtr,
                                   LLVMTypeRef destType,
                                   const ParsedType* destParsed,
                                   DesignatedInit** entries,
                                   size_t entryCount) {
    if (!ctx || !destPtr || LLVMGetTypeKind(destType) != LLVMArrayTypeKind) {
        return false;
    }
    (void)destParsed;

    LLVMTypeRef elementType = LLVMGetElementType(destType);
    unsigned long long implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned long long targetIndex = implicitIndex;
        if (entry->indexExpr) {
            bool ok = false;
            targetIndex = cg_eval_initializer_index(entry->indexExpr, &ok);
            if (!ok) return false;
        }
        implicitIndex = targetIndex + 1;

        LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
        LLVMValueRef idxVals[2] = {
            zero,
            LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), (unsigned)targetIndex, 0)
        };
        LLVMValueRef elementPtr = LLVMBuildGEP2(ctx->builder, destType, destPtr, idxVals, 2, "init.elem");
        const ParsedType* elementParsed = NULL;
        if (entry->expression->type == AST_COMPOUND_LITERAL) {
            if (!cg_store_compound_literal_into_ptr(ctx, elementPtr, elementType, elementParsed, entry->expression)) {
                return false;
            }
        } else {
            if (!cg_store_initializer_expression(ctx, elementPtr, elementType, elementParsed, entry->expression)) {
                return false;
            }
        }
    }
    return true;
}
