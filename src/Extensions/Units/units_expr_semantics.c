// SPDX-License-Identifier: Apache-2.0

#include "Extensions/Units/units_expr_semantics.h"

#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Extensions/extension_diagnostics.h"
#include "Extensions/extension_units_call_contracts.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/extension_units_expr_table.h"
#include "Extensions/extension_units_view.h"
#include "Extensions/Units/units_conversion.h"
#include "Syntax/scope.h"
#include "Syntax/type_checker.h"

#include <string.h>

static void walk_expr_results(ASTNode* node, CompilerContext* ctx);
static bool node_is_explicit_units_convert_call(const ASTNode* node);
static void maybe_record_owner_literal(CompilerContext* ctx,
                                       ASTNode* ownerNode,
                                       ASTNode* expression);
static bool type_info_is_record(const TypeInfo* info);
static ASTNode* resolve_units_record_definition(const ParsedType* parsedType,
                                                const TypeInfo* info,
                                                CompilerContext* ctx);
static bool lookup_units_record_field(ASTNode* recordDef,
                                      const char* fieldName,
                                      size_t positionalIndex,
                                      ASTNode** outFieldDecl,
                                      const ParsedType** outFieldType);
static Scope* s_units_root_scope = NULL;

typedef struct UnitsPointerAlias {
    const char* name;
    const FisicsUnitsAnnotation* annotation;
} UnitsPointerAlias;

#define UNITS_POINTER_ALIAS_MAX 256
static UnitsPointerAlias s_units_pointer_aliases[UNITS_POINTER_ALIAS_MAX];
static size_t s_units_pointer_alias_count = 0;

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

static const char* compound_assignment_base_op(const char* op) {
    if (!op) return NULL;
    if (strcmp(op, "+=") == 0) return "+";
    if (strcmp(op, "-=") == 0) return "-";
    if (strcmp(op, "*=") == 0) return "*";
    if (strcmp(op, "/=") == 0) return "/";
    return NULL;
}

static bool units_resolved_and_different(const FisicsUnitDef* leftUnit,
                                         bool leftUnitResolved,
                                         const FisicsUnitDef* rightUnit,
                                         bool rightUnitResolved) {
    return leftUnitResolved && rightUnitResolved && leftUnit && rightUnit && leftUnit != rightUnit;
}

static Symbol* resolve_named_callee_symbol(const ASTNode* callee) {
    if (!s_units_root_scope || !callee || callee->type != AST_IDENTIFIER || !callee->valueNode.value) {
        return NULL;
    }
    return resolveInScopeChain(s_units_root_scope, callee->valueNode.value);
}

static ASTNode* function_param_decl_at(const Symbol* sym, size_t index) {
    if (!sym || !sym->definition) return NULL;
    ASTNode* def = sym->definition;
    ASTNode** params = NULL;
    size_t paramCount = 0;
    if (def->type == AST_FUNCTION_DEFINITION) {
        params = def->functionDef.parameters;
        paramCount = def->functionDef.paramCount;
    } else if (def->type == AST_FUNCTION_DECLARATION) {
        params = def->functionDecl.parameters;
        paramCount = def->functionDecl.paramCount;
    }
    if (!params || index >= paramCount) return NULL;
    return params[index];
}

static void collect_function_return_units(ASTNode* node,
                                          CompilerContext* ctx,
                                          bool* found,
                                          bool* conflict,
                                          FisicsDim8* dim,
                                          const FisicsUnitDef** unit,
                                          bool* unitResolved) {
    if (!node || !ctx || !found || !conflict || !dim || !unit || !unitResolved || *conflict) return;

    switch (node->type) {
        case AST_BLOCK:
        case AST_PROGRAM:
            for (size_t i = 0; i < node->block.statementCount; ++i) {
                collect_function_return_units(node->block.statements ? node->block.statements[i] : NULL,
                                              ctx,
                                              found,
                                              conflict,
                                              dim,
                                              unit,
                                              unitResolved);
            }
            break;

        case AST_RETURN: {
            FisicsDim8 returnDim = fisics_dim_zero();
            const FisicsUnitDef* returnUnit = NULL;
            bool returnUnitResolved = false;
            if (!lookup_resolved_expr_metadata(ctx,
                                               node->returnStmt.returnValue,
                                               &returnDim,
                                               &returnUnit,
                                               &returnUnitResolved)) {
                break;
            }
            if (!*found) {
                *found = true;
                *dim = returnDim;
                *unit = returnUnit;
                *unitResolved = returnUnitResolved;
                break;
            }
            if (!fisics_dim_equal(*dim, returnDim) ||
                *unitResolved != returnUnitResolved ||
                (*unitResolved && returnUnitResolved && *unit != returnUnit)) {
                *conflict = true;
            }
            break;
        }

        case AST_IF_STATEMENT:
            collect_function_return_units(node->ifStmt.thenBranch, ctx, found, conflict, dim, unit, unitResolved);
            collect_function_return_units(node->ifStmt.elseBranch, ctx, found, conflict, dim, unit, unitResolved);
            break;

        case AST_FOR_LOOP:
            collect_function_return_units(node->forLoop.body, ctx, found, conflict, dim, unit, unitResolved);
            break;

        case AST_WHILE_LOOP:
            collect_function_return_units(node->whileLoop.body, ctx, found, conflict, dim, unit, unitResolved);
            break;

        case AST_SWITCH:
            for (size_t i = 0; i < node->switchStmt.caseListSize; ++i) {
                collect_function_return_units(node->switchStmt.caseList ? node->switchStmt.caseList[i] : NULL,
                                              ctx,
                                              found,
                                              conflict,
                                              dim,
                                              unit,
                                              unitResolved);
            }
            break;

        case AST_CASE:
            for (size_t i = 0; i < node->caseStmt.caseBodySize; ++i) {
                collect_function_return_units(node->caseStmt.caseBody ? node->caseStmt.caseBody[i] : NULL,
                                              ctx,
                                              found,
                                              conflict,
                                              dim,
                                              unit,
                                              unitResolved);
            }
            break;

        case AST_LABEL_DECLARATION:
            collect_function_return_units(node->label.statement, ctx, found, conflict, dim, unit, unitResolved);
            break;

        case AST_STATEMENT_EXPRESSION:
            collect_function_return_units(node->statementExpr.block, ctx, found, conflict, dim, unit, unitResolved);
            break;

        case AST_CONDITIONAL_DIRECTIVE:
            collect_function_return_units(node->conditionalDirective.body, ctx, found, conflict, dim, unit, unitResolved);
            break;

        default:
            break;
    }
}

static void maybe_record_function_call_result(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_FUNCTION_CALL || node_is_explicit_units_convert_call(node)) return;
    Symbol* sym = resolve_named_callee_symbol(node->functionCall.callee);
    if (!sym || !sym->definition || sym->definition->type != AST_FUNCTION_DEFINITION) return;

    bool found = false;
    bool conflict = false;
    FisicsDim8 returnDim = fisics_dim_zero();
    const FisicsUnitDef* returnUnit = NULL;
    bool returnUnitResolved = false;
    collect_function_return_units(sym->definition->functionDef.body,
                                  ctx,
                                  &found,
                                  &conflict,
                                  &returnDim,
                                  &returnUnit,
                                  &returnUnitResolved);
    if (!found || conflict) return;

    (void)fisics_extension_set_units_expr_result_with_unit(ctx,
                                                           node,
                                                           returnDim,
                                                           true,
                                                           returnUnitResolved ? returnUnit : NULL,
                                                           returnUnitResolved);
}

static void maybe_validate_function_call_argument_units(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_FUNCTION_CALL) return;
    bool sawContract = false;
    for (size_t i = 0; i < node->functionCall.argumentCount; ++i) {
        const FisicsUnitsCallArgContract* contract =
            fisics_extension_lookup_units_call_arg_contract(ctx, node, i);
        if (!contract) continue;
        sawContract = true;
        if (!contract->resolved || !contract->unitResolved || !contract->unitDef) continue;

        ASTNode* argNode = node->functionCall.arguments ? node->functionCall.arguments[i] : NULL;
        FisicsDim8 argDim = fisics_dim_zero();
        const FisicsUnitDef* argUnit = NULL;
        bool argUnitResolved = false;
        if (!lookup_resolved_expr_metadata(ctx, argNode, &argDim, &argUnit, &argUnitResolved)) continue;
        if (!fisics_dim_equal(argDim, contract->dim)) continue;
        if (units_resolved_and_different(contract->unitDef, true, argUnit, argUnitResolved)) {
            fisics_extension_diag_units_implicit_unit_conversion(ctx,
                                                                 argNode ? argNode : node,
                                                                 "argument",
                                                                 argUnit,
                                                                 contract->unitDef);
        }
    }
    if (sawContract) return;

    Symbol* sym = resolve_named_callee_symbol(node->functionCall.callee);
    if (!sym || sym->kind != SYMBOL_FUNCTION) return;

    size_t pairCount = node->functionCall.argumentCount < sym->signature.paramCount
                           ? node->functionCall.argumentCount
                           : sym->signature.paramCount;
    for (size_t i = 0; i < pairCount; ++i) {
        ASTNode* paramDecl = function_param_decl_at(sym, i);
        const FisicsUnitsAnnotation* ann = fisics_extension_lookup_units_annotation(ctx, paramDecl);
        if (!ann || !ann->resolved || !ann->unitResolved || !ann->unitDef || ann->dimDuplicateCount > 1) continue;

        ASTNode* argNode = node->functionCall.arguments ? node->functionCall.arguments[i] : NULL;
        FisicsDim8 argDim = fisics_dim_zero();
        const FisicsUnitDef* argUnit = NULL;
        bool argUnitResolved = false;
        if (!lookup_resolved_expr_metadata(ctx, argNode, &argDim, &argUnit, &argUnitResolved)) continue;
        if (!fisics_dim_equal(argDim, ann->dim)) continue;
        if (units_resolved_and_different(ann->unitDef, true, argUnit, argUnitResolved)) {
            fisics_extension_diag_units_implicit_unit_conversion(ctx,
                                                                 argNode ? argNode : node,
                                                                 "argument",
                                                                 argUnit,
                                                                 ann->unitDef);
        }
    }
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

static bool units_annotation_is_concrete(const FisicsUnitsAnnotation* ann) {
    return ann && ann->resolved && ann->dimDuplicateCount <= 1 && ann->unitResolved && ann->unitDef;
}

static bool units_annotations_same_concrete(const FisicsUnitsAnnotation* a,
                                            const FisicsUnitsAnnotation* b) {
    return units_annotation_is_concrete(a) &&
           units_annotation_is_concrete(b) &&
           fisics_dim_equal(a->dim, b->dim) &&
           a->unitDef == b->unitDef;
}

static const FisicsUnitsAnnotation* lookup_units_pointer_alias(const char* name) {
    if (!name || !name[0]) return NULL;
    for (size_t i = s_units_pointer_alias_count; i > 0; --i) {
        const UnitsPointerAlias* alias = &s_units_pointer_aliases[i - 1];
        if (alias->name && strcmp(alias->name, name) == 0) {
            return alias->annotation;
        }
    }
    return NULL;
}

static void remember_units_pointer_alias(const char* name, const FisicsUnitsAnnotation* ann) {
    if (!name || !name[0] || !units_annotation_is_concrete(ann)) return;
    for (size_t i = 0; i < s_units_pointer_alias_count; ++i) {
        UnitsPointerAlias* alias = &s_units_pointer_aliases[i];
        if (alias->name && strcmp(alias->name, name) == 0) {
            alias->annotation = ann;
            return;
        }
    }
    if (s_units_pointer_alias_count >= UNITS_POINTER_ALIAS_MAX) return;
    s_units_pointer_aliases[s_units_pointer_alias_count].name = name;
    s_units_pointer_aliases[s_units_pointer_alias_count].annotation = ann;
    ++s_units_pointer_alias_count;
}

static const ParsedType* lookup_bound_units_parsed_type(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !node) return NULL;
    return fisics_extension_lookup_units_symbol_type_binding(ctx, node);
}

static const FisicsUnitsAnnotation* lookup_assignment_lvalue_units_annotation(CompilerContext* ctx,
                                                                              ASTNode* node,
                                                                              int depth);

static const FisicsUnitsAnnotation* lookup_address_target_units_annotation(CompilerContext* ctx,
                                                                          ASTNode* node,
                                                                          int depth) {
    if (!ctx || !node || depth > 16) return NULL;

    if (node->type == AST_UNARY_EXPRESSION && node->expr.op && strcmp(node->expr.op, "&") == 0) {
        return lookup_assignment_lvalue_units_annotation(ctx, node->expr.left, depth + 1);
    }

    if (node->type == AST_TERNARY_EXPRESSION) {
        const FisicsUnitsAnnotation* left =
            lookup_address_target_units_annotation(ctx, node->ternaryExpr.trueExpr, depth + 1);
        const FisicsUnitsAnnotation* right =
            lookup_address_target_units_annotation(ctx, node->ternaryExpr.falseExpr, depth + 1);
        return units_annotations_same_concrete(left, right) ? left : NULL;
    }

    if (node->type == AST_COMMA_EXPRESSION && node->commaExpr.exprCount > 0) {
        ASTNode* tail = node->commaExpr.expressions
                            ? node->commaExpr.expressions[node->commaExpr.exprCount - 1]
                            : NULL;
        return lookup_address_target_units_annotation(ctx, tail, depth + 1);
    }

    if (node->type == AST_CAST_EXPRESSION) {
        return lookup_address_target_units_annotation(ctx, node->castExpr.expression, depth + 1);
    }

    if (node->type == AST_IDENTIFIER) {
        const FisicsUnitsAnnotation* aliasAnn = lookup_units_pointer_alias(node->valueNode.value);
        if (units_annotation_is_concrete(aliasAnn)) return aliasAnn;

        DesignatedInit* init =
            fisics_extension_lookup_units_symbol_initializer_binding(ctx, node);
        if (init && init->expression) {
            return lookup_address_target_units_annotation(ctx, init->expression, depth + 1);
        }
    }

    return NULL;
}

static void maybe_remember_units_pointer_alias(CompilerContext* ctx,
                                               ASTNode* nameNode,
                                               DesignatedInit* init) {
    if (!ctx || !nameNode || nameNode->type != AST_IDENTIFIER || !init || !init->expression) return;
    const FisicsUnitsAnnotation* ann = lookup_address_target_units_annotation(ctx, init->expression, 0);
    remember_units_pointer_alias(nameNode->valueNode.value, ann);
}

static bool lookup_member_base_type_info(CompilerContext* ctx,
                                         ASTNode* base,
                                         bool pointerAccess,
                                         TypeInfo* outInfo) {
    if (!ctx || !base || !outInfo || !s_units_root_scope) return false;
    const ParsedType* parsed = lookup_bound_units_parsed_type(ctx, base);
    ParsedType scratch;
    memset(&scratch, 0, sizeof(scratch));
    bool haveScratch = false;

    if (base->type == AST_POINTER_DEREFERENCE) {
        const ParsedType* ptrParsed = lookup_bound_units_parsed_type(ctx, base->pointerDeref.pointer);
        if (ptrParsed) {
            scratch = parsedTypePointerTargetType(ptrParsed);
            parsed = &scratch;
            haveScratch = true;
        }
    }

    if (!parsed) return false;
    if (pointerAccess) {
        ParsedType target = parsedTypePointerTargetType(parsed);
        if (haveScratch) {
            parsedTypeFree(&scratch);
            haveScratch = false;
        }
        scratch = target;
        parsed = &scratch;
        haveScratch = true;
    }

    *outInfo = typeInfoFromParsedType(parsed, s_units_root_scope);
    if (haveScratch) {
        parsedTypeFree(&scratch);
    }
    return outInfo->category != TYPEINFO_INVALID;
}

static bool var_decl_contains_name(ASTNode* decl, const char* name) {
    if (!decl || decl->type != AST_VARIABLE_DECLARATION || !name || !name[0]) return false;
    for (size_t i = 0; i < decl->varDecl.varCount; ++i) {
        ASTNode* varName = decl->varDecl.varNames ? decl->varDecl.varNames[i] : NULL;
        const char* value = (varName && varName->type == AST_IDENTIFIER) ? varName->valueNode.value : NULL;
        if (value && strcmp(value, name) == 0) return true;
    }
    return false;
}

static const FisicsUnitsAnnotation* lookup_units_annotation_by_decl_name(CompilerContext* ctx,
                                                                         const char* name) {
    if (!ctx || !ctx->extensionState || !name || !name[0]) return NULL;
    const FisicsUnitsAnnotation* match = NULL;
    for (size_t i = 0; i < ctx->extensionState->unitsAnnotationCount; ++i) {
        const FisicsUnitsAnnotation* ann = &ctx->extensionState->unitsAnnotations[i];
        if (!units_annotation_is_concrete(ann) || !var_decl_contains_name(ann->node, name)) continue;
        if (match && !units_annotations_same_concrete(match, ann)) {
            return NULL;
        }
        match = ann;
    }
    return match;
}

static const FisicsUnitsAnnotation* lookup_member_units_annotation(CompilerContext* ctx,
                                                                  ASTNode* node,
                                                                  int depth) {
    if (!ctx || !node || depth > 16) return NULL;
    if (node->type != AST_DOT_ACCESS && node->type != AST_POINTER_ACCESS) return NULL;
    if (!node->memberAccess.field || !node->memberAccess.field[0]) return NULL;

    TypeInfo baseInfo = makeInvalidType();
    if (!lookup_member_base_type_info(ctx,
                                      node->memberAccess.base,
                                      node->type == AST_POINTER_ACCESS,
                                      &baseInfo)) {
        return lookup_units_annotation_by_decl_name(ctx, node->memberAccess.field);
    }
    if (!type_info_is_record(&baseInfo)) {
        return lookup_units_annotation_by_decl_name(ctx, node->memberAccess.field);
    }

    ASTNode* recordDef = resolve_units_record_definition(baseInfo.originalType, &baseInfo, ctx);
    if (!recordDef) {
        return lookup_units_annotation_by_decl_name(ctx, node->memberAccess.field);
    }

    ASTNode* fieldDecl = NULL;
    if (!lookup_units_record_field(recordDef, node->memberAccess.field, 0, &fieldDecl, NULL)) {
        return lookup_units_annotation_by_decl_name(ctx, node->memberAccess.field);
    }
    const FisicsUnitsAnnotation* ann = fisics_extension_lookup_units_annotation(ctx, fieldDecl);
    if (units_annotation_is_concrete(ann)) return ann;
    return lookup_units_annotation_by_decl_name(ctx, node->memberAccess.field);
}

static const FisicsUnitsAnnotation* lookup_assignment_lvalue_units_annotation(CompilerContext* ctx,
                                                                              ASTNode* node,
                                                                              int depth) {
    if (!ctx || !node || depth > 16) return NULL;

    const FisicsUnitsAnnotation* ann = fisics_extension_lookup_units_annotation_binding(ctx, node);
    if (units_annotation_is_concrete(ann)) return ann;

    switch (node->type) {
        case AST_ARRAY_ACCESS:
            return lookup_assignment_lvalue_units_annotation(ctx, node->arrayAccess.array, depth + 1);
        case AST_DOT_ACCESS:
        case AST_POINTER_ACCESS:
            return lookup_member_units_annotation(ctx, node, depth + 1);
        case AST_POINTER_DEREFERENCE:
            return lookup_address_target_units_annotation(ctx, node->pointerDeref.pointer, depth + 1);
        case AST_UNARY_EXPRESSION:
            if (node->expr.op && strcmp(node->expr.op, "*") == 0) {
                return lookup_address_target_units_annotation(ctx, node->expr.left, depth + 1);
            }
            break;
        case AST_COMMA_EXPRESSION:
            if (node->commaExpr.exprCount > 0) {
                ASTNode* tail = node->commaExpr.expressions
                                    ? node->commaExpr.expressions[node->commaExpr.exprCount - 1]
                                    : NULL;
                return lookup_assignment_lvalue_units_annotation(ctx, tail, depth + 1);
            }
            break;
        case AST_CAST_EXPRESSION:
            return lookup_assignment_lvalue_units_annotation(ctx, node->castExpr.expression, depth + 1);
        default:
            break;
    }

    return NULL;
}

static bool fill_units_from_lvalue_annotation(CompilerContext* ctx,
                                              ASTNode* target,
                                              FisicsDim8* outDim,
                                              const FisicsUnitDef** outUnit,
                                              bool* outUnitResolved) {
    if (!ctx || !target || !outDim || !outUnit || !outUnitResolved) return false;
    const FisicsUnitsAnnotation* ann = lookup_assignment_lvalue_units_annotation(ctx, target, 0);
    if (!units_annotation_is_concrete(ann)) return false;
    *outDim = ann->dim;
    *outUnit = ann->unitDef;
    *outUnitResolved = true;
    return true;
}

static void maybe_record_assignment_result(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_ASSIGNMENT) return;
    if (!node->assignment.op) return;
    FisicsDim8 targetDim = fisics_dim_zero();
    FisicsDim8 valueDim = fisics_dim_zero();
    const FisicsUnitDef* targetUnit = NULL;
    const FisicsUnitDef* valueUnit = NULL;
    bool targetUnitResolved = false;
    bool valueUnitResolved = false;
    bool haveTarget = lookup_resolved_expr_metadata(ctx,
                                                    node->assignment.target,
                                                    &targetDim,
                                                    &targetUnit,
                                                    &targetUnitResolved);
    if (!haveTarget) {
        haveTarget = fill_units_from_lvalue_annotation(ctx,
                                                       node->assignment.target,
                                                       &targetDim,
                                                       &targetUnit,
                                                       &targetUnitResolved);
    }
    if (!haveTarget ||
        !lookup_resolved_expr_metadata(ctx, node->assignment.value, &valueDim, &valueUnit, &valueUnitResolved)) {
        return;
    }

    if (strcmp(node->assignment.op, "=") == 0) {
        if (!fisics_dim_equal(targetDim, valueDim)) {
            fisics_extension_diag_units_assign_dim_mismatch(ctx, node, targetDim, valueDim);
            return;
        }
        if (units_resolved_and_different(targetUnit, targetUnitResolved, valueUnit, valueUnitResolved)) {
            fisics_extension_diag_units_implicit_unit_conversion(ctx,
                                                                 node->assignment.value ? node->assignment.value : node,
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
        return;
    }

    const char* baseOp = compound_assignment_base_op(node->assignment.op);
    if (!baseOp) return;

    if (strcmp(baseOp, "+") == 0 || strcmp(baseOp, "-") == 0) {
        if (!fisics_dim_equal(targetDim, valueDim)) {
            if (strcmp(baseOp, "+") == 0) {
                fisics_extension_diag_units_add_dim_mismatch(ctx, node, targetDim, valueDim);
            } else {
                fisics_extension_diag_units_sub_dim_mismatch(ctx, node, targetDim, valueDim);
            }
            return;
        }
        if (units_resolved_and_different(targetUnit, targetUnitResolved, valueUnit, valueUnitResolved)) {
            fisics_extension_diag_units_implicit_unit_conversion(ctx,
                                                                 node,
                                                                 "compound assignment",
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
        return;
    }

    FisicsDim8 combinedDim = fisics_dim_zero();
    bool ok = (strcmp(baseOp, "*") == 0)
                  ? fisics_dim_add(targetDim, valueDim, &combinedDim)
                  : fisics_dim_sub(targetDim, valueDim, &combinedDim);
    if (!ok) {
        fisics_extension_diag_units_exponent_overflow(ctx, node, baseOp, targetDim, valueDim);
        return;
    }
    if (!fisics_dim_equal(targetDim, combinedDim)) {
        fisics_extension_diag_units_assign_dim_mismatch(ctx, node, targetDim, combinedDim);
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
    maybe_record_owner_literal(ctx, declNode, init->expression);
}

static void maybe_record_owner_literal(CompilerContext* ctx,
                                       ASTNode* ownerNode,
                                       ASTNode* expression) {
    if (!ctx || !ownerNode || !expression) return;
    if (!is_dimensionless_literal(expression)) return;

    const FisicsUnitsAnnotation* ann = fisics_extension_lookup_units_annotation(ctx, ownerNode);
    if (!ann || !ann->resolved || ann->dimDuplicateCount > 1) return;

    (void)fisics_extension_set_units_expr_result_with_unit(ctx,
                                                           expression,
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

static bool type_info_is_record(const TypeInfo* info) {
    return info && (info->category == TYPEINFO_STRUCT || info->category == TYPEINFO_UNION);
}

static ASTNode* resolve_units_record_definition(const ParsedType* parsedType,
                                                const TypeInfo* info,
                                                CompilerContext* ctx) {
    if (parsedType && parsedType->inlineStructOrUnionDef) {
        ASTNode* def = parsedType->inlineStructOrUnionDef;
        if (def->type == AST_STRUCT_DEFINITION || def->type == AST_UNION_DEFINITION) {
            return def;
        }
    }
    if (!info || !ctx || !info->userTypeName || !type_info_is_record(info)) {
        return NULL;
    }
    CCTagKind kind = (info->category == TYPEINFO_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
    ASTNode* def = cc_tag_definition(ctx, kind, info->userTypeName);
    if (!def || (def->type != AST_STRUCT_DEFINITION && def->type != AST_UNION_DEFINITION)) {
        return NULL;
    }
    return def;
}

static bool lookup_units_record_field(ASTNode* recordDef,
                                      const char* fieldName,
                                      size_t positionalIndex,
                                      ASTNode** outFieldDecl,
                                      const ParsedType** outFieldType) {
    if (outFieldDecl) *outFieldDecl = NULL;
    if (outFieldType) *outFieldType = NULL;
    if (!recordDef || (recordDef->type != AST_STRUCT_DEFINITION &&
                       recordDef->type != AST_UNION_DEFINITION)) {
        return false;
    }

    size_t ordinal = 0;
    for (size_t i = 0; i < recordDef->structDef.fieldCount; ++i) {
        ASTNode* field = recordDef->structDef.fields ? recordDef->structDef.fields[i] : NULL;
        if (!field || field->type != AST_VARIABLE_DECLARATION) {
            continue;
        }
        for (size_t k = 0; k < field->varDecl.varCount; ++k) {
            ASTNode* name = field->varDecl.varNames ? field->varDecl.varNames[k] : NULL;
            const char* nameValue = (name && name->type == AST_IDENTIFIER) ? name->valueNode.value : NULL;
            bool matched = false;
            if (fieldName && fieldName[0]) {
                matched = nameValue && strcmp(nameValue, fieldName) == 0;
            } else {
                matched = ordinal == positionalIndex;
            }
            if (matched) {
                if (outFieldDecl) *outFieldDecl = field;
                if (outFieldType) {
                    *outFieldType = field->varDecl.declaredTypes ? &field->varDecl.declaredTypes[k]
                                                                  : &field->varDecl.declaredType;
                }
                return true;
            }
            ++ordinal;
        }
    }
    return false;
}

static void validate_units_initializer_against_owner(CompilerContext* ctx,
                                                     ASTNode* ownerNode,
                                                     ASTNode* expression) {
    if (!ctx || !ownerNode || !expression) return;

    const FisicsUnitsAnnotation* ann = fisics_extension_lookup_units_annotation(ctx, ownerNode);
    if (!ann || !ann->resolved || ann->dimDuplicateCount > 1 || !ann->unitResolved || !ann->unitDef) return;

    FisicsDim8 initDim = fisics_dim_zero();
    const FisicsUnitDef* initUnit = NULL;
    bool initUnitResolved = false;
    if (!lookup_resolved_expr_metadata(ctx, expression, &initDim, &initUnit, &initUnitResolved)) return;
    if (!fisics_dim_equal(initDim, ann->dim)) return;
    if (units_resolved_and_different(ann->unitDef, true, initUnit, initUnitResolved)) {
        fisics_extension_diag_units_implicit_unit_conversion(ctx,
                                                             expression,
                                                             "initializer",
                                                             initUnit,
                                                             ann->unitDef);
    }
}

static void validate_units_aggregate_initializer(CompilerContext* ctx,
                                                 const ParsedType* aggregateType,
                                                 ASTNode* compoundExpr,
                                                 int depth) {
    if (!ctx || !aggregateType || !compoundExpr || compoundExpr->type != AST_COMPOUND_LITERAL) return;
    if (!s_units_root_scope || depth > 16) return;

    TypeInfo aggregateInfo = typeInfoFromParsedType(aggregateType, s_units_root_scope);
    if (!type_info_is_record(&aggregateInfo)) return;

    ASTNode* recordDef = resolve_units_record_definition(aggregateType, &aggregateInfo, ctx);
    if (!recordDef) return;

    size_t positionalIndex = 0;
    for (size_t i = 0; i < compoundExpr->compoundLiteral.entryCount; ++i) {
        DesignatedInit* entry =
            compoundExpr->compoundLiteral.entries ? compoundExpr->compoundLiteral.entries[i] : NULL;
        if (!entry || !entry->expression) {
            if (entry && (!entry->fieldName || !entry->fieldName[0])) {
                ++positionalIndex;
            }
            continue;
        }

        ASTNode* fieldDecl = NULL;
        const ParsedType* fieldType = NULL;
        if (!lookup_units_record_field(recordDef,
                                       entry->fieldName,
                                       positionalIndex,
                                       &fieldDecl,
                                       &fieldType)) {
            if (!entry->fieldName || !entry->fieldName[0]) {
                ++positionalIndex;
            }
            continue;
        }

        if (entry->expression->type == AST_COMPOUND_LITERAL && fieldType) {
            TypeInfo fieldInfo = typeInfoFromParsedType(fieldType, s_units_root_scope);
            if (type_info_is_record(&fieldInfo)) {
                validate_units_aggregate_initializer(ctx, fieldType, entry->expression, depth + 1);
            } else {
                validate_units_initializer_against_owner(ctx, fieldDecl, entry->expression);
            }
        } else {
            validate_units_initializer_against_owner(ctx, fieldDecl, entry->expression);
        }

        if (!entry->fieldName || !entry->fieldName[0]) {
            ++positionalIndex;
        }
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
                maybe_remember_units_pointer_alias(ctx,
                                                   node->varDecl.varNames ? node->varDecl.varNames[i] : NULL,
                                                   init);
                if (init && init->expression && init->expression->type == AST_COMPOUND_LITERAL) {
                    const ParsedType* varType = astVarDeclTypeAt(node, i);
                    if (varType && init->expression->compoundLiteral.literalType.kind == TYPE_INVALID) {
                        validate_units_aggregate_initializer(ctx, varType, init->expression, 0);
                    }
                }
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
            maybe_record_function_call_result(ctx, node);
            maybe_validate_function_call_argument_units(ctx, node);
            break;

        case AST_COMPOUND_LITERAL:
            for (size_t i = 0; i < node->compoundLiteral.entryCount; ++i) {
                walk_designated_init(node,
                                     node->compoundLiteral.entries ? node->compoundLiteral.entries[i] : NULL,
                                     ctx);
            }
            if (node->compoundLiteral.literalType.kind != TYPE_INVALID) {
                validate_units_aggregate_initializer(ctx, &node->compoundLiteral.literalType, node, 0);
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
    s_units_root_scope = globalScope;
    memset(s_units_pointer_aliases, 0, sizeof(s_units_pointer_aliases));
    s_units_pointer_alias_count = 0;
    fisics_extension_clear_units_expr_results(ctx);
    walk_expr_results(root, ctx);
    s_units_root_scope = NULL;
}
