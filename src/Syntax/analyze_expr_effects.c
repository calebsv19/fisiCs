// SPDX-License-Identifier: Apache-2.0

#include "analyze_expr_internal.h"
#include "syntax_errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* name;
    bool read;
    bool write;
} AccessEntry;

typedef struct {
    AccessEntry* entries;
    size_t count;
    size_t capacity;
} AccessSet;

static char* dupString(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* out = malloc(len);
    if (!out) return NULL;
    memcpy(out, s, len);
    return out;
}

static void accessSetFree(AccessSet* set) {
    if (!set) return;
    for (size_t i = 0; i < set->count; ++i) {
        free(set->entries[i].name);
        set->entries[i].name = NULL;
    }
    free(set->entries);
    set->entries = NULL;
    set->count = 0;
    set->capacity = 0;
}

static void accessSetAddOwned(AccessSet* set, char* name, bool asRead, bool asWrite) {
    if (!set || !name || (!asRead && !asWrite)) {
        free(name);
        return;
    }
    for (size_t i = 0; i < set->count; ++i) {
        if (set->entries[i].name == name ||
            (set->entries[i].name && name && strcmp(set->entries[i].name, name) == 0)) {
            set->entries[i].read |= asRead;
            set->entries[i].write |= asWrite;
            free(name);
            return;
        }
    }
    if (set->count == set->capacity) {
        size_t newCap = set->capacity == 0 ? 8 : set->capacity * 2;
        AccessEntry* grown = realloc(set->entries, newCap * sizeof(AccessEntry));
        if (!grown) {
            free(name);
            return;
        }
        set->entries = grown;
        set->capacity = newCap;
    }
    set->entries[set->count].name = name;
    set->entries[set->count].read = asRead;
    set->entries[set->count].write = asWrite;
    set->count++;
}

static void accessSetAddCopy(AccessSet* set, const char* name, bool asRead, bool asWrite) {
    if (!name) return;
    char* copy = dupString(name);
    if (!copy) return;
    accessSetAddOwned(set, copy, asRead, asWrite);
}

static void accessSetMerge(AccessSet* dst, const AccessSet* src) {
    if (!dst || !src) return;
    for (size_t i = 0; i < src->count; ++i) {
        accessSetAddCopy(dst, src->entries[i].name, src->entries[i].read, src->entries[i].write);
    }
}

static void warnUnsequenced(const ASTNode* atNode, const char* name) {
    if (!name) return;
    char buf[160];
    snprintf(buf, sizeof(buf), "Unsequenced modification and access of '%s'", name);
    int line = atNode ? atNode->line : 0;
    int column = 0;
    if (atNode) {
        line = atNode->location.start.line ? atNode->location.start.line : line;
        column = atNode->location.start.column;
    }
    addWarning(line, column, buf, NULL);
}

static void mergeUnsequenced(const ASTNode* atNode, AccessSet* accum, const AccessSet* other) {
    if (!accum || !other) return;
    for (size_t i = 0; i < other->count; ++i) {
        const AccessEntry* e = &other->entries[i];
        for (size_t j = 0; j < accum->count; ++j) {
            AccessEntry* a = &accum->entries[j];
            if ((a->name == e->name) || (a->name && e->name && strcmp(a->name, e->name) == 0)) {
                bool conflict = (a->write && (e->write || e->read)) ||
                                (e->write && (a->write || a->read));
                if (conflict) {
                    warnUnsequenced(atNode, a->name ? a->name : e->name);
                }
                break;
            }
        }
    }
    accessSetMerge(accum, other);
}

char* analyzeExprAccessPath(ASTNode* node) {
    if (!node) return NULL;
    switch (node->type) {
        case AST_IDENTIFIER:
            return node->valueNode.value ? dupString(node->valueNode.value) : NULL;
        case AST_POINTER_ACCESS:
        case AST_DOT_ACCESS: {
            char* base = analyzeExprAccessPath(node->memberAccess.base);
            if (!base || !node->memberAccess.field) {
                free(base);
                return NULL;
            }
            const char* sep = (node->type == AST_POINTER_ACCESS) ? "->" : ".";
            size_t len = strlen(base) + strlen(sep) + strlen(node->memberAccess.field) + 1;
            char* merged = malloc(len);
            if (!merged) {
                free(base);
                return NULL;
            }
            snprintf(merged, len, "%s%s%s", base, sep, node->memberAccess.field);
            free(base);
            return merged;
        }
        case AST_ARRAY_ACCESS:
            return analyzeExprAccessPath(node->arrayAccess.array);
        case AST_POINTER_DEREFERENCE: {
            char* base = analyzeExprAccessPath(node->pointerDeref.pointer);
            if (!base) return NULL;
            size_t len = strlen(base) + 2;
            char* merged = malloc(len);
            if (!merged) {
                free(base);
                return NULL;
            }
            snprintf(merged, len, "*%s", base);
            free(base);
            return merged;
        }
        case AST_UNARY_EXPRESSION:
            if (node->expr.op && strcmp(node->expr.op, "&") == 0) {
                return analyzeExprAccessPath(node->expr.left);
            }
            return NULL;
        default:
            return NULL;
    }
}

static AccessSet analyzeEffects(ASTNode* node, Scope* scope, bool asRead, bool asWrite);

static AccessSet analyzeEffectsBinary(ASTNode* node, Scope* scope) {
    const char* op = node->expr.op;
    bool isComma = op && strcmp(op, ",") == 0;
    bool isLogical = op && (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0);
    AccessSet left = analyzeEffects(node->expr.left, scope, true, false);
    AccessSet right = analyzeEffects(node->expr.right, scope, true, false);
    if (isComma || isLogical) {
        accessSetMerge(&left, &right);
    } else {
        mergeUnsequenced(node, &left, &right);
    }
    accessSetFree(&right);
    return left;
}

static AccessSet analyzeEffects(ASTNode* node, Scope* scope, bool asRead, bool asWrite) {
    (void)scope;
    AccessSet set = {0};
    if (!node) return set;

    switch (node->type) {
        case AST_IDENTIFIER:
            accessSetAddOwned(&set, analyzeExprAccessPath(node), asRead, asWrite);
            break;
        case AST_NUMBER_LITERAL:
        case AST_CHAR_LITERAL:
        case AST_STRING_LITERAL:
            break;
        case AST_ASSIGNMENT: {
            bool isCompound = node->assignment.op && strcmp(node->assignment.op, "=") != 0;
            AccessSet lhs = analyzeEffects(node->assignment.target, scope, isCompound, true);
            AccessSet rhs = analyzeEffects(node->assignment.value, scope, true, false);
            accessSetMerge(&lhs, &rhs);
            accessSetFree(&rhs);
            set = lhs;
            break;
        }
        case AST_BINARY_EXPRESSION:
            set = analyzeEffectsBinary(node, scope);
            break;
        case AST_COMMA_EXPRESSION:
            for (size_t i = 0; i < node->commaExpr.exprCount; ++i) {
                AccessSet part = analyzeEffects(node->commaExpr.expressions[i], scope, true, false);
                accessSetMerge(&set, &part);
                accessSetFree(&part);
            }
            break;
        case AST_UNARY_EXPRESSION: {
            const char* op = node->expr.op;
            bool isIncDec = op && ((strcmp(op, "++") == 0) || (strcmp(op, "--") == 0));
            AccessSet inner = analyzeEffects(node->expr.left, scope, true, isIncDec);
            accessSetMerge(&set, &inner);
            accessSetFree(&inner);
            break;
        }
        case AST_TERNARY_EXPRESSION: {
            AccessSet cond = analyzeEffects(node->ternaryExpr.condition, scope, true, false);
            AccessSet tset = analyzeEffects(node->ternaryExpr.trueExpr, scope, true, false);
            AccessSet fset = analyzeEffects(node->ternaryExpr.falseExpr, scope, true, false);
            accessSetMerge(&cond, &tset);
            accessSetMerge(&cond, &fset);
            set = cond;
            accessSetFree(&tset);
            accessSetFree(&fset);
            break;
        }
        case AST_ARRAY_ACCESS: {
            AccessSet base = analyzeEffects(node->arrayAccess.array, scope, false, false);
            AccessSet idx = analyzeEffects(node->arrayAccess.index, scope, true, false);
            mergeUnsequenced(node, &base, &idx);
            set = base;
            accessSetFree(&idx);
            break;
        }
        case AST_POINTER_ACCESS:
        case AST_DOT_ACCESS: {
            AccessSet base = analyzeEffects(node->memberAccess.base, scope, false, false);
            accessSetMerge(&set, &base);
            accessSetFree(&base);
            break;
        }
        case AST_POINTER_DEREFERENCE: {
            AccessSet base = analyzeEffects(node->pointerDeref.pointer, scope, true, false);
            accessSetMerge(&set, &base);
            accessSetFree(&base);
            break;
        }
        case AST_FUNCTION_CALL: {
            AccessSet callee = analyzeEffects(node->functionCall.callee, scope, true, false);
            AccessSet accum = callee;
            for (size_t i = 0; i < node->functionCall.argumentCount; ++i) {
                AccessSet arg = analyzeEffects(node->functionCall.arguments[i], scope, true, false);
                mergeUnsequenced(node, &accum, &arg);
                accessSetFree(&arg);
            }
            set = accum;
            break;
        }
        default:
            if (node->type == AST_CAST_EXPRESSION) {
                AccessSet inner = analyzeEffects(node->castExpr.expression, scope, true, false);
                accessSetMerge(&set, &inner);
                accessSetFree(&inner);
            } else if (node->type == AST_SIZEOF) {
                AccessSet inner = analyzeEffects(node->expr.left, scope, true, false);
                accessSetMerge(&set, &inner);
                accessSetFree(&inner);
            }
            break;
    }

    accessSetAddOwned(&set, analyzeExprAccessPath(node), asRead, asWrite);
    return set;
}

void analyzeExpressionEffects(ASTNode* node, Scope* scope) {
    static int g_effectDepth = 0;

    if (!node || g_effectDepth != 0) {
        return;
    }

    g_effectDepth++;
    AccessSet effects = analyzeEffects(node, scope, true, false);
    accessSetFree(&effects);
    g_effectDepth--;
}
