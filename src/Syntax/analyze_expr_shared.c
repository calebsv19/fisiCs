// SPDX-License-Identifier: Apache-2.0

#include "analyze_expr_internal.h"
#include "const_eval.h"
#include "syntax_errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool parsedTypePrependPointer(ParsedType* t) {
    if (!t) return false;
    TypeDerivation* grown =
        (TypeDerivation*)realloc(t->derivations, sizeof(TypeDerivation) * (t->derivationCount + 1));
    if (!grown) return false;
    t->derivations = grown;
    if (t->derivationCount > 0) {
        memmove(t->derivations + 1, t->derivations, sizeof(TypeDerivation) * t->derivationCount);
    }
    t->derivations[0].kind = TYPE_DERIVATION_POINTER;
    t->derivations[0].as.pointer.isConst = false;
    t->derivations[0].as.pointer.isVolatile = false;
    t->derivations[0].as.pointer.isRestrict = false;
    t->derivationCount += 1;
    parsedTypeAddPointerDepth(t, 1);
    return true;
}

void typeInfoAdoptParsedType(TypeInfo* info, ParsedType* ownedParsed) {
    if (!info || !ownedParsed) return;
    ParsedType* heap = (ParsedType*)malloc(sizeof(ParsedType));
    if (!heap) {
        parsedTypeFree(ownedParsed);
        memset(ownedParsed, 0, sizeof(*ownedParsed));
        ownedParsed->kind = TYPE_INVALID;
        info->originalType = NULL;
        return;
    }
    *heap = *ownedParsed;
    memset(ownedParsed, 0, sizeof(*ownedParsed));
    ownedParsed->kind = TYPE_INVALID;
    info->originalType = heap;
}

void reportOperandError(ASTNode* node, const char* expectation, const char* op) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Operator '%s' requires %s", op ? op : "?", expectation);
    SourceRange loc = node ? node->location : (SourceRange){0};
    SourceRange callSite = node ? node->macroCallSite : (SourceRange){0};
    SourceRange macroDef = node ? node->macroDefinition : (SourceRange){0};
    ASTNode* child = NULL;
    if (node && (node->type == AST_BINARY_EXPRESSION ||
                 node->type == AST_UNARY_EXPRESSION ||
                 node->type == AST_ALIGNOF ||
                 node->type == AST_SIZEOF ||
                 node->type == AST_CAST_EXPRESSION)) {
        child = node->expr.left ? node->expr.left : node->expr.right;
    }
    if (child) {
        if ((!loc.start.file || loc.start.column <= 0) && child->location.start.file) {
            loc = child->location;
        }
        if (!callSite.start.file && child->macroCallSite.start.file) {
            callSite = child->macroCallSite;
        }
        if (!macroDef.start.file && child->macroDefinition.start.file) {
            macroDef = child->macroDefinition;
        }
    }
    addErrorWithRanges(loc, callSite, macroDef, buffer, NULL);
}

void reportNodeError(ASTNode* node, const char* message, const char* hint) {
    if (!node) {
        addError(0, 0, message, hint);
        return;
    }
    SourceRange loc = node->location;
    SourceRange callSite = node->macroCallSite;
    SourceRange macroDef = node->macroDefinition;
    ASTNode* child = NULL;
    if (node->type == AST_BINARY_EXPRESSION ||
        node->type == AST_UNARY_EXPRESSION ||
        node->type == AST_ALIGNOF ||
        node->type == AST_SIZEOF ||
        node->type == AST_CAST_EXPRESSION) {
        child = node->expr.left ? node->expr.left : node->expr.right;
    } else if (node->type == AST_ASSIGNMENT) {
        child = node->assignment.target ? node->assignment.target : node->assignment.value;
    }
    if (child) {
        if ((!loc.start.file || loc.start.column <= 0) && child->location.start.file) {
            loc = child->location;
        }
        if (!callSite.start.file && child->macroCallSite.start.file) {
            callSite = child->macroCallSite;
        }
        if (!macroDef.start.file && child->macroDefinition.start.file) {
            macroDef = child->macroDefinition;
        }
    }
    addErrorWithRanges(loc, callSite, macroDef, message, hint);
}

bool typeInfoIsKnown(const TypeInfo* info) {
    return info && info->category != TYPEINFO_INVALID;
}

bool isNullPointerConstant(ASTNode* expr, Scope* scope) {
    ConstEvalResult res = constEval(expr, scope, true);
    return res.isConst && res.value == 0;
}

void restoreBaseCategory(TypeInfo* info) {
    if (!info || info->pointerDepth != 0) return;
    if (info->isFunction) {
        info->category = TYPEINFO_FUNCTION;
        return;
    }
    switch (info->tag) {
        case TAG_STRUCT: info->category = TYPEINFO_STRUCT; return;
        case TAG_UNION: info->category = TYPEINFO_UNION; return;
        case TAG_ENUM: info->category = TYPEINFO_ENUM; return;
        default: break;
    }
    if (info->primitive == TOKEN_FLOAT || info->primitive == TOKEN_DOUBLE) {
        info->category = TYPEINFO_FLOAT;
    } else if (info->primitive == TOKEN_VOID) {
        info->category = TYPEINFO_VOID;
    } else {
        info->category = TYPEINFO_INTEGER;
    }
}

bool isExpressionNodeType(ASTNodeType type) {
    switch (type) {
        case AST_ASSIGNMENT:
        case AST_BINARY_EXPRESSION:
        case AST_UNARY_EXPRESSION:
        case AST_TERNARY_EXPRESSION:
        case AST_COMMA_EXPRESSION:
        case AST_CAST_EXPRESSION:
        case AST_COMPOUND_LITERAL:
        case AST_ARRAY_ACCESS:
        case AST_POINTER_ACCESS:
        case AST_POINTER_DEREFERENCE:
        case AST_FUNCTION_CALL:
        case AST_IDENTIFIER:
        case AST_NUMBER_LITERAL:
        case AST_CHAR_LITERAL:
        case AST_STRING_LITERAL:
        case AST_SIZEOF:
        case AST_STATEMENT_EXPRESSION:
            return true;
        default:
            return false;
    }
}

bool parsedTypeTopLevelConst(const ParsedType* type) {
    if (!type) return false;
    if (type->derivationCount > 0) {
        const TypeDerivation* outer = NULL;
        for (size_t i = 0; i < type->derivationCount; ++i) {
            outer = parsedTypeGetDerivation(type, i);
            if (outer && outer->kind == TYPE_DERIVATION_POINTER) {
                break;
            }
        }
        if (outer && outer->kind == TYPE_DERIVATION_POINTER) {
            return outer->as.pointer.isConst;
        }
    }
    return type->isConst;
}
