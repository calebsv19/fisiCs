#include "codegen_private.h"

#include "codegen_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void recordStructInfo(CodegenContext* ctx,
                             const char* name,
                             bool isUnion,
                             StructFieldInfo* fields,
                             size_t fieldCount,
                             LLVMTypeRef llvmType) {
    if (!ctx || !name) {
        if (fields) free(fields);
        return;
    }

    for (size_t i = 0; i < ctx->structInfoCount; ++i) {
        if (ctx->structInfos[i].name && strcmp(ctx->structInfos[i].name, name) == 0) {
            for (size_t j = 0; j < ctx->structInfos[i].fieldCount; ++j) {
                free(ctx->structInfos[i].fields[j].name);
            }
            free(ctx->structInfos[i].fields);
            ctx->structInfos[i].fields = fields;
            ctx->structInfos[i].fieldCount = fieldCount;
            ctx->structInfos[i].isUnion = isUnion;
            ctx->structInfos[i].llvmType = llvmType;
            return;
        }
    }

    if (ctx->structInfoCount == ctx->structInfoCapacity) {
        size_t newCap = ctx->structInfoCapacity ? ctx->structInfoCapacity * 2 : 4;
        StructInfo* resized = (StructInfo*)realloc(ctx->structInfos, newCap * sizeof(StructInfo));
        if (!resized) {
            for (size_t j = 0; j < fieldCount; ++j) {
                free(fields[j].name);
            }
            free(fields);
            return;
        }
        ctx->structInfos = resized;
        ctx->structInfoCapacity = newCap;
    }

    ctx->structInfos[ctx->structInfoCount].name = strdup(name);
    ctx->structInfos[ctx->structInfoCount].fields = fields;
    ctx->structInfos[ctx->structInfoCount].fieldCount = fieldCount;
    ctx->structInfos[ctx->structInfoCount].isUnion = isUnion;
    ctx->structInfos[ctx->structInfoCount].llvmType = llvmType;
    ctx->structInfoCount += 1;
}

static const StructInfo* lookupStructInfo(CodegenContext* ctx, const char* name, LLVMTypeRef llvmType) {
    if (!ctx) return NULL;
    for (size_t i = 0; i < ctx->structInfoCount; ++i) {
        if (llvmType && ctx->structInfos[i].llvmType && ctx->structInfos[i].llvmType == llvmType) {
            return &ctx->structInfos[i];
        }
        if (name && ctx->structInfos[i].name && strcmp(ctx->structInfos[i].name, name) == 0) {
            return &ctx->structInfos[i];
        }
    }
    return NULL;
}

LLVMTypeRef ensureStructLLVMTypeByName(CodegenContext* ctx, const char* name, bool isUnionHint) {
    if (!ctx || !name || name[0] == '\0') return NULL;
    ParsedType temp;
    memset(&temp, 0, sizeof(temp));
    temp.kind = isUnionHint ? TYPE_UNION : TYPE_STRUCT;
    temp.userTypeName = (char*)name;
    return cg_type_from_parsed(ctx, &temp);
}

void declareStructSymbol(CodegenContext* ctx, const Symbol* sym) {
    if (!ctx || !sym) return;
    bool isUnion = false;
    if (sym->definition &&
        (sym->definition->type == AST_STRUCT_DEFINITION || sym->definition->type == AST_UNION_DEFINITION)) {
        isUnion = (sym->definition->type == AST_UNION_DEFINITION);
        codegenStructDefinition(ctx, sym->definition);
    } else if (sym->name) {
        CGTypeCache* cache = cg_context_get_type_cache(ctx);
        if (cache) {
            cg_type_cache_lookup_struct(cache, sym->name, &isUnion);
        }
        ensureStructLLVMTypeByName(ctx, sym->name, isUnion);
    }
}

LLVMValueRef buildArrayElementPointer(CodegenContext* ctx,
                                      LLVMValueRef arrayPtr,
                                      LLVMValueRef index,
                                      const ParsedType* baseParsedHint,
                                      LLVMTypeRef aggregateTypeHint,
                                      LLVMTypeRef elementTypeHint,
                                      LLVMTypeRef* outElementType) {
    if (!ctx || !arrayPtr || !index) return NULL;

    LLVMTypeRef valueType = LLVMTypeOf(arrayPtr);
    if (!valueType) return NULL;

    LLVMValueRef basePtr = arrayPtr;
    LLVMTypeRef ptrType = valueType;
    LLVMTypeKind valueKind = LLVMGetTypeKind(valueType);

    if (valueKind == LLVMArrayTypeKind) {
        LLVMValueRef temp = LLVMBuildAlloca(ctx->builder, valueType, "array.decay.tmp");
        LLVMBuildStore(ctx->builder, arrayPtr, temp);
        basePtr = temp;
        ptrType = LLVMTypeOf(basePtr);
        aggregateTypeHint = aggregateTypeHint ? aggregateTypeHint : valueType;
        valueKind = LLVMGetTypeKind(ptrType);
    }

    if (valueKind != LLVMPointerTypeKind) {
        LLVMValueRef temp = LLVMBuildAlloca(ctx->builder, valueType, "array.ptr.wrap");
        LLVMBuildStore(ctx->builder, arrayPtr, temp);
        basePtr = temp;
        ptrType = LLVMTypeOf(basePtr);
        valueKind = LLVMGetTypeKind(ptrType);
    }

    if (!ptrType || valueKind != LLVMPointerTypeKind) {
        return NULL;
    }

    unsigned pointerAddrSpace = LLVMGetPointerAddressSpace(ptrType);

    LLVMTypeRef aggregateType = aggregateTypeHint;
    if (!aggregateType && baseParsedHint) {
        if (parsedTypeIsDirectArray(baseParsedHint)) {
            aggregateType = cg_type_from_parsed(ctx, baseParsedHint);
        } else {
            ParsedType pointed = parsedTypePointerTargetType(baseParsedHint);
            if (parsedTypeIsDirectArray(&pointed)) {
                aggregateType = cg_type_from_parsed(ctx, &pointed);
            }
            parsedTypeFree(&pointed);
        }
    }

    LLVMTypeRef i32Type = LLVMInt32TypeInContext(ctx->llvmContext);
    LLVMValueRef idx = index;
    LLVMTypeRef idxType = LLVMTypeOf(idx);
    if (!idxType || LLVMGetTypeKind(idxType) != LLVMIntegerTypeKind || LLVMGetIntTypeWidth(idxType) != 32) {
        idx = LLVMBuildIntCast2(ctx->builder, idx, i32Type, 0, "array.idx.cast");
    }

    LLVMTypeRef elementType = elementTypeHint;
    if (aggregateType && LLVMGetTypeKind(aggregateType) == LLVMArrayTypeKind) {
        LLVMValueRef zero = LLVMConstInt(i32Type, 0, 0);
        LLVMValueRef indices[2] = { zero, idx };
        LLVMTypeRef ptrToAggregate = LLVMPointerType(aggregateType, pointerAddrSpace);
        LLVMValueRef aggregatePtr = basePtr;
        if (LLVMTypeOf(aggregatePtr) != ptrToAggregate) {
            aggregatePtr = LLVMBuildBitCast(ctx->builder, basePtr, ptrToAggregate, "array.aggregate.cast");
        }
        if (!elementType) {
            elementType = LLVMGetElementType(aggregateType);
        }
        if (outElementType && elementType) {
            *outElementType = elementType;
        }
        return LLVMBuildGEP2(ctx->builder, aggregateType, aggregatePtr, indices, 2, "arrayElemPtr");
    }

    if (!elementType) {
        elementType = cg_element_type_from_pointer(ctx, baseParsedHint, ptrType);
    }
    if (!elementType || LLVMGetTypeKind(elementType) == LLVMVoidTypeKind) {
        elementType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    LLVMTypeRef expectedPtrType = LLVMPointerType(elementType, pointerAddrSpace);
    if (LLVMTypeOf(basePtr) != expectedPtrType) {
        basePtr = LLVMBuildBitCast(ctx->builder, basePtr, expectedPtrType, "array.elem.base.cast");
    }

    if (outElementType) {
        *outElementType = elementType;
    }
    return LLVMBuildGEP2(ctx->builder, elementType, basePtr, &idx, 1, "arrayElemPtr");
}

static CGStructLLVMInfo* findStructInfoForAggregate(CodegenContext* ctx,
                                                    LLVMTypeRef aggregateType,
                                                    const char* structNameHint,
                                                    const ParsedType* parsedHint) {
    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    if (!cache) {
        return NULL;
    }
    if (structNameHint && structNameHint[0]) {
        CGStructLLVMInfo* info = cg_type_cache_get_struct_info(cache, structNameHint);
        if (info) {
            return info;
        }
    }
    if (parsedHint && parsedHint->userTypeName) {
        CGStructLLVMInfo* info = cg_type_cache_get_struct_info(cache, parsedHint->userTypeName);
        if (info) {
            return info;
        }
    }
    if (aggregateType) {
        CGStructLLVMInfo* info = cg_type_cache_find_struct_by_llvm(cache, aggregateType);
        if (info) {
            return info;
        }
    }
    return NULL;
}

LLVMValueRef buildStructFieldPointer(CodegenContext* ctx,
                                     LLVMValueRef basePtr,
                                     LLVMTypeRef aggregateTypeHint,
                                     const char* structName,
                                     const char* fieldName,
                                     const ParsedType* structParsedHint,
                                     LLVMTypeRef* outFieldType,
                                     const ParsedType** outFieldParsedType) {
    if (!ctx || !basePtr || !fieldName) {
        return NULL;
    }

    LLVMTypeRef ptrType = LLVMTypeOf(basePtr);
    if (LLVMGetTypeKind(ptrType) != LLVMPointerTypeKind) {
        return NULL;
    }

    LLVMTypeRef aggregateType = aggregateTypeHint;
    if (!aggregateType) {
        aggregateType = LLVMGetElementType(ptrType);
    }
    if ((!aggregateType || LLVMGetTypeKind(aggregateType) == LLVMVoidTypeKind) && LLVMIsAAllocaInst(basePtr)) {
        LLVMTypeRef allocated = LLVMGetAllocatedType(basePtr);
        if (allocated) {
            aggregateType = allocated;
        }
    }
    if (!aggregateType || LLVMGetTypeKind(aggregateType) != LLVMStructTypeKind) {
        return NULL;
    }

    const char* structNameHint = structName ? structName : LLVMGetStructName(aggregateType);
    unsigned fieldIndex = 0;
    LLVMTypeRef fieldLLVMType = NULL;
    bool found = false;

    bool isUnion = false;
    CGStructLLVMInfo* semanticInfo =
        findStructInfoForAggregate(ctx, aggregateType, structNameHint, structParsedHint);
    if (semanticInfo) {
        isUnion = semanticInfo->isUnion;
        if (!semanticInfo->llvmType && structNameHint) {
            semanticInfo->llvmType = ensureStructLLVMTypeByName(ctx, structNameHint, isUnion);
        }
        for (size_t i = 0; i < semanticInfo->fieldCount; ++i) {
            if (semanticInfo->fields[i].name && strcmp(semanticInfo->fields[i].name, fieldName) == 0) {
                fieldIndex = semanticInfo->isUnion ? 0 : semanticInfo->fields[i].index;
                fieldLLVMType = cg_type_from_parsed(ctx, &semanticInfo->fields[i].parsedType);
                if (outFieldParsedType) {
                    *outFieldParsedType = &semanticInfo->fields[i].parsedType;
                }
                found = true;
                break;
            }
        }
    }

    if (!found) {
        const StructInfo* legacy = lookupStructInfo(ctx, structNameHint, aggregateType);
        if (legacy) {
            for (size_t i = 0; i < legacy->fieldCount; ++i) {
                if (legacy->fields[i].name && strcmp(legacy->fields[i].name, fieldName) == 0) {
                    fieldIndex = legacy->isUnion ? 0 : legacy->fields[i].index;
                    fieldLLVMType = legacy->fields[i].type;
                    found = true;
                    break;
                }
            }
        }
    }

    if (!found) {
        CG_DEBUG("struct lookup failed for %s.%s (structInfos=%zu)",
                 structNameHint ? structNameHint : "<anon>",
                 fieldName,
                 ctx->structInfoCount);
        return NULL;
    }

    if (!fieldLLVMType || LLVMGetTypeKind(fieldLLVMType) == LLVMVoidTypeKind) {
        fieldLLVMType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    if (outFieldType) {
        *outFieldType = fieldLLVMType;
    }

    return LLVMBuildStructGEP2(ctx->builder, aggregateType, basePtr, fieldIndex, "fieldPtr");
}

bool codegenLValue(CodegenContext* ctx,
                   ASTNode* target,
                   LLVMValueRef* outPtr,
                   LLVMTypeRef* outType,
                   const ParsedType** outParsedType) {
    if (!ctx || !target || !outPtr || !outType) return false;
    if (outParsedType) {
        *outParsedType = NULL;
    }

    switch (target->type) {
        case AST_IDENTIFIER: {
            const char* name = target->valueNode.value;
            NamedValue* entry = cg_scope_lookup(ctx->currentScope, name);
            if (!entry) {
                const SemanticModel* model = cg_context_get_semantic_model(ctx);
                if (model) {
                    const Symbol* sym = semanticModelLookupGlobal(model, name);
                    if (sym) {
                        if (sym->kind == SYMBOL_VARIABLE) {
                            declareGlobalVariableSymbol(ctx, sym);
                        } else if (sym->kind == SYMBOL_FUNCTION) {
                            declareFunctionSymbol(ctx, sym);
                        }
                        entry = cg_scope_lookup(ctx->currentScope, name);
                    }
                }
            }
            if (!entry) {
                fprintf(stderr, "Error: assignment target '%s' not declared\n", name ? name : "<anonymous>");
                return false;
            }
            *outPtr = entry->value;
            if (entry->addressOnly) {
                *outType = entry->type;
                if (outParsedType) {
                    *outParsedType = entry->parsedType;
                }
                return true;
            }
            *outType = entry->type ? entry->type : LLVMGetElementType(LLVMTypeOf(entry->value));
            if (outParsedType) {
                *outParsedType = entry->parsedType;
            }
            return true;
        }
        case AST_ARRAY_ACCESS: {
            LLVMValueRef arrayPtr = codegenNode(ctx, target->arrayAccess.array);
            LLVMValueRef index = codegenNode(ctx, target->arrayAccess.index);
            const ParsedType* baseParsed = cg_resolve_expression_type(ctx, target->arrayAccess.array);
            LLVMTypeRef aggregateHint = NULL;
            if (baseParsed && parsedTypeIsDirectArray(baseParsed)) {
                aggregateHint = cg_type_from_parsed(ctx, baseParsed);
            }
            LLVMTypeRef elementHint = cg_element_type_hint_from_parsed(ctx, baseParsed);
            LLVMTypeRef elementType = NULL;
            LLVMValueRef elementPtr = buildArrayElementPointer(ctx,
                                                               arrayPtr,
                                                               index,
                                                               baseParsed,
                                                               aggregateHint,
                                                               elementHint,
                                                               &elementType);
            if (!elementPtr) return false;
            *outPtr = elementPtr;
            if (!elementType) {
                elementType = LLVMGetElementType(LLVMTypeOf(elementPtr));
                if (!elementType || LLVMGetTypeKind(elementType) == LLVMVoidTypeKind) {
                    elementType = LLVMInt32TypeInContext(ctx->llvmContext);
                }
            }
            *outType = elementType;
            return true;
        }
        case AST_POINTER_DEREFERENCE: {
            LLVMValueRef pointerValue = codegenNode(ctx, target->pointerDeref.pointer);
            if (!pointerValue) return false;
            const ParsedType* ptrParsed = cg_resolve_expression_type(ctx, target->pointerDeref.pointer);
            LLVMTypeRef elemType = cg_element_type_from_pointer(ctx, ptrParsed, LLVMTypeOf(pointerValue));
            if (!elemType || LLVMGetTypeKind(elemType) == LLVMVoidTypeKind) {
                elemType = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            *outPtr = pointerValue;
            *outType = elemType;
            return true;
        }
        case AST_DOT_ACCESS: {
            LLVMValueRef basePtr = NULL;
            LLVMTypeRef baseType = NULL;
            const ParsedType* baseParsed = NULL;
            if (!codegenLValue(ctx, target->memberAccess.base, &basePtr, &baseType, &baseParsed)) {
                LLVMValueRef baseVal = codegenNode(ctx, target->memberAccess.base);
                if (!baseVal) return false;
                if (LLVMGetTypeKind(LLVMTypeOf(baseVal)) == LLVMPointerTypeKind) {
                    basePtr = baseVal;
                    baseType = LLVMGetElementType(LLVMTypeOf(baseVal));
                } else {
                    LLVMValueRef tmpAlloca = LLVMBuildAlloca(ctx->builder, LLVMTypeOf(baseVal), "dot_tmp_lhs");
                    LLVMBuildStore(ctx->builder, baseVal, tmpAlloca);
                    basePtr = tmpAlloca;
                    baseType = LLVMTypeOf(baseVal);
                }
                if (!baseParsed) {
                    baseParsed = cg_resolve_expression_type(ctx, target->memberAccess.base);
                }
            }

            const char* nameHint = NULL;
            if (baseType && LLVMGetTypeKind(baseType) == LLVMStructTypeKind) {
                nameHint = LLVMGetStructName(baseType);
            }

            LLVMTypeRef fieldType = NULL;
            const ParsedType* fieldParsed = NULL;
            LLVMValueRef fieldPtr = buildStructFieldPointer(ctx,
                                                            basePtr,
                                                            baseType,
                                                            nameHint,
                                                            target->memberAccess.field,
                                                            baseParsed,
                                                            &fieldType,
                                                            &fieldParsed);
            if (!fieldPtr) return false;
            *outPtr = fieldPtr;
            if (!fieldType || LLVMGetTypeKind(fieldType) == LLVMVoidTypeKind) {
                fieldType = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            *outType = fieldType;
            if (outParsedType) {
                *outParsedType = fieldParsed;
            }
            return true;
        }
        case AST_POINTER_ACCESS: {
            LLVMValueRef baseValue = codegenNode(ctx, target->memberAccess.base);
            if (!baseValue) return false;
            if (LLVMGetTypeKind(LLVMTypeOf(baseValue)) != LLVMPointerTypeKind) {
                fprintf(stderr, "Error: Cannot apply '->' to non-pointer type\n");
                return false;
            }
            LLVMValueRef basePtr = baseValue;
            LLVMTypeRef baseType = LLVMGetElementType(LLVMTypeOf(baseValue));
            LLVMTypeRef fieldType = NULL;
            const ParsedType* fieldParsed = NULL;
            const ParsedType* baseParsed = cg_resolve_expression_type(ctx, target->memberAccess.base);
            ParsedType pointed = parsedTypePointerTargetType(baseParsed);
            const ParsedType* structHint = (pointed.kind != TYPE_INVALID) ? &pointed : baseParsed;
            LLVMValueRef fieldPtr = buildStructFieldPointer(ctx,
                                                            basePtr,
                                                            baseType,
                                                            LLVMGetStructName(baseType),
                                                            target->memberAccess.field,
                                                            structHint,
                                                            &fieldType,
                                                            &fieldParsed);
            if (!fieldPtr) return false;
            *outPtr = fieldPtr;
            *outType = fieldType ? fieldType : LLVMInt32TypeInContext(ctx->llvmContext);
            if (outParsedType) {
                *outParsedType = fieldParsed;
            }
            if (structHint == &pointed) {
                parsedTypeFree(&pointed);
            }
            return true;
        }
        default:
            break;
    }
    fprintf(stderr, "Error: expression is not assignable\n");
    return false;
}

LLVMTypeRef codegenStructDefinition(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_STRUCT_DEFINITION && node->type != AST_UNION_DEFINITION) {
        fprintf(stderr, "Error: Invalid node type for codegenStructDefinition\n");
        return NULL;
    }

    CG_DEBUG("[CG] Struct/Union definition node\n");
    ASTNode* nameNode = node->structDef.structName;
    const char* structName = (nameNode && nameNode->type == AST_IDENTIFIER) ? nameNode->valueNode.value : NULL;
    bool isUnion = (node->type == AST_UNION_DEFINITION);
    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    CGStructLLVMInfo* semanticInfo = NULL;
    if (cache) {
        if (structName && structName[0]) {
            semanticInfo = cg_type_cache_get_struct_info(cache, structName);
        }
        if (!semanticInfo) {
            semanticInfo = cg_type_cache_get_struct_by_definition(cache, node);
        }
    }

    LLVMTypeRef structType = NULL;
    if (semanticInfo && semanticInfo->llvmType) {
        structType = semanticInfo->llvmType;
    }

    if (!structType) {
        if (structName && structName[0]) {
            structType = ensureStructLLVMTypeByName(ctx, structName, isUnion);
        } else {
            char anonName[64];
            snprintf(anonName,
                     sizeof(anonName),
                     isUnion ? "anon.union.%p" : "anon.struct.%p",
                     (void*)node);
            structType = cg_context_lookup_named_type(ctx, anonName);
            if (!structType) {
                structType = LLVMStructCreateNamed(ctx->llvmContext, anonName);
                cg_context_cache_named_type(ctx, anonName, structType);
            }
            structName = anonName;
        }
    }

    if (semanticInfo && !semanticInfo->llvmType) {
        semanticInfo->llvmType = structType;
    }

    if (!LLVMIsOpaqueStruct(structType)) {
        return structType;
    }

    size_t totalFields = 0;
    for (size_t i = 0; i < node->structDef.fieldCount; ++i) {
        ASTNode* fieldNode = node->structDef.fields[i];
        if (!fieldNode) continue;
        if (fieldNode->type == AST_VARIABLE_DECLARATION) {
            totalFields += fieldNode->varDecl.varCount;
        }
    }

    if (totalFields == 0) {
        LLVMStructSetBody(structType, NULL, 0, 0);
        recordStructInfo(ctx, structName, isUnion, NULL, 0, structType);
        return structType;
    }

    LLVMTypeRef* fieldTypes = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * totalFields);
    StructFieldInfo* infos = (StructFieldInfo*)calloc(totalFields, sizeof(StructFieldInfo));
    if (!fieldTypes || !infos) {
        free(fieldTypes);
        free(infos);
        return NULL;
    }

    size_t fieldIndex = 0;
    for (size_t i = 0; i < node->structDef.fieldCount; ++i) {
        ASTNode* fieldNode = node->structDef.fields[i];
        if (!fieldNode) continue;
        if (fieldNode->type != AST_VARIABLE_DECLARATION) {
            fprintf(stderr, "Warning: Unsupported struct member node type %d\n", fieldNode->type);
            continue;
        }

        for (size_t v = 0; v < fieldNode->varDecl.varCount; ++v) {
            if (fieldIndex >= totalFields) break;
            ASTNode* nameNodeField = fieldNode->varDecl.varNames[v];
            const char* fname = (nameNodeField && nameNodeField->type == AST_IDENTIFIER)
                ? nameNodeField->valueNode.value
                : NULL;

            const ParsedType* parsed = astVarDeclTypeAt(fieldNode, v);
            LLVMTypeRef memberType = cg_type_from_parsed(ctx, parsed);
            if (!memberType || LLVMGetTypeKind(memberType) == LLVMVoidTypeKind) {
                memberType = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            char* memberTypeStr = LLVMPrintTypeToString(memberType);
            CG_DEBUG("[CG] Struct member type=%s\n", memberTypeStr ? memberTypeStr : "<null>");
            if (memberTypeStr) LLVMDisposeMessage(memberTypeStr);

            fieldTypes[fieldIndex] = memberType;
            infos[fieldIndex].name = fname ? strdup(fname) : NULL;
            infos[fieldIndex].index = isUnion ? 0 : (unsigned)fieldIndex;
            infos[fieldIndex].type = memberType;
            infos[fieldIndex].parsedType = parsed ? *parsed : fieldNode->varDecl.declaredType;
            fieldIndex++;
        }
    }

    LLVMStructSetBody(structType, fieldTypes, (unsigned)fieldIndex, 0);
    recordStructInfo(ctx, structName, isUnion, infos, fieldIndex, structType);
    free(fieldTypes);
    return structType;
}


LLVMValueRef codegenStructFieldAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_STRUCT_FIELD_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenStructFieldAccess\n");
        return NULL;
    }

    LLVMValueRef structPtr = NULL;
    LLVMTypeRef baseType = NULL;
    const ParsedType* baseParsed = NULL;
    if (!codegenLValue(ctx, node->structFieldAccess.structInstance, &structPtr, &baseType, &baseParsed)) {
        structPtr = codegenNode(ctx, node->structFieldAccess.structInstance);
        if (!structPtr) {
            fprintf(stderr, "Error: Failed to generate struct instance\n");
            return NULL;
        }
        baseType = LLVMTypeOf(structPtr);
        if (LLVMGetTypeKind(baseType) == LLVMPointerTypeKind) {
            baseType = LLVMGetElementType(baseType);
        }
        if (!baseParsed) {
            baseParsed = cg_resolve_expression_type(ctx, node->structFieldAccess.structInstance);
        }
    }

    const char* nameHint = NULL;
    if (baseType && LLVMGetTypeKind(baseType) == LLVMStructTypeKind) {
        nameHint = LLVMGetStructName(baseType);
    }

    LLVMTypeRef fieldType = NULL;
    LLVMValueRef fieldPtr = buildStructFieldPointer(ctx,
                                                    structPtr,
                                                    baseType,
                                                    nameHint,
                                                    node->structFieldAccess.fieldName,
                                                    baseParsed,
                                                    &fieldType,
                                                    NULL);
    if (!fieldPtr) {
        fprintf(stderr, "Error: Unknown struct field %s\n", node->structFieldAccess.fieldName);
        return NULL;
    }

    if (!fieldType || LLVMGetTypeKind(fieldType) == LLVMVoidTypeKind) {
        fieldType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    return LLVMBuildLoad2(ctx->builder, fieldType, fieldPtr, "fieldLoad");
}
