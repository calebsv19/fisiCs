// SPDX-License-Identifier: Apache-2.0

#include "analyze_expr_internal.h"
#include "symbol_table.h"
#include "Syntax/layout.h"
#include "Compiler/compiler_context.h"
#include <stdlib.h>
#include <string.h>

static ASTNode* resolveRecordDefinition(const TypeInfo* base, Scope* scope) {
    if (!base || !scope || !scope->ctx) {
        return NULL;
    }
    if (base->category != TYPEINFO_STRUCT && base->category != TYPEINFO_UNION) {
        return NULL;
    }
    CCTagKind kind = (base->category == TYPEINFO_STRUCT) ? CC_TAG_STRUCT : CC_TAG_UNION;
    ASTNode* def = NULL;
    if (base->originalType && base->originalType->inlineStructOrUnionDef) {
        def = base->originalType->inlineStructOrUnionDef;
    }
    if (!def &&
        base->originalType &&
        base->originalType->kind == TYPE_NAMED &&
        base->originalType->userTypeName) {
        Symbol* typeSym = resolveInScopeChain(scope, base->originalType->userTypeName);
        if (typeSym && typeSym->kind == SYMBOL_TYPEDEF) {
            if (typeSym->type.inlineStructOrUnionDef) {
                def = typeSym->type.inlineStructOrUnionDef;
            } else if (typeSym->type.userTypeName) {
                CCTagKind symKind = kind;
                if (typeSym->type.kind == TYPE_STRUCT) {
                    symKind = CC_TAG_STRUCT;
                } else if (typeSym->type.kind == TYPE_UNION) {
                    symKind = CC_TAG_UNION;
                }
                def = cc_tag_definition(scope->ctx, symKind, typeSym->type.userTypeName);
            }
        }
    }
    if (base->userTypeName) {
        if (!def) {
            def = cc_tag_definition(scope->ctx, kind, base->userTypeName);
        }
        if (!def) {
            Symbol* typeSym = resolveInScopeChain(scope, base->userTypeName);
            if (typeSym && typeSym->kind == SYMBOL_TYPEDEF) {
                if (typeSym->type.inlineStructOrUnionDef) {
                    def = typeSym->type.inlineStructOrUnionDef;
                } else if (typeSym->type.userTypeName) {
                    CCTagKind symKind = kind;
                    if (typeSym->type.kind == TYPE_STRUCT) {
                        symKind = CC_TAG_STRUCT;
                    } else if (typeSym->type.kind == TYPE_UNION) {
                        symKind = CC_TAG_UNION;
                    }
                    def = cc_tag_definition(scope->ctx, symKind, typeSym->type.userTypeName);
                }
            }
        }
    }
    if (!def || (def->type != AST_STRUCT_DEFINITION && def->type != AST_UNION_DEFINITION)) {
        return NULL;
    }
    return def;
}

static const ParsedType* lookupFieldTypeInRecordDef(ASTNode* def,
                                                    const char* fieldName,
                                                    Scope* scope) {
    if (!def || !fieldName || !scope) {
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
                ASTNode* anonDef = resolveRecordDefinition(&anonInfo, scope);
                if (anonDef) {
                    nested = lookupFieldTypeInRecordDef(anonDef, fieldName, scope);
                }
                if (!nested && anonType->inlineStructOrUnionDef) {
                    nested = lookupFieldTypeInRecordDef(anonType->inlineStructOrUnionDef, fieldName, scope);
                }
                if (nested) {
                    return nested;
                }
            }
        }
    }
    return NULL;
}

const ParsedType* analyzeExprLookupFieldType(const TypeInfo* base,
                                             const char* fieldName,
                                             Scope* scope) {
    if (!base || !fieldName || !scope || !scope->ctx) {
        return NULL;
    }
    if (base->category != TYPEINFO_STRUCT && base->category != TYPEINFO_UNION) {
        return NULL;
    }
    ASTNode* def = resolveRecordDefinition(base, scope);
    return lookupFieldTypeInRecordDef(def, fieldName, scope);
}

bool analyzeExprLookupFieldIsBitfield(const TypeInfo* base,
                                      const char* fieldName,
                                      Scope* scope) {
    if (!base || !fieldName || !scope || !scope->ctx) {
        return false;
    }
    if (base->category != TYPEINFO_STRUCT && base->category != TYPEINFO_UNION) {
        return false;
    }
    ASTNode* def = resolveRecordDefinition(base, scope);
    if (!def || (def->type != AST_STRUCT_DEFINITION && def->type != AST_UNION_DEFINITION)) {
        return false;
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
            return field->varDecl.bitFieldWidth != NULL;
        }
    }
    return false;
}

const CCTagFieldLayout* analyzeExprLookupFieldLayout(const TypeInfo* base,
                                                     const char* fieldName,
                                                     Scope* scope) {
    if (!base || !fieldName || !scope || !scope->ctx) return NULL;
    if (base->category != TYPEINFO_STRUCT && base->category != TYPEINFO_UNION) {
        return NULL;
    }
    const CCTagFieldLayout* layouts = NULL;
    size_t count = 0;
    CCTagKind kind = (base->category == TYPEINFO_STRUCT) ? CC_TAG_STRUCT : CC_TAG_UNION;
    if (base->userTypeName) {
        if (!cc_tag_is_defined(scope->ctx, kind, base->userTypeName) &&
            base->originalType &&
            base->originalType->inlineStructOrUnionDef) {
            ASTNode* def = base->originalType->inlineStructOrUnionDef;
            bool kindMatches =
                (kind == CC_TAG_STRUCT && def->type == AST_STRUCT_DEFINITION) ||
                (kind == CC_TAG_UNION && def->type == AST_UNION_DEFINITION);
            if (kindMatches) {
                (void)cc_define_tag(scope->ctx, kind, base->userTypeName, 0, def);
            }
        }
        size_t sz = 0, al = 0;
        (void)layout_struct_union(scope->ctx, scope, kind, base->userTypeName, &sz, &al);
    }
    if (!cc_get_tag_field_layouts(scope->ctx, kind, base->userTypeName, &layouts, &count) || !layouts) {
        return NULL;
    }
    for (size_t i = 0; i < count; ++i) {
        const CCTagFieldLayout* lay = &layouts[i];
        if (!lay->name) continue;
        if (strcmp(lay->name, fieldName) == 0) {
            return lay;
        }
    }
    return NULL;
}

bool evalOffsetofFieldPath(const ParsedType* baseType,
                           const char* fieldPath,
                           Scope* scope,
                           size_t* offsetOut,
                           bool* bitfieldOut) {
    if (bitfieldOut) *bitfieldOut = false;
    if (!baseType || !fieldPath || !scope || !scope->ctx || !offsetOut) {
        return false;
    }

    TypeInfo current = typeInfoFromParsedType(baseType, scope);
    if (current.category != TYPEINFO_STRUCT && current.category != TYPEINFO_UNION) {
        return false;
    }

    size_t totalOffset = 0;
    const char* cursor = fieldPath;
    while (*cursor) {
        const char* dot = strchr(cursor, '.');
        size_t segLen = dot ? (size_t)(dot - cursor) : strlen(cursor);
        if (segLen == 0) {
            return false;
        }

        char* segment = (char*)malloc(segLen + 1);
        if (!segment) {
            return false;
        }
        memcpy(segment, cursor, segLen);
        segment[segLen] = '\0';

        const CCTagFieldLayout* lay = analyzeExprLookupFieldLayout(&current, segment, scope);
        if (!lay) {
            free(segment);
            return false;
        }
        if (lay->isBitfield && !lay->isZeroWidth) {
            if (bitfieldOut) *bitfieldOut = true;
            free(segment);
            return false;
        }
        totalOffset += lay->byteOffset;

        if (!dot) {
            free(segment);
            break;
        }

        const ParsedType* nested = analyzeExprLookupFieldType(&current, segment, scope);
        free(segment);
        if (!nested) {
            return false;
        }
        current = typeInfoFromParsedType(nested, scope);
        if (current.category != TYPEINFO_STRUCT && current.category != TYPEINFO_UNION) {
            return false;
        }
        cursor = dot + 1;
    }

    *offsetOut = totalOffset;
    return true;
}
