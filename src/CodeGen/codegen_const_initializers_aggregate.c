// SPDX-License-Identifier: Apache-2.0

#include "codegen_const_initializers_internal.h"

#include "codegen_types.h"

#include <stdlib.h>
#include <string.h>

static LLVMValueRef cg_merge_const_initializer(CodegenContext* ctx,
                                               LLVMValueRef baseConst,
                                               ASTNode* expr,
                                               LLVMTypeRef targetType,
                                               const ParsedType* parsedType);
static bool cg_find_field_in_definition(const ASTNode* def,
                                        const char* fieldName,
                                        unsigned* outIndex,
                                        const ParsedType** outParsed);

static LLVMValueRef cg_zero_const(LLVMTypeRef type) {
    return type ? LLVMConstNull(type) : NULL;
}

static CGStructLLVMInfo* cg_find_struct_info(CodegenContext* ctx,
                                             LLVMTypeRef llvmType,
                                             const ParsedType* parsedType) {
    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    if (!cache) return NULL;
    if (parsedType && parsedType->userTypeName) {
        CGStructLLVMInfo* info = cg_type_cache_get_struct_info(cache, parsedType->userTypeName);
        if (info) return info;
    }
    if (llvmType) {
        return cg_type_cache_find_struct_by_llvm(cache, llvmType);
    }
    return NULL;
}

static bool cg_entries_flat_scalars(DesignatedInit** entries, size_t entryCount) {
    if (!entries || entryCount == 0) return false;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;
        if (entry->indexExpr || entry->fieldName) {
            return false;
        }
        if (entry->expression->type == AST_COMPOUND_LITERAL) {
            return false;
        }
    }
    return true;
}

static bool cg_entries_have_designators(DesignatedInit** entries, size_t entryCount) {
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

static LLVMValueRef cg_const_extract_aggregate_value(LLVMValueRef aggregateConst,
                                                     unsigned index,
                                                     LLVMTypeRef fallbackType) {
    if (!fallbackType) return NULL;
    if (!aggregateConst) {
        return cg_zero_const(fallbackType);
    }

    LLVMTypeRef aggregateType = LLVMTypeOf(aggregateConst);
    if (!aggregateType) {
        return cg_zero_const(fallbackType);
    }

    LLVMTypeKind kind = LLVMGetTypeKind(aggregateType);
    if (kind != LLVMArrayTypeKind && kind != LLVMStructTypeKind) {
        return cg_zero_const(fallbackType);
    }

    LLVMValueRef extracted = LLVMGetAggregateElement(aggregateConst, index);
    if (!extracted) {
        return cg_zero_const(fallbackType);
    }
    return extracted;
}

static LLVMValueRef cg_merge_const_array(CodegenContext* ctx,
                                         LLVMValueRef baseConst,
                                         LLVMTypeRef arrayType,
                                         const ParsedType* parsedType,
                                         DesignatedInit** entries,
                                         size_t entryCount) {
    if (!ctx || !arrayType || LLVMGetTypeKind(arrayType) != LLVMArrayTypeKind) return NULL;

    unsigned length = LLVMGetArrayLength(arrayType);
    LLVMTypeRef elemType = LLVMGetElementType(arrayType);
    if (!elemType) return NULL;

    LLVMValueRef* values = (LLVMValueRef*)calloc(length, sizeof(LLVMValueRef));
    if (!values) return NULL;
    for (unsigned i = 0; i < length; ++i) {
        values[i] = cg_const_extract_aggregate_value(baseConst, i, elemType);
    }

    ParsedType elementParsed = {0};
    bool hasElementParsed = false;
    if (parsedType && parsedTypeIsDirectArray(parsedType)) {
        elementParsed = parsedTypeArrayElementType(parsedType);
        hasElementParsed = true;
    }

    unsigned long long implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned long long targetIndex = implicitIndex;
        if (entry->indexExpr) {
            bool ok = false;
            targetIndex = cg_eval_initializer_index_const(ctx, entry->indexExpr, &ok);
            if (!ok) {
                free(values);
                if (hasElementParsed) parsedTypeFree(&elementParsed);
                return NULL;
            }
        }
        implicitIndex = targetIndex + 1;
        if (targetIndex >= length) {
            continue;
        }

        LLVMValueRef baseElementConst = values[targetIndex];
        if (entry->resetSubobjectBeforeStore &&
            entry->expression->type == AST_COMPOUND_LITERAL) {
            baseElementConst = cg_zero_const(elemType);
        }
        LLVMValueRef elementConst = cg_merge_const_initializer(ctx,
                                                               baseElementConst,
                                                               entry->expression,
                                                               elemType,
                                                               hasElementParsed ? &elementParsed : NULL);
        if (!elementConst) {
            free(values);
            if (hasElementParsed) parsedTypeFree(&elementParsed);
            return NULL;
        }
        values[targetIndex] = elementConst;
    }

    LLVMValueRef result = LLVMConstArray(elemType, values, length);
    free(values);
    if (hasElementParsed) parsedTypeFree(&elementParsed);
    return result;
}

static LLVMValueRef cg_merge_const_struct(CodegenContext* ctx,
                                          LLVMValueRef baseConst,
                                          LLVMTypeRef structType,
                                          const ParsedType* parsedType,
                                          DesignatedInit** entries,
                                          size_t entryCount) {
    if (!ctx || !structType || LLVMGetTypeKind(structType) != LLVMStructTypeKind) return NULL;

    const ParsedType* resolvedType = parsedType ? cg_resolve_typedef_parsed(ctx, parsedType) : parsedType;
    const ParsedType* lookupType = resolvedType ? resolvedType : parsedType;
    CGStructLLVMInfo* info = cg_find_struct_info(ctx, structType, lookupType);

    unsigned fieldCount = LLVMCountStructElementTypes(structType);
    if (fieldCount == 0 && parsedType) {
        LLVMTypeRef resolved = cg_type_from_parsed(ctx, parsedType);
        if (resolved) {
            structType = resolved;
            fieldCount = LLVMCountStructElementTypes(structType);
        }
    }
    if (fieldCount == 0 && info && info->definition) {
        (void)codegenStructDefinition(ctx, (ASTNode*)info->definition);
        fieldCount = LLVMCountStructElementTypes(structType);
    }
    if (fieldCount == 0 && info && info->fieldCount > 0) {
        LLVMTypeRef* fieldTypes = (LLVMTypeRef*)calloc(info->fieldCount, sizeof(LLVMTypeRef));
        if (fieldTypes) {
            for (size_t i = 0; i < info->fieldCount; ++i) {
                fieldTypes[i] = cg_type_from_parsed(ctx, &info->fields[i].parsedType);
            }
            LLVMStructSetBody(structType, fieldTypes, (unsigned)info->fieldCount, 0);
            free(fieldTypes);
            fieldCount = LLVMCountStructElementTypes(structType);
        }
    }
    if (!info && lookupType && lookupType->userTypeName && ctx->semanticModel) {
        CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
        if (cctx) {
            CCTagKind kind = (lookupType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
            ASTNode* def = cc_tag_definition(cctx, kind, lookupType->userTypeName);
            if (def) {
                (void)codegenStructDefinition(ctx, def);
                info = cg_find_struct_info(ctx, structType, lookupType);
            }
        }
    }
    if (fieldCount == 0 && parsedType && parsedType->userTypeName && ctx->semanticModel) {
        CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
        if (cctx) {
            CCTagKind kind = (parsedType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
            ASTNode* def = cc_tag_definition(cctx, kind, parsedType->userTypeName);
            if (def) {
                (void)codegenStructDefinition(ctx, def);
                structType = cg_type_from_parsed(ctx, parsedType);
                fieldCount = LLVMCountStructElementTypes(structType);
            }
        }
    }
    if (fieldCount == 0) {
        return NULL;
    }

    LLVMValueRef* fields = (LLVMValueRef*)calloc(fieldCount, sizeof(LLVMValueRef));
    if (!fields) return NULL;
    for (unsigned i = 0; i < fieldCount; ++i) {
        LLVMTypeRef fieldType = LLVMStructGetTypeAtIndex(structType, i);
        fields[i] = cg_const_extract_aggregate_value(baseConst, i, fieldType);
    }

    unsigned implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned targetIndex = implicitIndex;
        const ParsedType* fieldParsed = NULL;
        const char* targetFieldName = entry->fieldName;
        bool matchedField = false;
        if (entry->fieldName && info && info->fieldCount > 0) {
            for (size_t f = 0; f < info->fieldCount; ++f) {
                if (info->fields[f].name && strcmp(info->fields[f].name, entry->fieldName) == 0) {
                    targetIndex = info->fields[f].index;
                    fieldParsed = &info->fields[f].parsedType;
                    targetFieldName = info->fields[f].name;
                    matchedField = true;
                    break;
                }
            }
        }
        if (entry->fieldName && !matchedField) {
            const ASTNode* def = info ? (const ASTNode*)info->definition : NULL;
            if (!def && lookupType && lookupType->userTypeName && ctx->semanticModel) {
                CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
                if (cctx) {
                    CCTagKind kind = (lookupType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
                    def = cc_tag_definition(cctx, kind, lookupType->userTypeName);
                }
            }
            if (def) {
                matchedField = cg_find_field_in_definition(def, entry->fieldName, &targetIndex, &fieldParsed);
            }
        }
        if (!targetFieldName && info && targetIndex < info->fieldCount) {
            targetFieldName = info->fields[targetIndex].name;
        }
        implicitIndex = targetIndex + 1;
        if (targetIndex >= fieldCount) {
            continue;
        }

        LLVMTypeRef fieldType = LLVMStructGetTypeAtIndex(structType, targetIndex);
        LLVMValueRef fieldConst = NULL;
        size_t mergedLast = i;
        DesignatedInit** mergedEntries = NULL;
        size_t mergedCount = 0;
        if (!entry->resetSubobjectBeforeStore &&
            targetFieldName &&
            entry->expression->type == AST_COMPOUND_LITERAL &&
            cg_entries_have_designators(entry->expression->compoundLiteral.entries,
                                        entry->expression->compoundLiteral.entryCount) &&
            (LLVMGetTypeKind(fieldType) == LLVMArrayTypeKind ||
             LLVMGetTypeKind(fieldType) == LLVMStructTypeKind)) {
            mergedCount = entry->expression->compoundLiteral.entryCount;
            for (size_t j = i + 1; j < entryCount; ++j) {
                DesignatedInit* next = entries[j];
                if (!next || !next->expression || !next->fieldName) {
                    break;
                }
                if (strcmp(next->fieldName, targetFieldName) != 0 ||
                    next->resetSubobjectBeforeStore ||
                    next->expression->type != AST_COMPOUND_LITERAL ||
                    !cg_entries_have_designators(next->expression->compoundLiteral.entries,
                                                next->expression->compoundLiteral.entryCount)) {
                    break;
                }
                mergedCount += next->expression->compoundLiteral.entryCount;
                mergedLast = j;
            }
            if (mergedLast > i) {
                mergedEntries = (DesignatedInit**)calloc(mergedCount, sizeof(DesignatedInit*));
                if (!mergedEntries) {
                    free(fields);
                    return NULL;
                }
                size_t cursor = 0;
                for (size_t j = i; j <= mergedLast; ++j) {
                    DesignatedInit* next = entries[j];
                    for (size_t k = 0; k < next->expression->compoundLiteral.entryCount; ++k) {
                        mergedEntries[cursor++] = next->expression->compoundLiteral.entries[k];
                    }
                }
            }
        }
        if (mergedEntries) {
            if (LLVMGetTypeKind(fieldType) == LLVMArrayTypeKind) {
                fieldConst = cg_build_const_array(ctx, fieldType, fieldParsed, mergedEntries, mergedCount);
            } else {
                fieldConst = cg_build_const_struct(ctx, fieldType, fieldParsed, mergedEntries, mergedCount);
            }
            free(mergedEntries);
            i = mergedLast;
        } else {
            LLVMValueRef baseFieldConst = fields[targetIndex];
            if (entry->resetSubobjectBeforeStore &&
                entry->expression->type == AST_COMPOUND_LITERAL) {
                baseFieldConst = cg_zero_const(fieldType);
            }
            fieldConst = cg_merge_const_initializer(ctx,
                                                    baseFieldConst,
                                                    entry->expression,
                                                    fieldType,
                                                    fieldParsed);
        }
        if (!fieldConst) {
            free(fields);
            return NULL;
        }
        fields[targetIndex] = fieldConst;
    }

    LLVMValueRef result = LLVMConstNamedStruct(structType, fields, fieldCount);
    free(fields);
    return result;
}

static LLVMValueRef cg_merge_const_initializer(CodegenContext* ctx,
                                               LLVMValueRef baseConst,
                                               ASTNode* expr,
                                               LLVMTypeRef targetType,
                                               const ParsedType* parsedType) {
    if (!ctx || !expr || !targetType) return NULL;
    if (expr->type == AST_COMPOUND_LITERAL &&
        cg_entries_have_designators(expr->compoundLiteral.entries,
                                    expr->compoundLiteral.entryCount)) {
        LLVMTypeKind targetKind = LLVMGetTypeKind(targetType);
        if (targetKind == LLVMArrayTypeKind) {
            return cg_merge_const_array(ctx,
                                        baseConst,
                                        targetType,
                                        parsedType,
                                        expr->compoundLiteral.entries,
                                        expr->compoundLiteral.entryCount);
        }
        if (targetKind == LLVMStructTypeKind) {
            return cg_merge_const_struct(ctx,
                                         baseConst,
                                         targetType,
                                         parsedType,
                                         expr->compoundLiteral.entries,
                                         expr->compoundLiteral.entryCount);
        }
    }
    return cg_build_const_initializer(ctx, expr, targetType, parsedType);
}

static LLVMValueRef cg_build_const_array_flat(CodegenContext* ctx,
                                              LLVMTypeRef arrayType,
                                              const ParsedType* parsedType,
                                              DesignatedInit** entries,
                                              size_t entryCount,
                                              size_t* cursor) {
    if (!ctx || !arrayType || LLVMGetTypeKind(arrayType) != LLVMArrayTypeKind || !cursor) {
        return NULL;
    }

    unsigned length = LLVMGetArrayLength(arrayType);
    LLVMTypeRef elemType = LLVMGetElementType(arrayType);
    if (!elemType) return NULL;

    LLVMValueRef* values = (LLVMValueRef*)calloc(length, sizeof(LLVMValueRef));
    if (!values) return NULL;

    ParsedType elementParsed = {0};
    bool hasElementParsed = false;
    if (parsedType && parsedTypeIsDirectArray(parsedType)) {
        elementParsed = parsedTypeArrayElementType(parsedType);
        hasElementParsed = true;
    }

    for (unsigned i = 0; i < length; ++i) {
        LLVMValueRef elemConst = cg_zero_const(elemType);
        if (*cursor < entryCount) {
            DesignatedInit* entry = entries[*cursor];
            if (entry && entry->expression) {
                if (LLVMGetTypeKind(elemType) == LLVMArrayTypeKind) {
                    if (entry->expression->type == AST_COMPOUND_LITERAL) {
                        elemConst = cg_build_const_initializer(ctx,
                                                               entry->expression,
                                                               elemType,
                                                               hasElementParsed ? &elementParsed : NULL);
                        (*cursor)++;
                    } else {
                        elemConst = cg_build_const_array_flat(ctx,
                                                              elemType,
                                                              hasElementParsed ? &elementParsed : NULL,
                                                              entries,
                                                              entryCount,
                                                              cursor);
                    }
                } else {
                    elemConst = cg_build_const_initializer(ctx,
                                                           entry->expression,
                                                           elemType,
                                                           hasElementParsed ? &elementParsed : NULL);
                    (*cursor)++;
                }
            }
        }
        if (!elemConst) {
            elemConst = cg_zero_const(elemType);
        }
        values[i] = elemConst;
    }

    LLVMValueRef result = LLVMConstArray(elemType, values, length);
    free(values);
    if (hasElementParsed) parsedTypeFree(&elementParsed);
    return result;
}

LLVMValueRef cg_build_const_array(CodegenContext* ctx,
                                  LLVMTypeRef arrayType,
                                  const ParsedType* parsedType,
                                  DesignatedInit** entries,
                                  size_t entryCount) {
    if (!ctx || !arrayType || LLVMGetTypeKind(arrayType) != LLVMArrayTypeKind) return NULL;

    if (entryCount == 0) {
        return cg_zero_const(arrayType);
    }

    if (cg_entries_flat_scalars(entries, entryCount)) {
        size_t cursor = 0;
        return cg_build_const_array_flat(ctx, arrayType, parsedType, entries, entryCount, &cursor);
    }

    unsigned length = LLVMGetArrayLength(arrayType);
    LLVMTypeRef elemType = LLVMGetElementType(arrayType);
    if (!elemType) return NULL;

    LLVMValueRef* values = (LLVMValueRef*)calloc(length, sizeof(LLVMValueRef));
    if (!values) return NULL;

    ParsedType elementParsed = {0};
    bool hasElementParsed = false;
    if (parsedType && parsedTypeIsDirectArray(parsedType)) {
        elementParsed = parsedTypeArrayElementType(parsedType);
        hasElementParsed = true;
    }

    for (unsigned i = 0; i < length; ++i) {
        values[i] = cg_zero_const(elemType);
    }

    unsigned long long implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned long long targetIndex = implicitIndex;
        if (entry->indexExpr) {
            bool ok = false;
            targetIndex = cg_eval_initializer_index_const(ctx, entry->indexExpr, &ok);
            if (!ok) {
                free(values);
                if (hasElementParsed) parsedTypeFree(&elementParsed);
                return NULL;
            }
        }
        implicitIndex = targetIndex + 1;
        if (targetIndex >= length) {
            continue;
        }

        LLVMValueRef baseElementConst = values[targetIndex];
        if (entry->resetSubobjectBeforeStore &&
            entry->expression->type == AST_COMPOUND_LITERAL) {
            baseElementConst = cg_zero_const(elemType);
        }
        LLVMValueRef elementConst = cg_merge_const_initializer(ctx,
                                                               baseElementConst,
                                                               entry->expression,
                                                               elemType,
                                                               hasElementParsed ? &elementParsed : NULL);
        if (!elementConst) {
            free(values);
            if (hasElementParsed) parsedTypeFree(&elementParsed);
            return NULL;
        }
        values[targetIndex] = elementConst;
    }

    LLVMValueRef result = LLVMConstArray(elemType, values, length);
    free(values);
    if (hasElementParsed) parsedTypeFree(&elementParsed);
    return result;
}

static bool cg_find_field_in_definition(const ASTNode* def,
                                        const char* fieldName,
                                        unsigned* outIndex,
                                        const ParsedType** outParsed) {
    if (!def || !fieldName || !outIndex) return false;
    if (def->type != AST_STRUCT_DEFINITION && def->type != AST_UNION_DEFINITION) {
        return false;
    }
    unsigned index = 0;
    for (size_t f = 0; f < def->structDef.fieldCount; ++f) {
        ASTNode* fieldDecl = def->structDef.fields[f];
        if (!fieldDecl || fieldDecl->type != AST_VARIABLE_DECLARATION) continue;
        for (size_t v = 0; v < fieldDecl->varDecl.varCount; ++v) {
            ASTNode* nameNode = fieldDecl->varDecl.varNames[v];
            const char* candidate = (nameNode && nameNode->type == AST_IDENTIFIER)
                ? nameNode->valueNode.value
                : NULL;
            if (candidate && strcmp(candidate, fieldName) == 0) {
                *outIndex = (def->type == AST_UNION_DEFINITION) ? 0u : index;
                if (outParsed) {
                    const ParsedType* parsed = astVarDeclTypeAt((ASTNode*)fieldDecl, v);
                    *outParsed = parsed ? parsed : &fieldDecl->varDecl.declaredType;
                }
                return true;
            }
            if (def->type != AST_UNION_DEFINITION) {
                ++index;
            }
        }
    }
    return false;
}

LLVMValueRef cg_build_const_struct(CodegenContext* ctx,
                                   LLVMTypeRef structType,
                                   const ParsedType* parsedType,
                                   DesignatedInit** entries,
                                   size_t entryCount) {
    if (!ctx || !structType || LLVMGetTypeKind(structType) != LLVMStructTypeKind) return NULL;

    const ParsedType* resolvedType = parsedType ? cg_resolve_typedef_parsed(ctx, parsedType) : parsedType;
    const ParsedType* lookupType = resolvedType ? resolvedType : parsedType;
    bool isUnion = lookupType && lookupType->kind == TYPE_UNION;
    CGStructLLVMInfo* info = cg_find_struct_info(ctx, structType, lookupType);
    if (info && info->isUnion) {
        isUnion = true;
    }

    unsigned fieldCount = LLVMCountStructElementTypes(structType);
    if (fieldCount == 0 && parsedType) {
        LLVMTypeRef resolved = cg_type_from_parsed(ctx, parsedType);
        if (resolved) {
            structType = resolved;
            fieldCount = LLVMCountStructElementTypes(structType);
        }
    }
    if (fieldCount == 0 && info && info->definition) {
        (void)codegenStructDefinition(ctx, (ASTNode*)info->definition);
        fieldCount = LLVMCountStructElementTypes(structType);
    }
    if (fieldCount == 0 && info && info->fieldCount > 0) {
        LLVMTypeRef* fieldTypes = (LLVMTypeRef*)calloc(info->fieldCount, sizeof(LLVMTypeRef));
        if (fieldTypes) {
            for (size_t i = 0; i < info->fieldCount; ++i) {
                fieldTypes[i] = cg_type_from_parsed(ctx, &info->fields[i].parsedType);
            }
            LLVMStructSetBody(structType, fieldTypes, (unsigned)info->fieldCount, 0);
            free(fieldTypes);
            fieldCount = LLVMCountStructElementTypes(structType);
        }
    }
    if (!info && lookupType && lookupType->userTypeName && ctx->semanticModel) {
        CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
        if (cctx) {
            CCTagKind kind = (lookupType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
            ASTNode* def = cc_tag_definition(cctx, kind, lookupType->userTypeName);
            if (def) {
                (void)codegenStructDefinition(ctx, def);
                info = cg_find_struct_info(ctx, structType, lookupType);
            }
        }
    }

    if (fieldCount == 0 && parsedType && parsedType->userTypeName && ctx->semanticModel) {
        CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
        if (cctx) {
            CCTagKind kind = (parsedType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
            ASTNode* def = cc_tag_definition(cctx, kind, parsedType->userTypeName);
            if (def) {
                (void)codegenStructDefinition(ctx, def);
                structType = cg_type_from_parsed(ctx, parsedType);
                fieldCount = LLVMCountStructElementTypes(structType);
            }
        }
    }
    if (fieldCount == 0) {
        return LLVMConstStruct(NULL, 0, 0);
    }

    LLVMValueRef* fields = (LLVMValueRef*)calloc(fieldCount, sizeof(LLVMValueRef));
    if (!fields) return NULL;
    for (unsigned i = 0; i < fieldCount; ++i) {
        fields[i] = cg_zero_const(LLVMStructGetTypeAtIndex(structType, i));
    }

    if (isUnion) {
        if (entryCount > 0 && entries[0] && entries[0]->expression) {
            LLVMTypeRef firstType = LLVMStructGetTypeAtIndex(structType, 0);
            const ParsedType* fieldParsed = NULL;
            LLVMTypeRef fieldType = firstType;
            if (info && info->fieldCount > 0) {
                fieldParsed = &info->fields[0].parsedType;
            }
            if (entries[0]->fieldName && info) {
                for (size_t f = 0; f < info->fieldCount; ++f) {
                    const char* fname = info->fields[f].name;
                    if (fname && strcmp(fname, entries[0]->fieldName) == 0) {
                        fieldParsed = &info->fields[f].parsedType;
                        fieldType = cg_type_from_parsed(ctx, fieldParsed);
                        if (!fieldType) {
                            fieldType = firstType;
                        }
                        break;
                    }
                }
            }
            if (entries[0]->fieldName && !info && lookupType && lookupType->userTypeName && ctx->semanticModel) {
                CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
                if (cctx) {
                    CCTagKind kind = (lookupType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
                    ASTNode* def = cc_tag_definition(cctx, kind, lookupType->userTypeName);
                    if (def && (def->type == AST_UNION_DEFINITION || def->type == AST_STRUCT_DEFINITION)) {
                        for (size_t f = 0; f < def->structDef.fieldCount; ++f) {
                            ASTNode* fieldDecl = def->structDef.fields[f];
                            if (!fieldDecl || fieldDecl->type != AST_VARIABLE_DECLARATION) continue;
                            for (size_t v = 0; v < fieldDecl->varDecl.varCount; ++v) {
                                ASTNode* nameNode = fieldDecl->varDecl.varNames[v];
                                const char* fname = (nameNode && nameNode->type == AST_IDENTIFIER)
                                    ? nameNode->valueNode.value
                                    : NULL;
                                if (fname && strcmp(fname, entries[0]->fieldName) == 0) {
                                    const ParsedType* parsed = astVarDeclTypeAt(fieldDecl, v);
                                    fieldParsed = parsed ? parsed : &fieldDecl->varDecl.declaredType;
                                    fieldType = cg_type_from_parsed(ctx, fieldParsed);
                                    if (!fieldType) {
                                        fieldType = firstType;
                                    }
                                    f = def->structDef.fieldCount;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            LLVMValueRef val = cg_build_const_initializer(ctx, entries[0]->expression, fieldType, fieldParsed);
            if (val && LLVMTypeOf(val) != firstType) {
                val = LLVMConstBitCast(val, firstType);
            }
            if (val) {
                fields[0] = val;
            }
        }
        LLVMValueRef result = LLVMConstNamedStruct(structType, fields, fieldCount);
        free(fields);
        return result;
    }

    unsigned implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned targetIndex = implicitIndex;
        const ParsedType* fieldParsed = NULL;

        bool matchedField = false;
        if (entry->fieldName && info && info->fieldCount > 0) {
            for (size_t f = 0; f < info->fieldCount; ++f) {
                if (info->fields[f].name && strcmp(info->fields[f].name, entry->fieldName) == 0) {
                    targetIndex = info->fields[f].index;
                    fieldParsed = &info->fields[f].parsedType;
                    matchedField = true;
                    break;
                }
            }
        }
        if (entry->fieldName && !matchedField) {
            const ASTNode* def = info ? (const ASTNode*)info->definition : NULL;
            if (!def && lookupType && lookupType->userTypeName && ctx->semanticModel) {
                CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
                if (cctx) {
                    CCTagKind kind = (lookupType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
                    def = cc_tag_definition(cctx, kind, lookupType->userTypeName);
                }
            }
            if (def) {
                matchedField = cg_find_field_in_definition(def, entry->fieldName, &targetIndex, &fieldParsed);
            }
        }
        implicitIndex = targetIndex + 1;

        if (targetIndex >= fieldCount) {
            continue;
        }

        LLVMTypeRef fieldType = LLVMStructGetTypeAtIndex(structType, targetIndex);
        LLVMValueRef baseFieldConst = fields[targetIndex];
        if (entry->resetSubobjectBeforeStore &&
            entry->expression->type == AST_COMPOUND_LITERAL) {
            baseFieldConst = cg_zero_const(fieldType);
        }
        LLVMValueRef fieldConst = cg_merge_const_initializer(ctx,
                                                             baseFieldConst,
                                                             entry->expression,
                                                             fieldType,
                                                             fieldParsed);
        if (!fieldConst) {
            free(fields);
            return NULL;
        }
        fields[targetIndex] = fieldConst;
    }

    LLVMValueRef result = LLVMConstNamedStruct(structType, fields, fieldCount);
    free(fields);
    return result;
}
