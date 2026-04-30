// SPDX-License-Identifier: Apache-2.0

#include "Extensions/Units/units_expr_semantics.h"

#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Extensions/extension_diagnostics.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/extension_units_expr_table.h"
#include "Extensions/extension_units_view.h"
#include "Extensions/Units/units_conversion.h"
#include "Syntax/scope.h"

#include <string.h>

static void walk_expr_results(ASTNode* node, CompilerContext* ctx);
static bool node_is_explicit_units_convert_call(const ASTNode* node);

static bool is_dimensionless_literal(const ASTNode* node) {
    return node && (node->type == AST_NUMBER_LITERAL || node->type == AST_CHAR_LITERAL);
}

static void record_dimensionless_literal(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !is_dimensionless_literal(node)) return;
    (void)fisics_extension_set_units_expr_result(ctx, node, fisics_dim_zero(), true);
}

static bool lookup_resolved_expr_metadata(CompilerContext* ctx,
                                          ASTNode* node,
                                          FisicsDim8* outDim,
                                          const FisicsUnitDef** outUnitDef,
                                          bool* outUnitResolved) {
    if (!ctx || !node || !outDim) return false;
    if (outUnitDef) *outUnitDef = NULL;
    if (outUnitResolved) *outUnitResolved = false;
    const FisicsUnitsExprResult* result = fisics_extension_lookup_units_expr_result(ctx, node);
    if (!result || !result->resolved) return false;
    *outDim = result->dim;
    if (outUnitDef) *outUnitDef = result->unitDef;
    if (outUnitResolved) *outUnitResolved = result->unitResolved;
    return true;
}

static bool record_identifier_result(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_IDENTIFIER) return false;
    const FisicsUnitsAnnotation* ann = fisics_extension_lookup_units_annotation_binding(ctx, node);
    if (!ann || !ann->resolved || ann->dimDuplicateCount > 1) return false;
    return fisics_extension_set_units_expr_result_with_unit(ctx,
                                                            node,
                                                            ann->dim,
                                                            true,
                                                            ann->unitResolved ? ann->unitDef : NULL,
                                                            ann->unitResolved);
}

static bool is_units_preserving_unary_op(const char* op) {
    if (!op) return false;
    return strcmp(op, "+") == 0 ||
           strcmp(op, "-") == 0 ||
           strcmp(op, "++") == 0 ||
           strcmp(op, "--") == 0;
}

static void maybe_record_unary_result(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_UNARY_EXPRESSION) return;
    if (!is_units_preserving_unary_op(node->expr.op)) return;
    FisicsDim8 operandDim = fisics_dim_zero();
    const FisicsUnitDef* operandUnit = NULL;
    bool unitResolved = false;
    if (!lookup_resolved_expr_metadata(ctx, node->expr.left, &operandDim, &operandUnit, &unitResolved)) return;
    (void)fisics_extension_set_units_expr_result_with_unit(ctx,
                                                           node,
                                                           operandDim,
                                                           true,
                                                           operandUnit,
                                                           unitResolved);
}

static void maybe_record_cast_result(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_CAST_EXPRESSION) return;
    FisicsDim8 dim = fisics_dim_zero();
    const FisicsUnitDef* unit = NULL;
    bool unitResolved = false;
    if (!lookup_resolved_expr_metadata(ctx, node->castExpr.expression, &dim, &unit, &unitResolved)) return;
    (void)fisics_extension_set_units_expr_result_with_unit(ctx, node, dim, true, unit, unitResolved);
}

static void maybe_record_comma_result(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_COMMA_EXPRESSION || node->commaExpr.exprCount == 0) return;
    ASTNode* tail = node->commaExpr.expressions
                        ? node->commaExpr.expressions[node->commaExpr.exprCount - 1]
                        : NULL;
    FisicsDim8 dim = fisics_dim_zero();
    const FisicsUnitDef* unit = NULL;
    bool unitResolved = false;
    if (!lookup_resolved_expr_metadata(ctx, tail, &dim, &unit, &unitResolved)) return;
    (void)fisics_extension_set_units_expr_result_with_unit(ctx, node, dim, true, unit, unitResolved);
}

static bool is_units_add_sub_op(const char* op) {
    return op && (strcmp(op, "+") == 0 || strcmp(op, "-") == 0);
}

static bool is_units_mul_div_op(const char* op) {
    return op && (strcmp(op, "*") == 0 || strcmp(op, "/") == 0);
}

static bool is_units_comparison_op(const char* op) {
    if (!op) return false;
    return strcmp(op, "==") == 0 ||
           strcmp(op, "!=") == 0 ||
           strcmp(op, "<") == 0 ||
           strcmp(op, "<=") == 0 ||
           strcmp(op, ">") == 0 ||
           strcmp(op, ">=") == 0;
}

static bool units_resolved_and_different(const FisicsUnitDef* leftUnit,
                                         bool leftUnitResolved,
                                         const FisicsUnitDef* rightUnit,
                                         bool rightUnitResolved) {
    return leftUnitResolved && rightUnitResolved && leftUnit && rightUnit && leftUnit != rightUnit;
}

static const char* explicit_conversion_context_for_binary(const char* op) {
    if (!op) return "expression";
    if (strcmp(op, "+") == 0) return "addition";
    if (strcmp(op, "-") == 0) return "subtraction";
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
        strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
        strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
        return "comparison";
    }
    return "expression";
}

static void maybe_report_binary_mismatch(CompilerContext* ctx,
                                         ASTNode* node,
                                         FisicsDim8 leftDim,
                                         FisicsDim8 rightDim) {
    if (!ctx || !node || node->type != AST_BINARY_EXPRESSION) return;
    const char* op = node->expr.op;
    if (!is_units_add_sub_op(op)) return;
    if (fisics_dim_equal(leftDim, rightDim)) return;

    if (strcmp(op, "+") == 0) {
        fisics_extension_diag_units_add_dim_mismatch(ctx, node, leftDim, rightDim);
    } else {
        fisics_extension_diag_units_sub_dim_mismatch(ctx, node, leftDim, rightDim);
    }
}

static void maybe_record_binary_result(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_BINARY_EXPRESSION) return;
    FisicsDim8 leftDim = fisics_dim_zero();
    FisicsDim8 rightDim = fisics_dim_zero();
    const FisicsUnitDef* leftUnit = NULL;
    const FisicsUnitDef* rightUnit = NULL;
    bool leftUnitResolved = false;
    bool rightUnitResolved = false;
    if (!lookup_resolved_expr_metadata(ctx, node->expr.left, &leftDim, &leftUnit, &leftUnitResolved) ||
        !lookup_resolved_expr_metadata(ctx, node->expr.right, &rightDim, &rightUnit, &rightUnitResolved)) {
        return;
    }

    const char* op = node->expr.op;
    if (is_units_add_sub_op(op)) {
        if (!fisics_dim_equal(leftDim, rightDim)) {
            maybe_report_binary_mismatch(ctx, node, leftDim, rightDim);
            return;
        }
        if (units_resolved_and_different(leftUnit, leftUnitResolved, rightUnit, rightUnitResolved)) {
            fisics_extension_diag_units_implicit_unit_conversion(ctx,
                                                                 node,
                                                                 explicit_conversion_context_for_binary(op),
                                                                 rightUnit,
                                                                 leftUnit);
            return;
        }
        (void)fisics_extension_set_units_expr_result_with_unit(ctx,
                                                               node,
                                                               leftDim,
                                                               true,
                                                               leftUnitResolved ? leftUnit : NULL,
                                                               leftUnitResolved && leftUnit == rightUnit);
        return;
    }

    if (is_units_mul_div_op(op)) {
        FisicsDim8 outDim = fisics_dim_zero();
        bool ok = (strcmp(op, "*") == 0)
                      ? fisics_dim_add(leftDim, rightDim, &outDim)
                      : fisics_dim_sub(leftDim, rightDim, &outDim);
        if (!ok) {
            fisics_extension_diag_units_exponent_overflow(ctx, node, op, leftDim, rightDim);
            return;
        }
        const FisicsUnitDef* outUnit = NULL;
        bool outUnitResolved = false;
        bool leftDimensionless = fisics_dim_is_dimensionless(leftDim);
        bool rightDimensionless = fisics_dim_is_dimensionless(rightDim);
        if (strcmp(op, "*") == 0) {
            if (leftDimensionless && rightUnitResolved) {
                outUnit = rightUnit;
                outUnitResolved = true;
            } else if (rightDimensionless && leftUnitResolved) {
                outUnit = leftUnit;
                outUnitResolved = true;
            }
        } else {
            if (rightDimensionless && leftUnitResolved) {
                outUnit = leftUnit;
                outUnitResolved = true;
            }
        }
        (void)fisics_extension_set_units_expr_result_with_unit(ctx, node, outDim, true, outUnit, outUnitResolved);
        return;
    }

    if (is_units_comparison_op(op)) {
        if (!fisics_dim_equal(leftDim, rightDim)) {
            fisics_extension_diag_units_compare_dim_mismatch(ctx, node, leftDim, rightDim);
            return;
        }
        if (units_resolved_and_different(leftUnit, leftUnitResolved, rightUnit, rightUnitResolved)) {
            fisics_extension_diag_units_implicit_unit_conversion(ctx,
                                                                 node,
                                                                 explicit_conversion_context_for_binary(op),
                                                                 rightUnit,
                                                                 leftUnit);
            return;
        }
        (void)fisics_extension_set_units_expr_result(ctx, node, fisics_dim_zero(), true);
    }
}

static void maybe_record_assignment_result(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_ASSIGNMENT) return;
    if (!node->assignment.op || strcmp(node->assignment.op, "=") != 0) return;
    FisicsDim8 targetDim = fisics_dim_zero();
    FisicsDim8 valueDim = fisics_dim_zero();
    const FisicsUnitDef* targetUnit = NULL;
    const FisicsUnitDef* valueUnit = NULL;
    bool targetUnitResolved = false;
    bool valueUnitResolved = false;
    if (!lookup_resolved_expr_metadata(ctx, node->assignment.target, &targetDim, &targetUnit, &targetUnitResolved) ||
        !lookup_resolved_expr_metadata(ctx, node->assignment.value, &valueDim, &valueUnit, &valueUnitResolved)) {
        return;
    }
    if (!fisics_dim_equal(targetDim, valueDim)) {
        fisics_extension_diag_units_assign_dim_mismatch(ctx, node, targetDim, valueDim);
        return;
    }
    if (units_resolved_and_different(targetUnit, targetUnitResolved, valueUnit, valueUnitResolved)) {
        fisics_extension_diag_units_implicit_unit_conversion(ctx,
                                                             node,
                                                             "assignment",
                                                             valueUnit,
                                                             targetUnit);
        return;
    }
    (void)fisics_extension_set_units_expr_result_with_unit(ctx,
                                                           node,
                                                           targetDim,
                                                           true,
                                                           targetUnitResolved ? targetUnit : NULL,
                                                           targetUnitResolved);
}

static void maybe_record_ternary_result(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_TERNARY_EXPRESSION) return;
    FisicsDim8 trueDim = fisics_dim_zero();
    FisicsDim8 falseDim = fisics_dim_zero();
    const FisicsUnitDef* trueUnit = NULL;
    const FisicsUnitDef* falseUnit = NULL;
    bool trueUnitResolved = false;
    bool falseUnitResolved = false;
    if (!lookup_resolved_expr_metadata(ctx, node->ternaryExpr.trueExpr, &trueDim, &trueUnit, &trueUnitResolved) ||
        !lookup_resolved_expr_metadata(ctx, node->ternaryExpr.falseExpr, &falseDim, &falseUnit, &falseUnitResolved)) {
        return;
    }
    if (!fisics_dim_equal(trueDim, falseDim)) {
        fisics_extension_diag_units_ternary_dim_mismatch(ctx, node, trueDim, falseDim);
        return;
    }
    if (units_resolved_and_different(trueUnit, trueUnitResolved, falseUnit, falseUnitResolved)) {
        fisics_extension_diag_units_implicit_unit_conversion(ctx,
                                                             node,
                                                             "ternary result",
                                                             falseUnit,
                                                             trueUnit);
        return;
    }
    (void)fisics_extension_set_units_expr_result_with_unit(ctx,
                                                           node,
                                                           trueDim,
                                                           true,
                                                           trueUnitResolved ? trueUnit : NULL,
                                                           trueUnitResolved && trueUnit == falseUnit);
}

static void maybe_record_decl_owned_literal(CompilerContext* ctx,
                                            ASTNode* declNode,
                                            DesignatedInit* init) {
    if (!ctx || !declNode || !init || !init->expression) return;
    if (!is_dimensionless_literal(init->expression)) return;

    const FisicsUnitsAnnotation* ann = fisics_extension_lookup_units_annotation(ctx, declNode);
    if (!ann || !ann->resolved || ann->dimDuplicateCount > 1) return;

    (void)fisics_extension_set_units_expr_result_with_unit(ctx,
                                                           init->expression,
                                                           ann->dim,
                                                           true,
                                                           ann->unitResolved ? ann->unitDef : NULL,
                                                           ann->unitResolved);
}

static const char* string_literal_payload(const ASTNode* node) {
    if (!node || node->type != AST_STRING_LITERAL) return NULL;
    const char* payload = NULL;
    (void)ast_literal_encoding(node->valueNode.value, &payload);
    return payload;
}

static bool lookup_conversion_target_unit(ASTNode* targetNode,
                                          const FisicsUnitDef** outUnit,
                                          const char** outText) {
    if (outUnit) *outUnit = NULL;
    if (outText) *outText = NULL;
    const char* payload = string_literal_payload(targetNode);
    if (!payload || payload[0] == '\0') return false;
    if (outText) *outText = payload;
    return outUnit ? fisics_unit_lookup(payload, outUnit) : true;
}

static bool node_is_explicit_units_convert_call(const ASTNode* node) {
    if (!node || node->type != AST_FUNCTION_CALL || !node->functionCall.callee) return false;
    if (node->functionCall.callee->type != AST_IDENTIFIER) return false;
    const char* name = node->functionCall.callee->valueNode.value;
    return name &&
           (strcmp(name, "fisics_convert_unit") == 0 ||
            strcmp(name, "__builtin_fisics_convert_unit") == 0);
}

static void maybe_record_units_conversion_call(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !node_is_explicit_units_convert_call(node) || node->functionCall.argumentCount != 2) return;
    ASTNode* sourceNode = node->functionCall.arguments ? node->functionCall.arguments[0] : NULL;
    ASTNode* targetNode = node->functionCall.arguments ? node->functionCall.arguments[1] : NULL;
    FisicsDim8 sourceDim = fisics_dim_zero();
    const FisicsUnitDef* sourceUnit = NULL;
    bool sourceUnitResolved = false;
    if (!lookup_resolved_expr_metadata(ctx, sourceNode, &sourceDim, &sourceUnit, &sourceUnitResolved)) {
        return;
    }

    const FisicsUnitDef* targetUnit = NULL;
    const char* targetText = NULL;
    if (!lookup_conversion_target_unit(targetNode, &targetUnit, &targetText) || !targetUnit) {
        fisics_extension_diag_units_conversion_invalid_target(ctx,
                                                              node,
                                                              targetText ? targetText : "",
                                                              "explicit conversion target must name a seeded concrete unit string");
        return;
    }
    if (!sourceUnitResolved || !sourceUnit) {
        fisics_extension_diag_units_conversion_requires_source_unit(ctx, node, targetText ? targetText : targetUnit->name);
        return;
    }

    const char* detail = NULL;
    if (!fisics_unit_can_convert(sourceUnit, targetUnit, &detail)) {
        fisics_extension_diag_units_conversion_incompatible(ctx, node, sourceUnit, targetUnit, detail);
        return;
    }
    (void)fisics_extension_set_units_expr_result_with_unit(ctx,
                                                           node,
                                                           targetUnit->dim,
                                                           true,
                                                           targetUnit,
                                                           true);
}

static void maybe_validate_decl_owned_initializer_units(CompilerContext* ctx,
                                                        ASTNode* declNode,
                                                        DesignatedInit* init) {
    if (!ctx || !declNode || !init || !init->expression) return;
    const FisicsUnitsAnnotation* ann = fisics_extension_lookup_units_annotation(ctx, declNode);
    if (!ann || !ann->resolved || ann->dimDuplicateCount > 1 || !ann->unitResolved || !ann->unitDef) return;

    FisicsDim8 initDim = fisics_dim_zero();
    const FisicsUnitDef* initUnit = NULL;
    bool initUnitResolved = false;
    if (!lookup_resolved_expr_metadata(ctx, init->expression, &initDim, &initUnit, &initUnitResolved)) return;
    if (!fisics_dim_equal(initDim, ann->dim)) return;
    if (units_resolved_and_different(ann->unitDef, true, initUnit, initUnitResolved)) {
        fisics_extension_diag_units_implicit_unit_conversion(ctx,
                                                             init->expression,
                                                             "initializer",
                                                             initUnit,
                                                             ann->unitDef);
    }
}

static void walk_designated_init(ASTNode* declNode, DesignatedInit* init, CompilerContext* ctx) {
    if (!init) return;
    walk_expr_results(init->indexExpr, ctx);
    walk_expr_results(init->expression, ctx);
    maybe_record_decl_owned_literal(ctx, declNode, init);
    maybe_validate_decl_owned_initializer_units(ctx, declNode, init);
}

static void walk_expr_results(ASTNode* node, CompilerContext* ctx) {
    if (!node) return;

    switch (node->type) {
        case AST_PROGRAM:
        case AST_BLOCK:
            for (size_t i = 0; i < node->block.statementCount; ++i) {
                walk_expr_results(node->block.statements ? node->block.statements[i] : NULL, ctx);
            }
            break;

        case AST_STATEMENT_EXPRESSION:
            walk_expr_results(node->statementExpr.block, ctx);
            break;

        case AST_VARIABLE_DECLARATION:
            for (size_t i = 0; i < node->varDecl.varCount; ++i) {
                DesignatedInit* init = node->varDecl.initializers ? node->varDecl.initializers[i] : NULL;
                walk_designated_init(node, init, ctx);
            }
            walk_expr_results(node->varDecl.arraySize, ctx);
            walk_expr_results(node->varDecl.bitFieldWidth, ctx);
            break;

        case AST_STRUCT_DEFINITION:
        case AST_UNION_DEFINITION:
            for (size_t i = 0; i < node->structDef.fieldCount; ++i) {
                walk_expr_results(node->structDef.fields ? node->structDef.fields[i] : NULL, ctx);
            }
            break;

        case AST_ENUM_DEFINITION:
            for (size_t i = 0; i < node->enumDef.memberCount; ++i) {
                walk_expr_results(node->enumDef.values ? node->enumDef.values[i] : NULL, ctx);
            }
            break;

        case AST_CONDITIONAL_DIRECTIVE:
            walk_expr_results(node->conditionalDirective.body, ctx);
            break;

        case AST_FUNCTION_DECLARATION:
            for (size_t i = 0; i < node->functionDecl.paramCount; ++i) {
                walk_expr_results(node->functionDecl.parameters ? node->functionDecl.parameters[i] : NULL, ctx);
            }
            break;

        case AST_FUNCTION_DEFINITION:
            for (size_t i = 0; i < node->functionDef.paramCount; ++i) {
                walk_expr_results(node->functionDef.parameters ? node->functionDef.parameters[i] : NULL, ctx);
            }
            walk_expr_results(node->functionDef.body, ctx);
            break;

        case AST_IF_STATEMENT:
            walk_expr_results(node->ifStmt.condition, ctx);
            walk_expr_results(node->ifStmt.thenBranch, ctx);
            walk_expr_results(node->ifStmt.elseBranch, ctx);
            break;

        case AST_FOR_LOOP:
            walk_expr_results(node->forLoop.initializer, ctx);
            walk_expr_results(node->forLoop.condition, ctx);
            walk_expr_results(node->forLoop.increment, ctx);
            walk_expr_results(node->forLoop.body, ctx);
            break;

        case AST_WHILE_LOOP:
            walk_expr_results(node->whileLoop.condition, ctx);
            walk_expr_results(node->whileLoop.body, ctx);
            break;

        case AST_SWITCH:
            walk_expr_results(node->switchStmt.condition, ctx);
            for (size_t i = 0; i < node->switchStmt.caseListSize; ++i) {
                walk_expr_results(node->switchStmt.caseList ? node->switchStmt.caseList[i] : NULL, ctx);
            }
            break;

        case AST_CASE:
            walk_expr_results(node->caseStmt.caseValue, ctx);
            for (size_t i = 0; i < node->caseStmt.caseBodySize; ++i) {
                walk_expr_results(node->caseStmt.caseBody ? node->caseStmt.caseBody[i] : NULL, ctx);
            }
            break;

        case AST_RETURN:
            walk_expr_results(node->returnStmt.returnValue, ctx);
            break;

        case AST_LABEL_DECLARATION:
            walk_expr_results(node->label.statement, ctx);
            break;

        case AST_SEQUENCE:
            walk_expr_results(node->expr.left, ctx);
            walk_expr_results(node->expr.right, ctx);
            break;

        case AST_TERNARY_EXPRESSION:
            walk_expr_results(node->ternaryExpr.condition, ctx);
            walk_expr_results(node->ternaryExpr.trueExpr, ctx);
            walk_expr_results(node->ternaryExpr.falseExpr, ctx);
            maybe_record_ternary_result(ctx, node);
            break;

        case AST_BINARY_EXPRESSION:
            walk_expr_results(node->expr.left, ctx);
            walk_expr_results(node->expr.right, ctx);
            maybe_record_binary_result(ctx, node);
            break;

        case AST_UNARY_EXPRESSION:
        case AST_SIZEOF:
        case AST_ALIGNOF:
            walk_expr_results(node->expr.left, ctx);
            walk_expr_results(node->expr.right, ctx);
            maybe_record_unary_result(ctx, node);
            break;

        case AST_ASSIGNMENT:
            walk_expr_results(node->assignment.target, ctx);
            walk_expr_results(node->assignment.value, ctx);
            maybe_record_assignment_result(ctx, node);
            break;

        case AST_COMMA_EXPRESSION:
            for (size_t i = 0; i < node->commaExpr.exprCount; ++i) {
                walk_expr_results(node->commaExpr.expressions ? node->commaExpr.expressions[i] : NULL, ctx);
            }
            maybe_record_comma_result(ctx, node);
            break;

        case AST_CAST_EXPRESSION:
            walk_expr_results(node->castExpr.expression, ctx);
            maybe_record_cast_result(ctx, node);
            break;

        case AST_ARRAY_ACCESS:
            walk_expr_results(node->arrayAccess.array, ctx);
            walk_expr_results(node->arrayAccess.index, ctx);
            break;

        case AST_POINTER_DEREFERENCE:
            walk_expr_results(node->pointerDeref.pointer, ctx);
            break;

        case AST_POINTER_ACCESS:
        case AST_DOT_ACCESS:
            walk_expr_results(node->memberAccess.base, ctx);
            break;

        case AST_STRUCT_FIELD_ACCESS:
            walk_expr_results(node->structFieldAccess.structInstance, ctx);
            break;

        case AST_FUNCTION_CALL:
            walk_expr_results(node->functionCall.callee, ctx);
            for (size_t i = 0; i < node->functionCall.argumentCount; ++i) {
                walk_expr_results(node->functionCall.arguments ? node->functionCall.arguments[i] : NULL, ctx);
            }
            maybe_record_units_conversion_call(ctx, node);
            break;

        case AST_COMPOUND_LITERAL:
            for (size_t i = 0; i < node->compoundLiteral.entryCount; ++i) {
                walk_designated_init(node,
                                     node->compoundLiteral.entries ? node->compoundLiteral.entries[i] : NULL,
                                     ctx);
            }
            break;

        case AST_NUMBER_LITERAL:
        case AST_CHAR_LITERAL:
            record_dimensionless_literal(ctx, node);
            break;

        case AST_IDENTIFIER:
            (void)record_identifier_result(ctx, node);
            break;

        default:
            break;
    }
}

void fisics_units_run_expr_semantics(ASTNode* root, Scope* globalScope) {
    if (!root || !globalScope || !globalScope->ctx) return;
    CompilerContext* ctx = globalScope->ctx;
    fisics_extension_clear_units_expr_results(ctx);
    walk_expr_results(root, ctx);
}
