// SPDX-License-Identifier: Apache-2.0

#include "analyze_decls_internal.h"

static bool typeInfoIsStructLike(const TypeInfo* info);
static bool inlineAggregateHasField(const ASTNode* aggregateDef, const char* fieldName);
static bool aggregateTypeHasField(const ParsedType* parsedType,
                                  const TypeInfo* info,
                                  const char* fieldName,
                                  Scope* scope);
static bool aggregateTypeCanValidateFields(const ParsedType* parsedType,
                                           const TypeInfo* info,
                                           Scope* scope);
static ASTNode* resolveRecordDefinitionForInitValidation(const TypeInfo* base, Scope* scope);
static const ParsedType* lookupFieldTypeInAggregateDefForInitValidation(ASTNode* def,
                                                                        const char* fieldName,
                                                                        Scope* scope);
static const ParsedType* lookupAggregateFieldTypeForInitValidation(const ParsedType* parsedType,
                                                                   const TypeInfo* info,
                                                                   const char* fieldName,
                                                                   Scope* scope);

static bool typeInfoIsStructLike(const TypeInfo* info) {
    return info && (info->category == TYPEINFO_STRUCT || info->category == TYPEINFO_UNION);
}

static bool inlineAggregateHasField(const ASTNode* aggregateDef, const char* fieldName) {
    if (!aggregateDef || !fieldName || !fieldName[0]) {
        return false;
    }
    if (aggregateDef->type != AST_STRUCT_DEFINITION &&
        aggregateDef->type != AST_UNION_DEFINITION) {
        return false;
    }
    for (size_t i = 0; i < aggregateDef->structDef.fieldCount; ++i) {
        ASTNode* field = aggregateDef->structDef.fields ? aggregateDef->structDef.fields[i] : NULL;
        if (!field || field->type != AST_VARIABLE_DECLARATION || !field->varDecl.varNames) {
            continue;
        }
        for (size_t k = 0; k < field->varDecl.varCount; ++k) {
            ASTNode* nameNode = field->varDecl.varNames[k];
            if (!nameNode || nameNode->type != AST_IDENTIFIER || !nameNode->valueNode.value) {
                continue;
            }
            if (strcmp(nameNode->valueNode.value, fieldName) == 0) {
                return true;
            }
        }
    }
    return false;
}

static bool aggregateTypeHasField(const ParsedType* parsedType,
                                  const TypeInfo* info,
                                  const char* fieldName,
                                  Scope* scope) {
    if (!fieldName || !fieldName[0]) {
        return true;
    }
    if (parsedType && parsedType->inlineStructOrUnionDef &&
        inlineAggregateHasField(parsedType->inlineStructOrUnionDef, fieldName)) {
        return true;
    }
    if (!scope || !scope->ctx || !info || !info->userTypeName) {
        return false;
    }
    CCTagKind kind = (info->category == TYPEINFO_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
    ASTNode* tagDef = cc_tag_definition(scope->ctx, kind, info->userTypeName);
    if (tagDef && inlineAggregateHasField(tagDef, fieldName)) {
        return true;
    }
    const CCTagFieldLayout* layouts = NULL;
    size_t count = 0;
    if (!cc_get_tag_field_layouts(scope->ctx, kind, info->userTypeName, &layouts, &count) ||
        !layouts) {
        return false;
    }
    for (size_t i = 0; i < count; ++i) {
        const CCTagFieldLayout* lay = &layouts[i];
        if (!lay->name) {
            continue;
        }
        if (strcmp(lay->name, fieldName) == 0) {
            return true;
        }
    }
    return false;
}

static bool aggregateTypeCanValidateFields(const ParsedType* parsedType,
                                           const TypeInfo* info,
                                           Scope* scope) {
    if (parsedType && parsedType->inlineStructOrUnionDef) {
        return true;
    }
    if (!scope || !scope->ctx || !info || !info->userTypeName) {
        return false;
    }
    CCTagKind kind = (info->category == TYPEINFO_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
    ASTNode* tagDef = cc_tag_definition(scope->ctx, kind, info->userTypeName);
    if (tagDef && (tagDef->type == AST_STRUCT_DEFINITION || tagDef->type == AST_UNION_DEFINITION)) {
        return true;
    }
    const CCTagFieldLayout* layouts = NULL;
    size_t count = 0;
    return cc_get_tag_field_layouts(scope->ctx, kind, info->userTypeName, &layouts, &count) &&
           layouts &&
           count > 0;
}

static ASTNode* resolveRecordDefinitionForInitValidation(const TypeInfo* base, Scope* scope) {
    if (!base || !scope || !scope->ctx) {
        return NULL;
    }
    if (base->category != TYPEINFO_STRUCT && base->category != TYPEINFO_UNION) {
        return NULL;
    }

    CCTagKind kind = (base->category == TYPEINFO_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
    ASTNode* def = NULL;
    if (base->originalType && base->originalType->inlineStructOrUnionDef) {
        def = base->originalType->inlineStructOrUnionDef;
    }
    if (base->userTypeName && !def) {
        def = cc_tag_definition(scope->ctx, kind, base->userTypeName);
    }
    if (!def || (def->type != AST_STRUCT_DEFINITION && def->type != AST_UNION_DEFINITION)) {
        return NULL;
    }
    return def;
}

static const ParsedType* lookupFieldTypeInAggregateDefForInitValidation(ASTNode* def,
                                                                        const char* fieldName,
                                                                        Scope* scope) {
    if (!def || !fieldName || !scope) {
        return NULL;
    }
    if (def->type != AST_STRUCT_DEFINITION && def->type != AST_UNION_DEFINITION) {
        return NULL;
    }

    for (size_t i = 0; i < def->structDef.fieldCount; ++i) {
        ASTNode* field = def->structDef.fields ? def->structDef.fields[i] : NULL;
        if (!field || field->type != AST_VARIABLE_DECLARATION) {
            continue;
        }
        for (size_t k = 0; k < field->varDecl.varCount; ++k) {
            ASTNode* name = field->varDecl.varNames ? field->varDecl.varNames[k] : NULL;
            const char* nameValue = name ? name->valueNode.value : NULL;
            if (!nameValue || strcmp(nameValue, fieldName) != 0) {
                continue;
            }
            if (field->varDecl.declaredTypes) {
                return &field->varDecl.declaredTypes[k];
            }
            return &field->varDecl.declaredType;
        }

        if (field->varDecl.varCount == 0) {
            const ParsedType* anonType = &field->varDecl.declaredType;
            TypeInfo anonInfo = typeInfoFromParsedType(anonType, scope);
            if (anonInfo.category == TYPEINFO_STRUCT || anonInfo.category == TYPEINFO_UNION) {
                const ParsedType* nested = NULL;
                ASTNode* anonDef = resolveRecordDefinitionForInitValidation(&anonInfo, scope);
                if (anonDef) {
                    nested = lookupFieldTypeInAggregateDefForInitValidation(anonDef, fieldName, scope);
                }
                if (!nested && anonType->inlineStructOrUnionDef) {
                    nested = lookupFieldTypeInAggregateDefForInitValidation(
                        anonType->inlineStructOrUnionDef,
                        fieldName,
                        scope);
                }
                if (nested) {
                    return nested;
                }
            }
        }
    }

    return NULL;
}

static const ParsedType* lookupAggregateFieldTypeForInitValidation(const ParsedType* parsedType,
                                                                   const TypeInfo* info,
                                                                   const char* fieldName,
                                                                   Scope* scope) {
    if (!fieldName || !fieldName[0] || !scope) {
        return NULL;
    }
    if (parsedType && parsedType->inlineStructOrUnionDef) {
        const ParsedType* fromInline =
            lookupFieldTypeInAggregateDefForInitValidation(parsedType->inlineStructOrUnionDef,
                                                           fieldName,
                                                           scope);
        if (fromInline) {
            return fromInline;
        }
    }
    if (!info) {
        return NULL;
    }
    ASTNode* def = resolveRecordDefinitionForInitValidation(info, scope);
    if (!def) {
        return NULL;
    }
    return lookupFieldTypeInAggregateDefForInitValidation(def, fieldName, scope);
}

void validateAggregateDesignatedFieldsRecursive(const ParsedType* aggregateType,
                                                const TypeInfo* aggregateInfo,
                                                ASTNode* compoundExpr,
                                                const char* variableName,
                                                Scope* scope,
                                                int fallbackLine,
                                                int depth) {
    if (!aggregateType || !aggregateInfo || !compoundExpr || !scope || depth > 16) {
        return;
    }
    if (compoundExpr->type != AST_COMPOUND_LITERAL) {
        return;
    }

    bool canValidateFields = aggregateTypeCanValidateFields(aggregateType, aggregateInfo, scope);
    for (size_t i = 0; i < compoundExpr->compoundLiteral.entryCount; ++i) {
        DesignatedInit* entry =
            compoundExpr->compoundLiteral.entries ? compoundExpr->compoundLiteral.entries[i] : NULL;
        if (!entry || !entry->fieldName) {
            continue;
        }

        if (canValidateFields &&
            !aggregateTypeHasField(aggregateType, aggregateInfo, entry->fieldName, scope)) {
            char buffer[256];
            snprintf(buffer,
                     sizeof(buffer),
                     "Unknown field '%s' in designated initializer for '%s'",
                     entry->fieldName,
                     variableName ? variableName : "<unnamed>");
            int errLine = fallbackLine;
            if (entry->expression && entry->expression->line > 0) {
                errLine = entry->expression->line;
            } else if (compoundExpr->line > 0) {
                errLine = compoundExpr->line;
            }
            ASTNode* errorNode = entry->expression ? entry->expression : compoundExpr;
            reportErrorAtAstNode(errorNode, errLine, buffer, NULL);
            continue;
        }

        if (!entry->expression || entry->expression->type != AST_COMPOUND_LITERAL) {
            continue;
        }

        const ParsedType* fieldType =
            lookupAggregateFieldTypeForInitValidation(aggregateType,
                                                      aggregateInfo,
                                                      entry->fieldName,
                                                      scope);
        if (!fieldType) {
            continue;
        }

        TypeInfo fieldInfo = typeInfoFromParsedType(fieldType, scope);
        if (!typeInfoIsStructLike(&fieldInfo)) {
            continue;
        }

        validateAggregateDesignatedFieldsRecursive(fieldType,
                                                   &fieldInfo,
                                                   entry->expression,
                                                   variableName,
                                                   scope,
                                                   fallbackLine,
                                                   depth + 1);
    }
}
