#include "analyze_expr.h"
#include "analyze_core.h"
#include "syntax_errors.h"
#include "symbol_table.h"
#include "Parser/Helpers/designated_init.h"
#include "Parser/Helpers/parsed_type.h"
#include "literal_utils.h"
#include "Compiler/compiler_context.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void analyzeDesignatedInitializerExpr(DesignatedInit* init, Scope* scope);
static bool isArithmeticOperator(const char* op);
static bool isComparisonOperator(const char* op);
static bool isLogicalOperator(const char* op);
static bool isBitwiseOperator(const char* op);
static bool literalLooksFloat(const char* text);
static TypeInfo typeFromLiteral(const char* text);
static TypeInfo typeFromStringLiteral(Scope* scope, const char* value);
static void validateStringLiteral(ASTNode* node, Scope* scope, const TypeInfo* baseType);
static void reportOperandError(ASTNode* node, const char* expectation, const char* op);
static bool typeInfoIsKnown(const TypeInfo* info);
static void reportArgumentCountError(ASTNode* call, const char* calleeName, size_t expected, size_t actual, bool tooFew);
static void reportArgumentTypeError(ASTNode* argNode, size_t index, const char* calleeName, const char* message);
static const char* fallbackFunctionName(const char* name);
static bool isExpressionNodeType(ASTNodeType type);

typedef struct {
    const char* name;
    bool read;
    bool write;
} AccessEntry;

typedef struct {
    AccessEntry* entries;
    size_t count;
    size_t capacity;
} AccessSet;

static void accessSetFree(AccessSet* set) {
    if (!set) return;
    free(set->entries);
    set->entries = NULL;
    set->count = 0;
    set->capacity = 0;
}

static void accessSetAdd(AccessSet* set, const char* name, bool asRead, bool asWrite) {
    if (!set || !name || (!asRead && !asWrite)) return;
    for (size_t i = 0; i < set->count; ++i) {
        if (set->entries[i].name == name || (set->entries[i].name && name && strcmp(set->entries[i].name, name) == 0)) {
            set->entries[i].read |= asRead;
            set->entries[i].write |= asWrite;
            return;
        }
    }
    if (set->count == set->capacity) {
        size_t newCap = set->capacity == 0 ? 8 : set->capacity * 2;
        AccessEntry* grown = realloc(set->entries, newCap * sizeof(AccessEntry));
        if (!grown) return;
        set->entries = grown;
        set->capacity = newCap;
    }
    set->entries[set->count].name = name;
    set->entries[set->count].read = asRead;
    set->entries[set->count].write = asWrite;
    set->count++;
}

static void accessSetMerge(AccessSet* dst, const AccessSet* src) {
    if (!dst || !src) return;
    for (size_t i = 0; i < src->count; ++i) {
        accessSetAdd(dst, src->entries[i].name, src->entries[i].read, src->entries[i].write);
    }
}

static void warnUnsequenced(const ASTNode* atNode, const char* name) {
    if (!name) return;
    char buf[160];
    snprintf(buf, sizeof(buf), "Unsequenced modification and access of '%s'", name);
    addWarning(atNode ? atNode->line : 0, 0, buf, NULL);
}

static void mergeUnsequenced(const ASTNode* atNode, AccessSet* accum, const AccessSet* other) {
    if (!accum || !other) return;
    for (size_t i = 0; i < other->count; ++i) {
        const AccessEntry* e = &other->entries[i];
        for (size_t j = 0; j < accum->count; ++j) {
            AccessEntry* a = &accum->entries[j];
            if ((a->name == e->name) || (a->name && e->name && strcmp(a->name, e->name) == 0)) {
                bool conflict = (a->write && (e->write || e->read)) || (e->write && (a->write || a->read));
                if (conflict) {
                    warnUnsequenced(atNode, a->name ? a->name : e->name);
                }
                break;
            }
        }
    }
    accessSetMerge(accum, other);
}

static const char* baseIdentifierName(ASTNode* node) {
    if (!node) return NULL;
    switch (node->type) {
        case AST_IDENTIFIER:
            return node->valueNode.value;
        case AST_POINTER_ACCESS:
        case AST_DOT_ACCESS:
            return baseIdentifierName(node->memberAccess.base);
        case AST_ARRAY_ACCESS:
            return baseIdentifierName(node->arrayAccess.array);
        case AST_POINTER_DEREFERENCE:
            return baseIdentifierName(node->pointerDeref.pointer);
        case AST_UNARY_EXPRESSION:
            if (node->expr.op && strcmp(node->expr.op, "&") == 0) {
                return baseIdentifierName(node->expr.left);
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
        case AST_IDENTIFIER: {
            accessSetAdd(&set, node->valueNode.value, asRead, asWrite);
            break;
        }
        case AST_NUMBER_LITERAL:
        case AST_CHAR_LITERAL:
        case AST_STRING_LITERAL:
            break;
        case AST_ASSIGNMENT: {
            bool isCompound = node->assignment.op && strcmp(node->assignment.op, "=") != 0;
            AccessSet lhs = analyzeEffects(node->assignment.target, scope, isCompound, true);
            AccessSet rhs = analyzeEffects(node->assignment.value, scope, true, false);
            // Assignment sequences the write after evaluating both operands; treat as sequenced
            // to avoid false positives on idioms like x = x + 1.
            accessSetMerge(&lhs, &rhs);
            accessSetFree(&rhs);
            set = lhs;
            break;
        }
        case AST_BINARY_EXPRESSION: {
            set = analyzeEffectsBinary(node, scope);
            break;
        }
        case AST_COMMA_EXPRESSION: {
            for (size_t i = 0; i < node->commaExpr.exprCount; ++i) {
                AccessSet part = analyzeEffects(node->commaExpr.expressions[i], scope, true, false);
                accessSetMerge(&set, &part);
                accessSetFree(&part);
            }
            break;
        }
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
            AccessSet base = analyzeEffects(node->arrayAccess.array, scope, true, false);
            AccessSet idx = analyzeEffects(node->arrayAccess.index, scope, true, false);
            mergeUnsequenced(node, &base, &idx);
            set = base;
            accessSetFree(&idx);
            break;
        }
        case AST_POINTER_ACCESS:
        case AST_DOT_ACCESS: {
            AccessSet base = analyzeEffects(node->memberAccess.base, scope, true, false);
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
            // Recurse generically over known expression children
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

    // Mark base object access if this node is being treated as a read/write lvalue directly.
    const char* baseName = baseIdentifierName(node);
    if (baseName) {
        accessSetAdd(&set, baseName, asRead, asWrite);
    }
    return set;
}

static int g_effectDepth = 0;

static bool parsedTypeIsRestrictPointer(const ParsedType* pt) {
    if (!pt) return false;
    const TypeDerivation* d = parsedTypeGetDerivation(pt, 0);
    if (!d || d->kind != TYPE_DERIVATION_POINTER) return false;
    return d->as.pointer.isRestrict;
}

static void analyzeDesignatedInitializerExpr(DesignatedInit* init, Scope* scope) {
    if (!init) return;
    if (init->indexExpr) {
        analyzeExpression(init->indexExpr, scope);
    }
    if (init->expression) {
        analyzeExpression(init->expression, scope);
    }
}

static bool isArithmeticOperator(const char* op) {
    if (!op) return false;
    return strcmp(op, "+") == 0 ||
           strcmp(op, "-") == 0 ||
           strcmp(op, "*") == 0 ||
           strcmp(op, "/") == 0 ||
           strcmp(op, "%") == 0;
}

static bool isComparisonOperator(const char* op) {
    if (!op) return false;
    return strcmp(op, "==") == 0 ||
           strcmp(op, "!=") == 0 ||
           strcmp(op, "<") == 0  ||
           strcmp(op, "<=") == 0 ||
           strcmp(op, ">") == 0  ||
           strcmp(op, ">=") == 0;
}

static bool isLogicalOperator(const char* op) {
    if (!op) return false;
    return strcmp(op, "&&") == 0 || strcmp(op, "||") == 0;
}

static bool isBitwiseOperator(const char* op) {
    if (!op) return false;
    return strcmp(op, "&") == 0 || strcmp(op, "|") == 0 ||
           strcmp(op, "^") == 0 || strcmp(op, "<<") == 0 ||
           strcmp(op, ">>") == 0;
}

static bool literalLooksFloat(const char* text) {
    if (!text) return false;
    bool isHex = text[0] == '0' && (text[1] == 'x' || text[1] == 'X');
    for (const char* p = text; *p; ++p) {
        if (*p == '.') {
            return true;
        }
        if (!isHex && (*p == 'e' || *p == 'E')) {
            return true;
        }
        if (isHex && (*p == 'p' || *p == 'P')) {
            return true;
        }
    }
    size_t len = strlen(text);
    if (len > 0) {
        char last = text[len - 1];
        if (!isHex && (last == 'f' || last == 'F')) {
            return true;
        }
    }
    return false;
}

static TypeInfo typeFromLiteral(const char* text) {
    if (literalLooksFloat(text)) {
        size_t len = text ? strlen(text) : 0;
        bool hasImag = len > 0 && (text[len - 1] == 'i' || text[len - 1] == 'I' || text[len - 1] == 'j' || text[len - 1] == 'J');
        size_t coreLen = len;
        if (hasImag && coreLen > 0) {
            coreLen--;
        }
        bool isFloat = coreLen > 0 && (text[coreLen - 1] == 'f' || text[coreLen - 1] == 'F');
        bool isLongDouble = coreLen > 0 && (text[coreLen - 1] == 'l' || text[coreLen - 1] == 'L');
        FloatKind fk = isFloat ? FLOAT_KIND_FLOAT : (isLongDouble ? FLOAT_KIND_LONG_DOUBLE : FLOAT_KIND_DOUBLE);
        TypeInfo info = makeFloatTypeInfo(fk, hasImag, NULL);
        info.isImaginary = hasImag;
        info.isLValue = false;
        return info;
    }
    TypeInfo info = makeIntegerType(32, true, TOKEN_INT);
    info.isLValue = false;
    return info;
}

static TypeInfo makeWCharType(Scope* scope) {
    TypeInfo info = makeIntegerType(32, true, TOKEN_INT);
    if (!scope) return info;
    Symbol* w = resolveInScopeChain(scope, "wchar_t");
    if (w && w->kind == SYMBOL_TYPEDEF) {
        info = typeInfoFromParsedType(&w->type, scope);
    }
    return info;
}

static TypeInfo typeFromStringLiteral(Scope* scope, const char* value) {
    const char* payload = NULL;
    LiteralEncoding enc = ast_literal_encoding(value, &payload);
    (void)payload;

    TypeInfo base = makeIntegerType(8, true, TOKEN_CHAR);
    if (enc == LIT_ENC_WIDE) {
        base = makeWCharType(scope);
    }

    TypeInfo info = makeInvalidType();
    info.category = TYPEINFO_POINTER;
    info.pointerDepth = 1;
    info.primitive = base.primitive;
    info.tag = base.tag;
    info.userTypeName = base.userTypeName;
    info.bitWidth = base.bitWidth ? base.bitWidth : 8;
    info.isSigned = base.isSigned;
    info.isConst = true;
    info.isComplete = true;
    info.isLValue = false;
    return info;
}

static void validateStringLiteral(ASTNode* node, Scope* scope, const TypeInfo* baseType) {
    if (!node || !baseType) return;
    (void)scope;
    const char* payload = NULL;
    ast_literal_encoding(node->valueNode.value, &payload);
    int bitWidth = baseType->bitWidth ? baseType->bitWidth : 8;
    LiteralDecodeResult res = decode_c_string_literal(payload ? payload : "", bitWidth, NULL, NULL);
    if (!res.ok) {
        addError(node->line, 0, "Invalid escape sequence in string literal", NULL);
    } else if (res.overflow) {
        addError(node->line, 0, "String literal contains code point not representable in target character type", NULL);
    }
}

static void reportOperandError(ASTNode* node, const char* expectation, const char* op) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Operator '%s' requires %s", op ? op : "?", expectation);
    SourceRange loc = node ? node->location : (SourceRange){0};
    SourceRange callSite = node ? node->macroCallSite : (SourceRange){0};
    SourceRange macroDef = node ? node->macroDefinition : (SourceRange){0};
    addErrorWithRanges(loc, callSite, macroDef, buffer, NULL);
}

static bool typeInfoIsKnown(const TypeInfo* info) {
    return info && info->category != TYPEINFO_INVALID;
}

static void restoreBaseCategory(TypeInfo* info) {
    if (!info || info->pointerDepth != 0) return;
    if (info->isFunction) {
        info->category = TYPEINFO_FUNCTION;
        return;
    }
    switch (info->tag) {
        case TAG_STRUCT:
            info->category = TYPEINFO_STRUCT;
            return;
        case TAG_UNION:
            info->category = TYPEINFO_UNION;
            return;
        case TAG_ENUM:
            info->category = TYPEINFO_ENUM;
            return;
        default:
            break;
    }
    if (info->primitive == TOKEN_FLOAT || info->primitive == TOKEN_DOUBLE) {
        info->category = TYPEINFO_FLOAT;
    } else if (info->primitive == TOKEN_VOID) {
        info->category = TYPEINFO_VOID;
    } else {
        info->category = TYPEINFO_INTEGER;
    }
}

static const char* fallbackFunctionName(const char* name) {
    return name ? name : "<anonymous>";
}

static bool isExpressionNodeType(ASTNodeType type) {
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

static void reportArgumentCountError(ASTNode* call, const char* calleeName, size_t expected, size_t actual, bool tooFew) {
    char buffer[160];
    snprintf(buffer,
             sizeof(buffer),
             "Too %s arguments in call to '%s' (expected %zu, got %zu)",
             tooFew ? "few" : "many",
             fallbackFunctionName(calleeName),
             expected,
             actual);
    addError(call ? call->line : 0, 0, buffer, NULL);
}

static void reportArgumentTypeError(ASTNode* argNode, size_t index, const char* calleeName, const char* message) {
    char buffer[200];
    snprintf(buffer,
             sizeof(buffer),
             "Argument %zu of '%s' %s",
             index + 1,
             fallbackFunctionName(calleeName),
             message);
    addError(argNode ? argNode->line : 0, 0, buffer, NULL);
}

static const ParsedType* lookupFieldType(const TypeInfo* base,
                                         const char* fieldName,
                                         Scope* scope) {
    if (!base || !fieldName || !scope || !scope->ctx) {
        return NULL;
    }
    if (base->category != TYPEINFO_STRUCT && base->category != TYPEINFO_UNION) {
        return NULL;
    }
    CCTagKind kind = (base->category == TYPEINFO_STRUCT) ? CC_TAG_STRUCT : CC_TAG_UNION;
    ASTNode* def = cc_tag_definition(scope->ctx, kind, base->userTypeName);
    if (!def || (def->type != AST_STRUCT_DEFINITION && def->type != AST_UNION_DEFINITION)) {
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
    }
    return NULL;
}

static const CCTagFieldLayout* lookupFieldLayout(const TypeInfo* base,
                                                 const char* fieldName,
                                                 Scope* scope) {
    if (!base || !fieldName || !scope || !scope->ctx) return NULL;
    if (base->category != TYPEINFO_STRUCT && base->category != TYPEINFO_UNION) {
        return NULL;
    }
    const CCTagFieldLayout* layouts = NULL;
    size_t count = 0;
    CCTagKind kind = (base->category == TYPEINFO_STRUCT) ? CC_TAG_STRUCT : CC_TAG_UNION;
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

TypeInfo decayToRValue(TypeInfo info) {
    if (info.isArray) {
        info.category = TYPEINFO_POINTER;
        PointerQualifier q = { info.isConst, info.isVolatile, info.isRestrict };
        typeInfoPrependPointerLevel(&info, q);
        info.isArray = false;
        info.isLValue = false;
        return info;
    }
    if (info.category == TYPEINFO_FUNCTION || info.isFunction) {
        info.category = TYPEINFO_POINTER;
        typeInfoPrependPointerLevel(&info, (PointerQualifier){0});
        info.isFunction = false;
        info.isLValue = false;
        return info;
    }
    if (!info.isLValue) {
        return info;
    }
    if (info.isBitfield) {
        info.isBitfield = false;
        info.bitfieldLayout = NULL;
    }
    info.isLValue = false;
    return info;
}

static bool isModifiableLValue(const TypeInfo* info) {
    if (!info || !info->isLValue) {
        return false;
    }
    if (info->isConst || info->isArray || info->category == TYPEINFO_FUNCTION || info->category == TYPEINFO_VOID) {
        return false;
    }
    if (info->isVLA) {
        return false;
    }
    return true;
}

TypeInfo analyzeExpression(ASTNode* node, Scope* scope) {
    if (!node) return makeInvalidType();

    if (g_effectDepth == 0) {
        g_effectDepth++;
        AccessSet effects = analyzeEffects(node, scope, true, false);
        accessSetFree(&effects);
        g_effectDepth--;
    }

    switch (node->type) {
        case AST_IDENTIFIER: {
            Symbol* sym = resolveInScopeChain(scope, node->valueNode.value);
            if (!sym) {
                addErrorWithRanges(node->location,
                                   node->macroCallSite,
                                   node->macroDefinition,
                                   "Undeclared identifier",
                                   node->valueNode.value);
                return makeInvalidType();
            }

            if (sym->kind == SYMBOL_FUNCTION) {
                TypeInfo fnInfo = makeInvalidType();
                fnInfo.category = TYPEINFO_FUNCTION;
                fnInfo.isFunction = true;
                fnInfo.isLValue = false;
                return fnInfo;
            }

            TypeInfo info = typeInfoFromParsedType(&sym->type, scope);
            info.isLValue = (sym->kind == SYMBOL_VARIABLE);
            return info;
        }

        case AST_NUMBER_LITERAL:
            return typeFromLiteral(node->valueNode.value);

        case AST_CHAR_LITERAL: {
            const char* payload = NULL;
            LiteralEncoding enc = ast_literal_encoding(node->valueNode.value, &payload);
            (void)payload;
            if (enc == LIT_ENC_WIDE) {
                return makeWCharType(scope);
            }
            return makeIntegerType(32, true, TOKEN_INT);
        }

        case AST_STRING_LITERAL: {
            const char* payload = NULL;
            LiteralEncoding enc = ast_literal_encoding(node->valueNode.value, &payload);
            (void)payload;
            TypeInfo base = makeIntegerType(8, true, TOKEN_CHAR);
            if (enc == LIT_ENC_WIDE) {
                base = makeWCharType(scope);
            }
            validateStringLiteral(node, scope, &base);
            return typeFromStringLiteral(scope, node->valueNode.value);
        }

        case AST_PARSED_TYPE: {
            return typeInfoFromParsedType(&node->parsedTypeNode.parsed, scope);
        }

        case AST_ASSIGNMENT: {
            TypeInfo targetInfo = analyzeExpression(node->assignment.target, scope);
            if (!isModifiableLValue(&targetInfo)) {
                const char* op = node->assignment.op ? node->assignment.op : "=";
                char buffer[128];
                snprintf(buffer, sizeof(buffer), "Left operand of '%s' must be a modifiable lvalue", op);
                addError(node->line, 0, buffer, NULL);
            }
            TypeInfo valueInfo = analyzeExpression(node->assignment.value, scope);
            TypeInfo rvalue = decayToRValue(valueInfo);
            AssignmentCheckResult assignResult = canAssignTypes(&targetInfo, &rvalue);
            if (assignResult == ASSIGN_QUALIFIER_LOSS) {
                addError(node->line, 0, "Assignment discards qualifiers from pointer target", NULL);
            } else if (assignResult == ASSIGN_INCOMPATIBLE) {
                addError(node->line, 0, "Incompatible assignment operands", NULL);
            }
            targetInfo.isLValue = false;
            return targetInfo;
        }

        case AST_BINARY_EXPRESSION: {
            TypeInfo left = analyzeExpression(node->expr.left, scope);
            TypeInfo right = analyzeExpression(node->expr.right, scope);
            const char* op = node->expr.op;

            left = decayToRValue(left);
            right = decayToRValue(right);

            if (isArithmeticOperator(op)) {
                bool leftPtr = typeInfoIsPointerLike(&left);
                bool rightPtr = typeInfoIsPointerLike(&right);

                if (strcmp(op, "+") == 0) {
                    if (leftPtr && typeInfoIsInteger(&right)) {
                        left.isLValue = false;
                        left.isArray = false;
                        left.category = TYPEINFO_POINTER;
                        if (left.pointerDepth == 0) left.pointerDepth = 1;
                        return left;
                    }
                    if (rightPtr && typeInfoIsInteger(&left)) {
                        right.isLValue = false;
                        right.isArray = false;
                        right.category = TYPEINFO_POINTER;
                        if (right.pointerDepth == 0) right.pointerDepth = 1;
                        return right;
                    }
                    if (leftPtr || rightPtr) {
                        reportOperandError(node, "pointer arithmetic (pointer +/- integer)", op);
                        return makeInvalidType();
                    }
                } else if (strcmp(op, "-") == 0) {
                    if (leftPtr && typeInfoIsInteger(&right)) {
                        left.isLValue = false;
                        left.isArray = false;
                        left.category = TYPEINFO_POINTER;
                        if (left.pointerDepth == 0) left.pointerDepth = 1;
                        return left;
                    }
                    if (leftPtr && rightPtr) {
                        // Allow pointer difference if both are pointer-like; target equality is relaxed.
                        TypeInfo diff = makeIntegerType(64, true, TOKEN_LONG);
                        diff.isLValue = false;
                        return diff;
                    }
                    if (rightPtr) {
                        reportOperandError(node, "pointer arithmetic (pointer +/- integer)", op);
                        return makeInvalidType();
                    }
                }

                bool ok = true;
                TypeInfo result = usualArithmeticConversion(left, right, &ok);
                if (!ok) {
                    if (typeInfoIsKnown(&left) || typeInfoIsKnown(&right)) {
                        reportOperandError(node, "arithmetic operands", op);
                    }
                    return makeInvalidType();
                }
                result.isLValue = false;
                return result;
            }

            if (isComparisonOperator(op)) {
                if (typeInfoIsPointerLike(&left) && typeInfoIsPointerLike(&right)) {
                    return makeBoolType();
                }
                // Permit pointer comparisons against integer constants/null (common in headers).
                if ((typeInfoIsPointerLike(&left) && typeInfoIsInteger(&right)) ||
                    (typeInfoIsPointerLike(&right) && typeInfoIsInteger(&left))) {
                    return makeBoolType();
                }
                bool ok = true;
                (void)usualArithmeticConversion(left, right, &ok);
                if (!ok) {
                    if (typeInfoIsKnown(&left) || typeInfoIsKnown(&right)) {
                        reportOperandError(node, "comparable operands", op);
                    }
                    return makeInvalidType();
                }
                return makeBoolType();
            }

            if (isLogicalOperator(op)) {
                if ((!typeInfoIsArithmetic(&left) && !typeInfoIsPointerLike(&left)) ||
                    (!typeInfoIsArithmetic(&right) && !typeInfoIsPointerLike(&right))) {
                    if (typeInfoIsKnown(&left) || typeInfoIsKnown(&right)) {
                        reportOperandError(node, "scalar operands", op);
                    }
                    return makeInvalidType();
                }
                return makeBoolType();
            }

            if (isBitwiseOperator(op)) {
                if (!typeInfoIsInteger(&left) || !typeInfoIsInteger(&right)) {
                    if (typeInfoIsKnown(&left) || typeInfoIsKnown(&right)) {
                        reportOperandError(node, "integer operands", op);
                    }
                    return makeInvalidType();
                }
                bool ok = true;
                TypeInfo result = usualArithmeticConversion(left, right, &ok);
                result.isLValue = false;
                return result;
            }

            return makeInvalidType();
        }

        case AST_COMMA_EXPRESSION: {
            TypeInfo last = makeInvalidType();
            for (size_t i = 0; i < node->commaExpr.exprCount; ++i) {
                last = analyzeExpression(node->commaExpr.expressions[i], scope);
            }
            return decayToRValue(last);
        }

        case AST_UNARY_EXPRESSION: {
            const char* op = node->expr.op;
            TypeInfo operand = analyzeExpression(node->expr.left, scope);

            if (!op) return operand;

            if (strcmp(op, "++") == 0 || strcmp(op, "--") == 0) {
                if (!isModifiableLValue(&operand)) {
                    reportOperandError(node, "modifiable lvalue operand", op);
                    return makeInvalidType();
                }
                operand.isLValue = !node->expr.isPostfix;
                return operand;
            }

            if (strcmp(op, "&") == 0) {
                bool isFuncDesignator = (operand.category == TYPEINFO_FUNCTION || operand.isFunction);
                if (!operand.isLValue && !isFuncDesignator) {
                    reportOperandError(node, "lvalue operand", "&");
                    return makeInvalidType();
                }
                if (operand.isBitfield) {
                    reportOperandError(node, "non-bitfield lvalue operand", "&");
                    return makeInvalidType();
                }
                ASTNode* target = node->expr.left;
                if (target && target->type == AST_IDENTIFIER) {
                    Symbol* sym = resolveInScopeChain(scope, target->valueNode.value);
                    if (sym && sym->storage == STORAGE_REGISTER) {
                        addErrorWithRanges(target->location,
                                           target->macroCallSite,
                                           target->macroDefinition,
                                           "Cannot take address of register object",
                                           target->valueNode.value);
                    }
                }
                PointerQualifier q = { operand.isConst, operand.isVolatile, operand.isRestrict };
                if (isFuncDesignator) {
                    q.isConst = false;
                    q.isVolatile = false;
                    q.isRestrict = false;
                    operand.isFunction = false;
                }
                operand.category = TYPEINFO_POINTER;
                typeInfoPrependPointerLevel(&operand, q);
                operand.isArray = false;
                operand.isLValue = false;
                return operand;
            }

            operand = decayToRValue(operand);

            if (strcmp(op, "*") == 0) {
                if (operand.category == TYPEINFO_POINTER && operand.pointerDepth > 0) {
                    if (operand.originalType) {
                        ParsedType targetParsed = parsedTypePointerTargetType(operand.originalType);
                        if (targetParsed.kind != TYPE_INVALID) {
                            TypeInfo targetInfo = typeInfoFromParsedType(&targetParsed, scope);
                            targetInfo.isLValue = true;
                            parsedTypeFree(&targetParsed);
                            return targetInfo;
                        }
                        parsedTypeFree(&targetParsed);
                    }
                    typeInfoDropPointerLevel(&operand);
                    if (operand.pointerDepth == 0) {
                        restoreBaseCategory(&operand);
                    }
                    operand.isArray = false;
                    operand.isLValue = true;
                    return operand;
                }
                if (typeInfoIsKnown(&operand)) {
                    reportOperandError(node, "pointer operand", "*");
                }
                return makeInvalidType();
            }

            if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) {
                if (!typeInfoIsArithmetic(&operand)) {
                    if (typeInfoIsKnown(&operand)) {
                        reportOperandError(node, "arithmetic operand", op);
                    }
                    return makeInvalidType();
                }
                TypeInfo promoted = integerPromote(operand);
                promoted.isLValue = false;
                return promoted;
            }

            if (strcmp(op, "!") == 0) {
                if (!typeInfoIsArithmetic(&operand) && !typeInfoIsPointerLike(&operand)) {
                    if (typeInfoIsKnown(&operand)) {
                        reportOperandError(node, "scalar operand", op);
                    }
                    return makeInvalidType();
                }
                return makeBoolType();
            }

            if (strcmp(op, "~") == 0) {
                if (!typeInfoIsInteger(&operand)) {
                    if (typeInfoIsKnown(&operand)) {
                        reportOperandError(node, "integer operand", op);
                    }
                    return makeInvalidType();
                }
                operand.isLValue = false;
                return operand;
            }

            return operand;
        }

        case AST_CAST_EXPRESSION: {
            TypeInfo target = typeInfoFromParsedType(&node->castExpr.castType, scope);
            analyzeExpression(node->castExpr.expression, scope);
            target.isLValue = false;
            return target;
        }

        case AST_FUNCTION_CALL: {
            (void)analyzeExpression(node->functionCall.callee, scope);

            size_t argCount = node->functionCall.argumentCount;
            TypeInfo* argInfos = NULL;
            if (argCount > 0) {
                argInfos = calloc(argCount, sizeof(TypeInfo));
            }
            for (size_t i = 0; i < argCount; i++) {
                ASTNode* argNode = node->functionCall.arguments ? node->functionCall.arguments[i] : NULL;
                TypeInfo argType = analyzeExpression(argNode, scope);
                if (argInfos) {
                    argInfos[i] = decayToRValue(argType);
                }
            }

            const char* calleeName = NULL;
            Symbol* sym = NULL;
            if (node->functionCall.callee &&
                node->functionCall.callee->type == AST_IDENTIFIER) {
                calleeName = node->functionCall.callee->valueNode.value;
                sym = resolveInScopeChain(scope, calleeName);
            }

            TypeInfo result = makeInvalidType();
            if (calleeName &&
                (strcmp(calleeName, "__builtin_va_arg") == 0 ||
                 strcmp(calleeName, "va_arg") == 0)) {
                if (argCount == 2 &&
                    node->functionCall.arguments[1] &&
                    node->functionCall.arguments[1]->type == AST_PARSED_TYPE) {
                    result = typeInfoFromParsedType(&node->functionCall.arguments[1]->parsedTypeNode.parsed, scope);
                    result.isLValue = false;
                    return result;
                }
            }
            if (calleeName && strcmp(calleeName, "va_start") == 0) {
                if (!scope || !scope->inFunction) {
                    addError(node->line, 0, "va_start can only appear inside a function", NULL);
                } else {
                    if (argCount < 2) {
                        addError(node->line, 0, "va_start expects at least two arguments", NULL);
                    }
                    if (!scope->currentFunctionIsVariadic) {
                        addError(node->line, 0, "va_start used in non-variadic function", NULL);
                    }
                    if (scope->currentFunctionFixedParams == 0) {
                        addError(node->line, 0, "va_start requires at least one named parameter", NULL);
                    }
                }
            }
            if (sym && sym->kind == SYMBOL_FUNCTION) {
                FunctionSignature* sig = &sym->signature;
                size_t expected = sig->paramCount;
                bool tooFew = argCount < expected;
                bool tooMany = !sig->isVariadic && argCount > expected;
                if (tooFew) {
                    reportArgumentCountError(node, calleeName, expected, argCount, true);
                }
                if (tooMany) {
                    reportArgumentCountError(node, calleeName, expected, argCount, false);
                }
                size_t pairCount = expected < argCount ? expected : argCount;
                bool* paramRestrict = pairCount ? calloc(pairCount, sizeof(bool)) : NULL;
                const char** argBases = pairCount ? calloc(pairCount, sizeof(const char*)) : NULL;
                for (size_t i = 0; i < pairCount; ++i) {
                    TypeInfo paramInfo = typeInfoFromParsedType(&sig->params[i], scope);
                    if (paramInfo.isArray) {
                        paramInfo = decayToRValue(paramInfo);
                    }
                    if (paramRestrict) {
                        paramRestrict[i] = parsedTypeIsRestrictPointer(&sig->params[i]) ||
                                           ((paramInfo.pointerDepth > 0) && paramInfo.pointerLevels[0].isRestrict);
                    }
                    if (argBases && node->functionCall.arguments) {
                        argBases[i] = baseIdentifierName(node->functionCall.arguments[i]);
                    }
                    TypeInfo argInfo = argInfos ? argInfos[i] : makeInvalidType();
                    AssignmentCheckResult check = canAssignTypes(&paramInfo, &argInfo);
                    if (check == ASSIGN_QUALIFIER_LOSS) {
                        reportArgumentTypeError(node->functionCall.arguments ? node->functionCall.arguments[i] : node,
                                                i,
                                                calleeName,
                                                "discards qualifiers from pointer target");
                    } else if (check == ASSIGN_INCOMPATIBLE) {
                        reportArgumentTypeError(node->functionCall.arguments ? node->functionCall.arguments[i] : node,
                                                i,
                                                calleeName,
                                                "has incompatible type");
                    }
                }
                if (paramRestrict && argBases) {
                    for (size_t i = 0; i < pairCount; ++i) {
                        if (!paramRestrict[i] || !argBases[i]) continue;
                        for (size_t j = i + 1; j < pairCount; ++j) {
                            if (!paramRestrict[j] || !argBases[j]) continue;
                            if (strcmp(argBases[i], argBases[j]) == 0) {
                                addWarning(node->line, 0, "Restrict parameters may alias the same object", argBases[i]);
                            }
                        }
                    }
                }
                if (sig->isVariadic && argInfos) {
                    for (size_t i = expected; i < argCount; ++i) {
                        argInfos[i] = defaultArgumentPromotion(argInfos[i]);
                    }
                }
                result = typeInfoFromParsedType(&sym->type, scope);

                free(paramRestrict);
                free(argBases);
            }

            if (argInfos) {
                free(argInfos);
            }

            if (result.category == TYPEINFO_INVALID) {
                result = makeIntegerType(32, true, TOKEN_INT);
            }
            result.isLValue = false;
            return result;
        }

        case AST_POINTER_DEREFERENCE: {
            TypeInfo base = analyzeExpression(node->pointerDeref.pointer, scope);
            if (base.category == TYPEINFO_POINTER && base.pointerDepth > 0) {
                if (base.originalType) {
                    ParsedType targetParsed = parsedTypePointerTargetType(base.originalType);
                    if (targetParsed.kind != TYPE_INVALID) {
                        TypeInfo targetInfo = typeInfoFromParsedType(&targetParsed, scope);
                        targetInfo.isLValue = true;
                        parsedTypeFree(&targetParsed);
                        return targetInfo;
                    }
                    parsedTypeFree(&targetParsed);
                }
                typeInfoDropPointerLevel(&base);
                if (base.pointerDepth == 0) {
                    restoreBaseCategory(&base);
                }
                base.isArray = false;
                base.isLValue = true;
                return base;
            }
            if (typeInfoIsKnown(&base)) {
                reportOperandError(node, "pointer operand", "*");
            }
            return makeInvalidType();
        }

        case AST_DOT_ACCESS:
        case AST_POINTER_ACCESS: {
            TypeInfo base = analyzeExpression(node->memberAccess.base, scope);
            if (node->type == AST_POINTER_ACCESS) {
                if (base.category == TYPEINFO_POINTER && base.pointerDepth > 0) {
                    typeInfoDropPointerLevel(&base);
                    if (base.pointerDepth == 0) {
                        restoreBaseCategory(&base);
                    }
                    base.isArray = false;
                    base.isLValue = true;
                } else if (typeInfoIsKnown(&base)) {
                    reportOperandError(node, "pointer operand", "->");
                    return makeInvalidType();
                }
            }

            const ParsedType* fieldType = lookupFieldType(&base, node->memberAccess.field, scope);
            if (fieldType) {
                TypeInfo fieldInfo = typeInfoFromParsedType(fieldType, scope);
                fieldInfo.isLValue = true;
                fieldInfo.isConst = base.isConst || fieldInfo.isConst;
                const CCTagFieldLayout* lay = lookupFieldLayout(&base, node->memberAccess.field, scope);
                if (lay && lay->isBitfield && !lay->isZeroWidth) {
                    fieldInfo.isBitfield = true;
                    fieldInfo.bitfieldLayout = lay;
                }
                return fieldInfo;
            }

            TypeInfo field = makeIntegerType(32, true, TOKEN_INT);
            field.isLValue = true;
            field.isConst = base.isConst;
            return field;
        }

        case AST_ARRAY_ACCESS: {
            TypeInfo arrayInfo = analyzeExpression(node->arrayAccess.array, scope);
            analyzeExpression(node->arrayAccess.index, scope);
            if (arrayInfo.isArray) {
                arrayInfo = decayToRValue(arrayInfo);
            }
            if (arrayInfo.category == TYPEINFO_POINTER && arrayInfo.pointerDepth > 0) {
                if (arrayInfo.originalType) {
                    ParsedType targetParsed = parsedTypePointerTargetType(arrayInfo.originalType);
                    if (targetParsed.kind != TYPE_INVALID) {
                        TypeInfo targetInfo = typeInfoFromParsedType(&targetParsed, scope);
                        parsedTypeFree(&targetParsed);
                        targetInfo.isLValue = true;
                        return targetInfo;
                    }
                    parsedTypeFree(&targetParsed);
                }
                typeInfoDropPointerLevel(&arrayInfo);
                if (arrayInfo.pointerDepth == 0) {
                    restoreBaseCategory(&arrayInfo);
                }
                arrayInfo.isVLA = false;
                arrayInfo.isArray = false;
                arrayInfo.isLValue = true;
                return arrayInfo;
            }
            if (typeInfoIsKnown(&arrayInfo)) {
                reportOperandError(node, "pointer or array base", "[]");
            }
            return makeInvalidType();
        }

        case AST_COMPOUND_LITERAL: {
            for (size_t i = 0; i < node->compoundLiteral.entryCount; ++i) {
                analyzeDesignatedInitializerExpr(node->compoundLiteral.entries[i], scope);
            }
            TypeInfo info = typeInfoFromParsedType(&node->compoundLiteral.literalType, scope);
            node->compoundLiteral.isStaticStorage = scope ? (scope->depth == 0) : false;
            info.isLValue = true;
            return info;
        }

        case AST_STATEMENT_EXPRESSION: {
            if (!node->statementExpr.block) {
                return makeInvalidType();
            }
            Scope* inner = createScope(scope);
            TypeInfo result = makeInvalidType();
            ASTNode* block = node->statementExpr.block;
            if (block->type == AST_BLOCK) {
                size_t count = block->block.statementCount;
                for (size_t i = 0; i < count; ++i) {
                    ASTNode* stmt = block->block.statements[i];
                    if (!stmt) continue;
                    bool last = (i + 1 == count);
                    if (last && isExpressionNodeType(stmt->type)) {
                        result = analyzeExpression(stmt, inner);
                    } else {
                        analyze(stmt, inner);
                    }
                }
            } else {
                analyze(block, inner);
            }
            destroyScope(inner);
            return result;
        }

        case AST_TERNARY_EXPRESSION: {
            analyzeExpression(node->ternaryExpr.condition, scope);
            TypeInfo trueInfo = analyzeExpression(node->ternaryExpr.trueExpr, scope);
            TypeInfo falseInfo = analyzeExpression(node->ternaryExpr.falseExpr, scope);
            bool ok = true;
            bool truePtr = typeInfoIsPointerLike(&trueInfo);
            bool falsePtr = typeInfoIsPointerLike(&falseInfo);
            bool trueNull = typeInfoIsInteger(&trueInfo) && trueInfo.bitWidth == 0;
            bool falseNull = typeInfoIsInteger(&falseInfo) && falseInfo.bitWidth == 0;

            if (typeInfoIsArithmetic(&trueInfo) && typeInfoIsArithmetic(&falseInfo)) {
                TypeInfo merged = usualArithmeticConversion(trueInfo, falseInfo, &ok);
                if (!ok) {
                    if (typeInfoIsKnown(&trueInfo) || typeInfoIsKnown(&falseInfo)) {
                        reportOperandError(node, "compatible operands", "?:");
                    }
                    return makeInvalidType();
                }
                merged.isLValue = false;
                return merged;
            }

            if (truePtr && falsePtr) {
                /* void* dominates; otherwise require compatibility. */
                if ((trueInfo.primitive == TOKEN_VOID && trueInfo.pointerDepth == falseInfo.pointerDepth) ||
                    (falseInfo.primitive == TOKEN_VOID && trueInfo.pointerDepth == falseInfo.pointerDepth)) {
                    TypeInfo v = (trueInfo.primitive == TOKEN_VOID) ? trueInfo : falseInfo;
                    v.isConst = trueInfo.isConst || falseInfo.isConst;
                    v.isVolatile = trueInfo.isVolatile || falseInfo.isVolatile;
                    v.isRestrict = trueInfo.isRestrict || falseInfo.isRestrict;
                    v.isLValue = false;
                    return v;
                }
                if (typesAreEqual(&trueInfo, &falseInfo)) {
                    trueInfo.isConst = trueInfo.isConst || falseInfo.isConst;
                    trueInfo.isVolatile = trueInfo.isVolatile || falseInfo.isVolatile;
                    trueInfo.isRestrict = trueInfo.isRestrict || falseInfo.isRestrict;
                    trueInfo.isLValue = false;
                    return trueInfo;
                }
            }

            if (truePtr && falseNull) {
                trueInfo.isLValue = false;
                return trueInfo;
            }
            if (falsePtr && trueNull) {
                falseInfo.isLValue = false;
                return falseInfo;
            }

            addError(node->line, 0, "Incompatible types in ternary expression", NULL);
            return makeInvalidType();
        }

        case AST_SIZEOF:
            if (node->expr.left) {
                TypeInfo target = analyzeExpression(node->expr.left, scope);
                if (target.category == TYPEINFO_FUNCTION || target.isFunction) {
                    addError(node->line, 0, "sizeof applied to function type", NULL);
                    return makeInvalidType();
                }
                if ((target.category == TYPEINFO_STRUCT || target.category == TYPEINFO_UNION) && !target.isComplete) {
                    addError(node->line, 0, "sizeof applied to incomplete type", NULL);
                    return makeInvalidType();
                }
            }
            return makeIntegerType(64, false, TOKEN_UNSIGNED);
        case AST_ALIGNOF:
            if (node->expr.left) {
                TypeInfo target = analyzeExpression(node->expr.left, scope);
                if ((target.category == TYPEINFO_STRUCT || target.category == TYPEINFO_UNION) && !target.isComplete) {
                    addError(node->line, 0, "alignof applied to incomplete type", NULL);
                    return makeInvalidType();
                }
            }
            return makeIntegerType(64, false, TOKEN_UNSIGNED);

        default:
            return makeInvalidType();
    }
}
