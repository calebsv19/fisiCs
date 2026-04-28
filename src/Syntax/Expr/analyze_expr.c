// SPDX-License-Identifier: Apache-2.0

#include "analyze_expr.h"
#include "analyze_expr_internal.h"
#include "analyze_core.h"
#include "syntax_errors.h"
#include "symbol_table.h"
#include "Parser/Helpers/parsed_type.h"
#include "const_eval.h"
#include "literal_utils.h"
#include "Compiler/compiler_context.h"
#include "Syntax/target_layout.h"
#include "Utils/profiler.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static bool isArithmeticOperator(const char* op);
static bool isComparisonOperator(const char* op);
static bool isRelationalComparisonOperator(const char* op);
static bool isLogicalOperator(const char* op);

static bool typeInfoIsComplexLike(const TypeInfo* info) {
    return info && typeInfoIsFloating(info) && (info->isComplex || info->isImaginary);
}

static bool isBuiltinLiteralConstName(const char* name) {
    if (!name) return false;
    return strcmp(name, "true") == 0 || strcmp(name, "false") == 0;
}

static bool buildFunctionDesignatorType(const Symbol* sym, ParsedType* outType) {
    if (!sym || !outType || sym->kind != SYMBOL_FUNCTION) return false;
    *outType = parsedTypeClone(&sym->type);
    if (outType->kind == TYPE_INVALID) return false;
    if (!parsedTypeAppendFunction(outType, sym->signature.params, sym->signature.paramCount, sym->signature.isVariadic)) {
        parsedTypeFree(outType);
        memset(outType, 0, sizeof(*outType));
        outType->kind = TYPE_INVALID;
        return false;
    }
    if (outType->derivationCount > 1) {
        size_t last = outType->derivationCount - 1;
        TypeDerivation fnDeriv = outType->derivations[last];
        memmove(outType->derivations + 1, outType->derivations, sizeof(TypeDerivation) * last);
        outType->derivations[0] = fnDeriv;
    }
    return true;
}

static bool isBitwiseOperator(const char* op);
static bool literalLooksFloat(const char* text);
static unsigned scope_int_bits(Scope* scope);
static TypeInfo typeFromLiteral(const char* text, Scope* scope, bool* outOverflow);
static TypeInfo typeFromStringLiteral(Scope* scope, const char* value);
static void validateStringLiteral(ASTNode* node, Scope* scope, const TypeInfo* baseType);

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

static bool isRelationalComparisonOperator(const char* op) {
    if (!op) return false;
    return strcmp(op, "<") == 0  ||
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

static unsigned scope_int_bits(Scope* scope) {
    const TargetLayout* tl = (scope && scope->ctx) ? cc_get_target_layout(scope->ctx) : NULL;
    if (tl && tl->intBits) return (unsigned)tl->intBits;
    return 32;
}

static TypeInfo typeFromLiteral(const char* text, Scope* scope, bool* outOverflow) {
    if (outOverflow) *outOverflow = false;
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
    IntegerLiteralInfo lit;
    if (parse_integer_literal_info(text, (scope && scope->ctx) ? cc_get_target_layout(scope->ctx) : NULL, &lit) && lit.ok) {
        if (outOverflow) *outOverflow = lit.overflow;
        unsigned intBits = scope_int_bits(scope);
        TokenType prim = (lit.bits > intBits) ? TOKEN_LONG : TOKEN_INT;
        TypeInfo info = makeIntegerType(lit.bits, !lit.isUnsigned, prim);
        info.isLValue = false;
        return info;
    }
    TypeInfo info = makeIntegerType(scope_int_bits(scope), true, TOKEN_INT);
    info.isLValue = false;
    return info;
}

static TypeInfo makeWCharType(Scope* scope) {
    TypeInfo info = makeIntegerType(32, true, TOKEN_INT);
    if (!scope) return info;
    Symbol* w = resolveInScopeChain(scope, "wchar_t");
    if (w && w->kind == SYMBOL_TYPEDEF) {
        profiler_record_value("semantic_count_type_info_site_symbol", 1);
        info = typeInfoFromSymbolCached(w, scope);
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

    ParsedType owned;
    memset(&owned, 0, sizeof(owned));
    if (enc == LIT_ENC_WIDE) {
        owned.kind = TYPE_PRIMITIVE;
        owned.primitiveType = TOKEN_INT;
        owned.isConst = true;
    } else {
        owned.kind = TYPE_PRIMITIVE;
        owned.primitiveType = TOKEN_CHAR;
        owned.isConst = true;
    }
    if (parsedTypePrependPointer(&owned)) {
        typeInfoAdoptParsedType(&info, &owned);
    } else {
        parsedTypeFree(&owned);
    }
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

TypeInfo decayToRValue(TypeInfo info) {
    if (info.isArray) {
        info.category = TYPEINFO_POINTER;
        // C array-to-pointer decay yields "pointer to element type"; qualifiers
        // from the array object itself do not become qualifiers on the new pointer.
        typeInfoPrependPointerLevel(&info, (PointerQualifier){0});
        info.isArray = false;
        info.isLValue = false;
        return info;
    }
    if (info.category == TYPEINFO_FUNCTION || info.isFunction) {
        info.category = TYPEINFO_POINTER;
        typeInfoPrependPointerLevel(&info, (PointerQualifier){0});
        if (info.originalType) {
            ParsedType ptrType = parsedTypeClone(info.originalType);
            if (ptrType.kind != TYPE_INVALID && parsedTypePrependPointer(&ptrType)) {
                ptrType.isFunctionPointer = true;
                typeInfoAdoptParsedType(&info, &ptrType);
            } else {
                parsedTypeFree(&ptrType);
                info.originalType = NULL;
            }
        }
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
    bool isConstObject = info->isConst;
    if (info->originalType) {
        isConstObject = parsedTypeTopLevelConst(info->originalType);
    }
    if (isConstObject || info->isArray || info->category == TYPEINFO_FUNCTION || info->category == TYPEINFO_VOID) {
        return false;
    }
    return true;
}

TypeInfo analyzeExpression(ASTNode* node, Scope* scope) {
    if (!node) return makeInvalidType();
    profiler_record_value("semantic_count_analyze_expression", 1);

    analyzeExpressionEffects(node, scope);

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
                ParsedType fnParsed;
                memset(&fnParsed, 0, sizeof(fnParsed));
                fnParsed.kind = TYPE_INVALID;
                if (buildFunctionDesignatorType(sym, &fnParsed)) {
                    typeInfoAdoptParsedType(&fnInfo, &fnParsed);
                    parsedTypeFree(&fnParsed);
                } else {
                    fnInfo.originalType = &sym->type;
                }
                return fnInfo;
            }

            profiler_record_value("semantic_count_type_info_site_symbol", 1);
            TypeInfo info = typeInfoFromSymbolCached(sym, scope);
            info.isLValue = (sym->kind == SYMBOL_VARIABLE) &&
                            !isBuiltinLiteralConstName(sym->name);
            return info;
        }

        case AST_NUMBER_LITERAL: {
            bool overflow = false;
            TypeInfo info = typeFromLiteral(node->valueNode.value, scope, &overflow);
            if (overflow) {
                addError(node->line, 0, "Integer literal is too large for its type", NULL);
            }
            return info;
        }

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
            profiler_record_value("semantic_count_type_info_site_temp", 1);
            return typeInfoFromParsedType(&node->parsedTypeNode.parsed, scope);
        }

        case AST_ASSIGNMENT: {
            TypeInfo targetInfo = analyzeExpression(node->assignment.target, scope);
            if (!isModifiableLValue(&targetInfo)) {
                const char* op = node->assignment.op ? node->assignment.op : "=";
                char buffer[128];
                snprintf(buffer, sizeof(buffer), "Left operand of '%s' must be a modifiable lvalue", op);
                reportNodeError(node, buffer, NULL);
            }
            TypeInfo valueInfo = analyzeExpression(node->assignment.value, scope);
            TypeInfo rvalue = decayToRValue(valueInfo);
            const char* op = node->assignment.op ? node->assignment.op : "=";
            AssignmentCheckResult assignResult = ASSIGN_OK;
            bool suppressIncompatibleDiag = false;
            if (strcmp(op, "=") == 0) {
                assignResult = canAssignTypesInScope(&targetInfo, &rvalue, scope);
                if (assignResult == ASSIGN_INCOMPATIBLE &&
                    typeInfoIsPointerLike(&targetInfo) &&
                    typeInfoIsInteger(&rvalue)) {
                    long long zero = 1;
                    if (constEvalInteger(node->assignment.value, scope, &zero, true) && zero == 0) {
                        assignResult = ASSIGN_OK;
                    }
                }
            } else if ((strcmp(op, "+=") == 0 || strcmp(op, "-=") == 0) &&
                       typeInfoIsPointerLike(&targetInfo) &&
                       typeInfoIsInteger(&rvalue)) {
                if (targetInfo.primitive == TOKEN_VOID) {
                    assignResult = ASSIGN_INCOMPATIBLE;
                } else {
                    assignResult = ASSIGN_OK;
                }
            } else {
                /*
                 * For compound assignments, require the binary operation to be
                 * well-typed first, then ensure the result can be assigned back.
                 */
                ASTNode synthetic = {0};
                synthetic.type = AST_BINARY_EXPRESSION;
                synthetic.line = node->line;
                if (strcmp(op, "+=") == 0) synthetic.expr.op = "+";
                else if (strcmp(op, "-=") == 0) synthetic.expr.op = "-";
                else if (strcmp(op, "*=") == 0) synthetic.expr.op = "*";
                else if (strcmp(op, "/=") == 0) synthetic.expr.op = "/";
                else if (strcmp(op, "%=") == 0) synthetic.expr.op = "%";
                else if (strcmp(op, "<<=") == 0) synthetic.expr.op = "<<";
                else if (strcmp(op, ">>=") == 0) synthetic.expr.op = ">>";
                else if (strcmp(op, "&=") == 0) synthetic.expr.op = "&";
                else if (strcmp(op, "^=") == 0) synthetic.expr.op = "^";
                else if (strcmp(op, "|=") == 0) synthetic.expr.op = "|";
                if (synthetic.expr.op) {
                    synthetic.expr.left = node->assignment.target;
                    synthetic.expr.right = node->assignment.value;
                    size_t errorsBeforeSynthetic = getErrorCount();
                    TypeInfo combined = analyzeExpression(&synthetic, scope);
                    size_t errorsAfterSynthetic = getErrorCount();
                    TypeInfo combinedRValue = decayToRValue(combined);
                    assignResult = canAssignTypesInScope(&targetInfo, &combinedRValue, scope);
                    if (errorsAfterSynthetic > errorsBeforeSynthetic &&
                        assignResult == ASSIGN_INCOMPATIBLE) {
                        suppressIncompatibleDiag = true;
                    }
                } else {
                    assignResult = canAssignTypesInScope(&targetInfo, &rvalue, scope);
                }
            }
            if (assignResult == ASSIGN_QUALIFIER_LOSS) {
                reportNodeError(node, "Assignment discards qualifiers from pointer target", NULL);
            } else if (assignResult == ASSIGN_INCOMPATIBLE && !suppressIncompatibleDiag) {
                reportNodeError(node, "Incompatible assignment operands", NULL);
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

                if (strcmp(op, "%") == 0) {
                    if (!typeInfoIsInteger(&left) || !typeInfoIsInteger(&right)) {
                        if (typeInfoIsKnown(&left) || typeInfoIsKnown(&right)) {
                            reportOperandError(node, "integer operands", op);
                        }
                        return makeInvalidType();
                    }
                    bool ok = true;
                    TypeInfo result = usualArithmeticConversion(left, right, &ok);
                    if (!ok) {
                        if (typeInfoIsKnown(&left) || typeInfoIsKnown(&right)) {
                            reportOperandError(node, "integer operands", op);
                        }
                        return makeInvalidType();
                    }
                    result.isLValue = false;
                    return result;
                }

                if (strcmp(op, "+") == 0) {
                    if (leftPtr && typeInfoIsInteger(&right)) {
                        if (left.primitive == TOKEN_VOID) {
                            reportOperandError(node, "pointer arithmetic on void*", op);
                            return makeInvalidType();
                        }
                        left.isLValue = false;
                        left.isArray = false;
                        left.category = TYPEINFO_POINTER;
                        if (left.pointerDepth == 0) left.pointerDepth = 1;
                        return left;
                    }
                    if (rightPtr && typeInfoIsInteger(&left)) {
                        if (right.primitive == TOKEN_VOID) {
                            reportOperandError(node, "pointer arithmetic on void*", op);
                            return makeInvalidType();
                        }
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
                        if (left.primitive == TOKEN_VOID) {
                            reportOperandError(node, "pointer arithmetic on void*", op);
                            return makeInvalidType();
                        }
                        left.isLValue = false;
                        left.isArray = false;
                        left.category = TYPEINFO_POINTER;
                        if (left.pointerDepth == 0) left.pointerDepth = 1;
                        return left;
                    }
                    if (leftPtr && rightPtr) {
                        if (left.primitive == TOKEN_VOID || right.primitive == TOKEN_VOID) {
                            reportOperandError(node, "pointer arithmetic on void*", op);
                            return makeInvalidType();
                        }
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
                if (isRelationalComparisonOperator(op) &&
                    (typeInfoIsComplexLike(&left) || typeInfoIsComplexLike(&right))) {
                    reportOperandError(node, "real (non-complex) comparable operands", op);
                    return makeInvalidType();
                }
                if (typeInfoIsPointerLike(&left) && typeInfoIsPointerLike(&right)) {
                    return makeBoolType();
                }
                // Permit pointer comparisons against null pointer constants; warn otherwise.
                if (typeInfoIsPointerLike(&left) && typeInfoIsInteger(&right)) {
                    if (!isNullPointerConstant(node->expr.right, scope)) {
                        addWarning(node->line, 0, "Pointer comparison against non-null integer", NULL);
                    }
                    return makeBoolType();
                }
                if (typeInfoIsPointerLike(&right) && typeInfoIsInteger(&left)) {
                    if (!isNullPointerConstant(node->expr.left, scope)) {
                        addWarning(node->line, 0, "Pointer comparison against non-null integer", NULL);
                    }
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
                if (strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) {
                    long long shiftAmount = 0;
                    if (constEvalInteger(node->expr.right, scope, &shiftAmount, true)) {
                        TypeInfo promotedLeft = integerPromote(left);
                        unsigned lhsBits = promotedLeft.bitWidth ? promotedLeft.bitWidth : scope_int_bits(scope);
                        if (shiftAmount < 0 || shiftAmount >= (long long)lhsBits) {
                            char message[128];
                            snprintf(message,
                                     sizeof(message),
                                     "Invalid shift width %lld for %u-bit operand",
                                     shiftAmount,
                                     lhsBits);
                            reportNodeError(node, message, NULL);
                            return makeInvalidType();
                        }
                    }
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
                if (operand.originalType) {
                    ParsedType ptrType = parsedTypeClone(operand.originalType);
                    if (ptrType.kind != TYPE_INVALID && parsedTypePrependPointer(&ptrType)) {
                        if (isFuncDesignator) {
                            ptrType.isFunctionPointer = true;
                        }
                        profiler_record_value("semantic_count_type_info_temp_address_of", 1);
                        TypeInfo addrInfo = typeInfoFromParsedType(&ptrType, scope);
                        typeInfoAdoptParsedType(&addrInfo, &ptrType);
                        addrInfo.isArray = false;
                        addrInfo.isFunction = false;
                        addrInfo.isLValue = false;
                        return addrInfo;
                    }
                    parsedTypeFree(&ptrType);
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
                    if (getenv("DEBUG_DEREF")) {
                        fprintf(stderr,
                                "[deref] cat=%d ptrDepth=%d orig=%p\n",
                                operand.category,
                                operand.pointerDepth,
                                (void*)operand.originalType);
                        if (operand.originalType) {
                            fprintf(stderr, "[deref] derivations=");
                            for (size_t i = 0; i < operand.originalType->derivationCount; ++i) {
                                const TypeDerivation* d = parsedTypeGetDerivation(operand.originalType, i);
                                const char* k = d ? (d->kind == TYPE_DERIVATION_POINTER ? "ptr" :
                                                     d->kind == TYPE_DERIVATION_ARRAY ? "arr" : "fn") : "?";
                                fprintf(stderr, "%s%s", i ? "," : "", k);
                            }
                            fprintf(stderr, "\n");
                        }
                    }
                    if (operand.originalType) {
                        ParsedType targetParsed = parsedTypeIsDirectArray(operand.originalType)
                            ? parsedTypeArrayElementType(operand.originalType)
                            : parsedTypePointerTargetType(operand.originalType);
                        if (getenv("DEBUG_DEREF")) {
                            fprintf(stderr,
                                    "[deref] target kind=%d derivations=%zu\n",
                                    targetParsed.kind,
                                    targetParsed.derivationCount);
                        }
                        if (targetParsed.kind != TYPE_INVALID) {
                            profiler_record_value("semantic_count_type_info_site_temp", 1);
                            TypeInfo targetInfo = typeInfoFromParsedType(&targetParsed, scope);
                            ParsedType* owned = malloc(sizeof(ParsedType));
                            if (owned) {
                                *owned = targetParsed;
                                targetInfo.originalType = owned;
                            } else {
                                targetInfo.originalType = NULL;
                                parsedTypeFree(&targetParsed);
                                targetInfo.isLValue = true;
                                return targetInfo;
                            }
                            targetInfo.isLValue = true;
                            if (!owned) {
                                parsedTypeFree(&targetParsed);
                            }
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
            profiler_record_value("semantic_count_type_info_temp_cast_expr", 1);
            TypeInfo target = typeInfoFromParsedType(&node->castExpr.castType, scope);
            TypeInfo source = analyzeExpression(node->castExpr.expression, scope);
            source = decayToRValue(source);
            bool targetKnown = typeInfoIsKnown(&target);
            bool sourceKnown = typeInfoIsKnown(&source);
            bool targetIsVoid = target.category == TYPEINFO_VOID;
            bool targetIsScalar = typeInfoIsArithmetic(&target) || typeInfoIsPointerLike(&target);
            bool sourceIsScalar = typeInfoIsArithmetic(&source) || typeInfoIsPointerLike(&source);
            if (!targetIsVoid && targetKnown && sourceKnown && (!targetIsScalar || !sourceIsScalar)) {
                addError(node->line, 0, "Invalid cast between non-scalar types", NULL);
            }
            target.isLValue = false;
            return target;
        }

        case AST_FUNCTION_CALL:
            return analyzeFunctionCallExpression(node, scope);

        case AST_POINTER_DEREFERENCE:
            return analyzePointerDereferenceExpression(node, scope);

        case AST_DOT_ACCESS:
        case AST_POINTER_ACCESS:
            return analyzeMemberAccessExpression(node, scope);

        case AST_ARRAY_ACCESS:
            return analyzeArrayAccessExpression(node, scope);

        case AST_COMPOUND_LITERAL:
            return analyzeCompoundLiteralExpression(node, scope);

        case AST_STATEMENT_EXPRESSION:
            return analyzeStatementExpressionValue(node, scope);

        case AST_TERNARY_EXPRESSION:
            return analyzeTernaryExpression(node, scope);

        case AST_SIZEOF:
            if (node->expr.left) {
                TypeInfo target = analyzeExpression(node->expr.left, scope);
                if (target.category == TYPEINFO_VOID && target.pointerDepth == 0 && !target.isFunction) {
                    addError(node->line, 0, "sizeof applied to void type", NULL);
                    return makeInvalidType();
                }
                if (target.category == TYPEINFO_FUNCTION || target.isFunction) {
                    addError(node->line, 0, "sizeof applied to function type", NULL);
                    return makeInvalidType();
                }
                if (target.isBitfield) {
                    addError(node->line, 0, "sizeof applied to bitfield", NULL);
                    return makeInvalidType();
                }
                if (node->expr.left->type == AST_DOT_ACCESS ||
                    node->expr.left->type == AST_POINTER_ACCESS) {
                    ASTNode* baseNode = node->expr.left->memberAccess.base;
                    TypeInfo baseInfo = analyzeExpression(baseNode, scope);
                    if (node->expr.left->type == AST_POINTER_ACCESS) {
                        if (baseInfo.category == TYPEINFO_POINTER && baseInfo.pointerDepth > 0) {
                            typeInfoDropPointerLevel(&baseInfo);
                            if (baseInfo.pointerDepth == 0) {
                                restoreBaseCategory(&baseInfo);
                            }
                        }
                    }
                    if (analyzeExprLookupFieldIsBitfield(&baseInfo, node->expr.left->memberAccess.field, scope)) {
                        addError(node->line, 0, "sizeof applied to bitfield", NULL);
                        return makeInvalidType();
                    }
                }
                if ((target.category == TYPEINFO_STRUCT || target.category == TYPEINFO_UNION) && !target.isComplete) {
                    addError(node->line, 0, "sizeof applied to incomplete type", NULL);
                    return makeInvalidType();
                }
            }
            return makeIntegerType(64, false, TOKEN_UNSIGNED);
        case AST_ALIGNOF:
            if (node->expr.left) {
                if (node->expr.left->type != AST_PARSED_TYPE) {
                    reportNodeError(node, "alignof requires type-name operand", NULL);
                    return makeInvalidType();
                }
                TypeInfo target = analyzeExpression(node->expr.left, scope);
                if (target.category == TYPEINFO_VOID && target.pointerDepth == 0 && !target.isFunction) {
                    reportNodeError(node, "alignof applied to void type", NULL);
                    return makeInvalidType();
                }
                if ((target.category == TYPEINFO_STRUCT || target.category == TYPEINFO_UNION) && !target.isComplete) {
                    reportNodeError(node, "alignof applied to incomplete type", NULL);
                    return makeInvalidType();
                }
            }
            return makeIntegerType(64, false, TOKEN_UNSIGNED);

        default:
            return makeInvalidType();
    }
}
