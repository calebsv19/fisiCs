#include "analyze_expr.h"
#include "analyze_core.h"
#include "syntax_errors.h"
#include "symbol_table.h"
#include "Parser/Helpers/designated_init.h"
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
static TypeInfo typeFromStringLiteral(void);
static void reportOperandError(ASTNode* node, const char* expectation, const char* op);
static bool typeInfoIsKnown(const TypeInfo* info);
static void reportArgumentCountError(ASTNode* call, const char* calleeName, size_t expected, size_t actual, bool tooFew);
static void reportArgumentTypeError(ASTNode* argNode, size_t index, const char* calleeName, const char* message);
static const char* fallbackFunctionName(const char* name);
static bool isExpressionNodeType(ASTNodeType type);

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
    for (const char* p = text; *p; ++p) {
        if (*p == '.' || *p == 'e' || *p == 'E') {
            return true;
        }
    }
    size_t len = strlen(text);
    if (len > 0) {
        char last = text[len - 1];
        if (last == 'f' || last == 'F') {
            return true;
        }
    }
    return false;
}

static TypeInfo typeFromLiteral(const char* text) {
    if (literalLooksFloat(text)) {
        size_t len = text ? strlen(text) : 0;
        bool isFloat = len > 0 && (text[len - 1] == 'f' || text[len - 1] == 'F');
        TypeInfo info = makeFloatTypeInfo(!isFloat);
        info.isLValue = false;
        return info;
    }
    TypeInfo info = makeIntegerType(32, true, TOKEN_INT);
    info.isLValue = false;
    return info;
}

static TypeInfo typeFromStringLiteral(void) {
    TypeInfo info = makeInvalidType();
    info.category = TYPEINFO_POINTER;
    info.pointerDepth = 1;
    info.primitive = TOKEN_CHAR;
    info.isConst = true;
    info.bitWidth = 64;
    info.isLValue = false;
    return info;
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

static TypeInfo decayToRValue(TypeInfo info) {
    if (!info.isLValue) {
        return info;
    }
    if (info.isArray) {
        info.category = TYPEINFO_POINTER;
        info.pointerDepth += 1;
        info.isArray = false;
    } else if (info.category == TYPEINFO_FUNCTION || info.isFunction) {
        info.category = TYPEINFO_POINTER;
        info.pointerDepth += 1;
        info.isFunction = false;
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

        case AST_CHAR_LITERAL:
            return makeIntegerType(32, true, TOKEN_INT);

        case AST_STRING_LITERAL:
            return typeFromStringLiteral();

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
                        return left;
                    }
                    if (rightPtr && typeInfoIsInteger(&left)) {
                        right.isLValue = false;
                        right.isArray = false;
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
                        return left;
                    }
                    if (leftPtr && rightPtr) {
                        if (!typesAreEqual(&left, &right)) {
                            reportOperandError(node, "pointers to the same element type", op);
                            return makeInvalidType();
                        }
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
                if (!operand.isLValue) {
                    reportOperandError(node, "lvalue operand", "&");
                    return makeInvalidType();
                }
                operand.category = TYPEINFO_POINTER;
                operand.pointerDepth += 1;
                operand.isArray = false;
                operand.isLValue = false;
                return operand;
            }

            operand = decayToRValue(operand);

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
                if (!typeInfoIsArithmetic(&operand)) {
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
                for (size_t i = 0; i < pairCount; ++i) {
                    TypeInfo paramInfo = typeInfoFromParsedType(&sig->params[i], scope);
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
                if (sig->isVariadic && argInfos) {
                    for (size_t i = expected; i < argCount; ++i) {
                        argInfos[i] = defaultArgumentPromotion(argInfos[i]);
                    }
                }
                result = typeInfoFromParsedType(&sym->type, scope);
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
                base.pointerDepth -= 1;
                if (base.pointerDepth == 0) {
                    base.category = TYPEINFO_INVALID;
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
            (void)base;
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
                arrayInfo.pointerDepth -= 1;
                if (arrayInfo.pointerDepth == 0) {
                    arrayInfo.category = TYPEINFO_INVALID;
                }
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

        case AST_SIZEOF:
            return makeIntegerType(64, false, TOKEN_UNSIGNED);

        default:
            return makeInvalidType();
    }
}
