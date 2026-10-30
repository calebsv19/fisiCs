// SPDX-License-Identifier: Apache-2.0
#include "Extensions/Units/units_math_semantics.h"

#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Extensions/extension_diagnostics.h"
#include "Extensions/extension_hooks.h"
#include "Extensions/extension_units_expr_table.h"
#include "Syntax/symbol_table.h"
#include <stdlib.h>
#include <string.h>

static bool is_math_header_declaration(const CompilerContext* ctx, const Symbol* symbol) {
    if (!symbol->definition || symbol->definition->type != AST_FUNCTION_DECLARATION)
        return false;
    ASTNode* name = symbol->definition->functionDecl.funcName;
    const char* file = name ? name->location.start.file : NULL;
    if (!file) file = symbol->definition->location.start.file;
    if (!file) return false;
    size_t count = 0;
    const FisicsInclude* includes = cc_get_includes(ctx, &count);
    bool mathIncluded = false, declarationInSystemHeader = false;
    for (size_t i = 0; i < count; ++i) {
        const FisicsInclude* inc = &includes[i];
        if (!inc->resolved || inc->origin != FISICS_INCLUDE_SYSTEM_ORIGIN) continue;
        if (inc->name && strcmp(inc->name, "math.h") == 0) mathIncluded = true;
        if (inc->resolved_path && strcmp(inc->resolved_path, file) == 0)
            declarationInSystemHeader = true;
    }
    /* Some C libraries put the declarations in nested system math headers. */
    return mathIncluded && declarationInSystemHeader;
}

/* Only compiler-seeded builtins and the compiler's own virtual tgmath wrappers
 * qualify without a standard math-header declaration. User names never suffice. */
static bool is_virtual_math_wrapper(const Symbol* callee) {
    const ASTNode* def = callee->definition;
    return def && def->type == AST_FUNCTION_DEFINITION &&
        def->location.start.file &&
        !strcmp(def->location.start.file, "<fisics>/tgmath.h");
}

void fisics_units_note_math_call(CompilerContext* ctx, ASTNode* call,
                                const Symbol* callee, bool realArguments) {
    if (!cc_overlay_physics_units_enabled(ctx) || !call || !callee ||
        callee->kind != SYMBOL_FUNCTION || !callee->name || !realArguments) return;
    static const struct { const char* name; FisicsUnitsMathOperation op; size_t arity; } rules[] = {
        {"sqrt", FISICS_UNITS_MATH_SQRT, 1}, {"fabs", FISICS_UNITS_MATH_FABS, 1},
        {"hypot", FISICS_UNITS_MATH_HYPOT, 2}, {"fmax", FISICS_UNITS_MATH_FMAX, 2},
        {"expm1", FISICS_UNITS_MATH_EXPM1, 1}, {"log", FISICS_UNITS_MATH_LOG, 1},
        {"fmin", FISICS_UNITS_MATH_FMIN, 2}, {"atan2", FISICS_UNITS_MATH_ATAN2, 2}
    };
    FisicsUnitsMathOperation op = FISICS_UNITS_MATH_NONE;
    for (size_t i = 0; i < sizeof(rules)/sizeof(rules[0]); ++i) {
        const char* name = callee->name;
        bool generic = !strncmp(name, "__fisics_tg_", 12);
        bool builtin = !strncmp(name, "__builtin_", 10);
        const char* base = generic ? name + 12 : builtin ? name + 10 : name;
        size_t n = strlen(rules[i].name);
        bool spelling = !strncmp(base, rules[i].name, n) &&
            (!base[n] || (!generic && (base[n] == 'f' || base[n] == 'l') && !base[n+1]));
        if (!spelling || call->functionCall.argumentCount != rules[i].arity) continue;
        bool seededGeneric = !callee->definition &&
            (rules[i].op == FISICS_UNITS_MATH_SQRT || rules[i].op == FISICS_UNITS_MATH_FABS);
        bool trusted = generic ? (seededGeneric || is_virtual_math_wrapper(callee)) :
            builtin ? (rules[i].op == FISICS_UNITS_MATH_FABS && !callee->definition) :
            is_math_header_declaration(ctx, callee);
        bool signature = generic && seededGeneric;
        signature = signature || (callee->signature.hasPrototype && !callee->signature.isVariadic &&
                                   callee->signature.paramCount == rules[i].arity);
        /* New contracts require an exact real floating prototype, not just
         * a name and parameter count. Definitions/user wrappers stay unknown. */
        if (rules[i].op >= FISICS_UNITS_MATH_EXPM1 ||
            (!generic && (rules[i].op == FISICS_UNITS_MATH_SQRT || rules[i].op == FISICS_UNITS_MATH_HYPOT))) {
            TokenType primitive = base[n] == 'f' ? TOKEN_FLOAT : TOKEN_DOUBLE;
            bool extended = base[n] == 'l';
            const ParsedType* result = callee->definition &&
                callee->definition->type == AST_FUNCTION_DECLARATION ?
                &callee->definition->functionDecl.returnType : NULL;
            signature = signature && !generic && result &&
                result->kind == TYPE_PRIMITIVE && result->primitiveType == primitive &&
                result->isLong == extended && !result->pointerDepth &&
                !parsedTypeIsDirectArray(result) && !result->isFunctionPointer &&
                !result->isComplex && !result->isImaginary;
            for (size_t j = 0; signature && j < rules[i].arity; j++) {
                const ParsedType* arg = &callee->signature.params[j];
                signature = arg->kind == TYPE_PRIMITIVE && arg->primitiveType == primitive &&
                    arg->isLong == extended && !arg->pointerDepth &&
                    !parsedTypeIsDirectArray(arg) && !arg->isFunctionPointer &&
                    !arg->isComplex && !arg->isImaginary;
            }
        }
        if (trusted && signature) op = rules[i].op;
    }
    if (op == FISICS_UNITS_MATH_NONE || !fisics_extension_state_ensure(ctx)) return;
    FisicsExtensionState* state = ctx->extensionState;
    for (size_t i = 0; i < state->unitsMathCallCount; ++i)
        if (state->unitsMathCalls[i].node == call) return;
    if (state->unitsMathCallCount == state->unitsMathCallCapacity) {
        size_t cap = state->unitsMathCallCapacity ? state->unitsMathCallCapacity * 2 : 8;
        FisicsUnitsMathCall* grown = realloc(state->unitsMathCalls, cap * sizeof(*grown));
        if (!grown) return;
        state->unitsMathCalls = grown;
        state->unitsMathCallCapacity = cap;
    }
    state->unitsMathCalls[state->unitsMathCallCount++] = (FisicsUnitsMathCall){call, op};
}

static const FisicsUnitDef* sqrt_unit(const FisicsUnitDef* source, FisicsDim8 resultDim) {
    static const char* const roots[][2] = {
        {"square_meter", "meter"},
        {"square_millimeter", "millimeter"},
        {"square_centimeter", "centimeter"},
        {"meter_to_fourth", "square_meter"},
        {"millimeter_to_fourth", "square_millimeter"},
        {"centimeter_to_fourth", "square_centimeter"},
    };
    for (size_t i = 0; i < sizeof(roots) / sizeof(roots[0]); ++i) {
        const FisicsUnitDef* result = NULL;
        if (!strcmp(source->name, roots[i][0]) && fisics_unit_lookup(roots[i][1], &result) &&
            result && fisics_dim_equal(result->dim, resultDim)) return result;
    }
    return NULL;
}

FisicsUnitsMathOperation fisics_units_math_operation(const CompilerContext *ctx,const ASTNode *call) {
    if(!ctx || !ctx->extensionState || !call)return FISICS_UNITS_MATH_NONE;
    const FisicsExtensionState *state=ctx->extensionState;
    for(size_t i=0;i<state->unitsMathCallCount;i++)
        if(state->unitsMathCalls[i].node==call)return state->unitsMathCalls[i].operation;
    return FISICS_UNITS_MATH_NONE;
}

bool fisics_units_record_math_result(CompilerContext* ctx, ASTNode* call) {
    if (!ctx || !ctx->extensionState || !call) return false;
    FisicsExtensionState* state = ctx->extensionState;
    FisicsUnitsMathOperation op = FISICS_UNITS_MATH_NONE;
    for (size_t i = 0; i < state->unitsMathCallCount; ++i)
        if (state->unitsMathCalls[i].node == call) { op = state->unitsMathCalls[i].operation; break; }
    if (op == FISICS_UNITS_MATH_NONE) return false;
    const FisicsUnitsExprResult* arg = fisics_extension_lookup_units_expr_result(
        ctx, call->functionCall.arguments[0]);
    if (!arg || !arg->resolved) return true;
    if (op == FISICS_UNITS_MATH_EXPM1 || op == FISICS_UNITS_MATH_LOG) {
        FisicsDim8 zero = fisics_dim_zero();
        if (!fisics_dim_equal(arg->dim, zero)) {
            fisics_extension_diag_units_math_argument_dim_mismatch(ctx, call,
                op == FISICS_UNITS_MATH_EXPM1 ? "expm1" : "log", zero, arg->dim);
            return true;
        }
        (void)fisics_extension_set_units_expr_result_with_unit(ctx, call, zero, true, NULL, false);
        return true;
    }
    if (op == FISICS_UNITS_MATH_ATAN2) {
        const FisicsUnitsExprResult *rhs = fisics_extension_lookup_units_expr_result(ctx, call->functionCall.arguments[1]);
        if (!rhs || !rhs->resolved) return true;
        if (!fisics_dim_equal(arg->dim, rhs->dim)) {
            fisics_extension_diag_units_math_argument_dim_mismatch(ctx, call, "atan2", arg->dim, rhs->dim);
            return true;
        }
        if (arg->unitResolved && rhs->unitResolved && strcmp(arg->unitDef->name, rhs->unitDef->name)) {
            fisics_extension_diag_units_implicit_unit_conversion(ctx, call, "atan2", arg->unitDef, rhs->unitDef);
            return true;
        }
        (void)fisics_extension_set_units_expr_result_with_unit(ctx, call, fisics_dim_zero(), true, NULL, false);
        return true;
    }
    if (op != FISICS_UNITS_MATH_SQRT) {
        FisicsDim8 dim = arg->dim;
        const FisicsUnitDef* unit = arg->unitResolved ? arg->unitDef : NULL;
        if (op != FISICS_UNITS_MATH_FABS) {
            const char* name = op == FISICS_UNITS_MATH_HYPOT ? "hypot" : op == FISICS_UNITS_MATH_FMIN ? "fmin" : "fmax";
            const FisicsUnitsExprResult* rhs = fisics_extension_lookup_units_expr_result(
                ctx, call->functionCall.arguments[1]);
            if (!rhs || !rhs->resolved) return true;
            if (!fisics_dim_equal(dim, rhs->dim)) {
                fisics_extension_diag_units_math_argument_dim_mismatch(ctx, call, name, dim, rhs->dim);
                return true;
            }
            const FisicsUnitDef* rhsUnit = rhs->unitResolved ? rhs->unitDef : NULL;
            if (unit && rhsUnit && strcmp(unit->name, rhsUnit->name)) {
                fisics_extension_diag_units_implicit_unit_conversion(ctx, call, name, unit, rhsUnit);
                return true;
            }
            if (!rhsUnit) unit = NULL;
        }
        (void)fisics_extension_set_units_expr_result_with_unit(ctx, call, dim, true, unit, unit != NULL);
        return true;
    }
    FisicsDim8 resultDim = fisics_dim_zero();
    for (int i = 0; i < FISICS_DIM_COUNT; ++i) {
        if (arg->dim.e[i] % 2 != 0) {
            fisics_extension_diag_units_sqrt_unsupported(ctx, call, arg->dim, NULL,
                "square root requires even exponents on every dimension axis");
            return true;
        }
        resultDim.e[i] = arg->dim.e[i] / 2;
    }
    const FisicsUnitDef* resultUnit = NULL;
    if (arg->unitResolved && arg->unitDef) {
        resultUnit = sqrt_unit(arg->unitDef, resultDim);
        if (!resultUnit) {
            fisics_extension_diag_units_sqrt_unsupported(ctx, call, arg->dim, arg->unitDef,
                "square root of this concrete unit has no supported registry mapping");
            return true;
        }
    }
    /* Copy argument facts before insertion: the expression table may reallocate. */
    (void)fisics_extension_set_units_expr_result_with_unit(ctx, call, resultDim, true,
                                                         resultUnit, resultUnit != NULL);
    return true;
}
