// SPDX-License-Identifier: Apache-2.0
#include "Extensions/Units/units_coverage_model.h"
#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Extensions/extension_units_view.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/extension_units_expr_table.h"
#include "Extensions/extension_units_call_contracts.h"

static FisicsCoverageFact unknown(const ASTNode* site, FisicsCoverageReason reason) {
    FisicsCoverageFact fact = {0};
    fact.site = site;
    if (site) fact.location = site->location;
    fact.reason = reason;
    return fact;
}

static void declared(FisicsCoverageFact* fact, const ASTNode* owner,
                     FisicsDim8 dim, const FisicsUnitDef* unit) {
    fact->origin = FISICS_COVERAGE_DECLARED;
    fact->originNode = owner;
    if (owner) fact->originLocation = owner->location;
    fact->dimension = dim;
    fact->hasDimension = true;
    fact->unit = unit;
    fact->hasUnit = unit != NULL;
    fact->provenanceComplete = owner != NULL;
    fact->reason = FISICS_COVERAGE_NONE;
}

FisicsCoverageFact fisics_coverage_declaration(const CompilerContext* ctx,
    const ASTNode* node, size_t index) {
    FisicsCoverageFact fact = unknown(node, FISICS_COVERAGE_MISSING_ANNOTATION);
    if (!cc_overlay_physics_units_enabled(ctx)) {
        fact.reason = FISICS_COVERAGE_OVERLAY_DISABLED;
        return fact;
    }
    const FisicsUnitsAnnotation* ann =
        fisics_extension_lookup_units_declarator_annotation(ctx, node, index);
    if (!ann) return fact;
    if (!ann->resolved || ann->dimDuplicateCount > 1) {
        fact.reason = FISICS_COVERAGE_UNRESOLVED_ANNOTATION;
        return fact;
    }
    const ASTNode* owner = node;
    if (node && node->type == AST_VARIABLE_DECLARATION &&
        index < node->varDecl.varCount && node->varDecl.varNames)
        owner = node->varDecl.varNames[index];
    declared(&fact, owner, ann->dim, ann->unitResolved ? ann->unitDef : NULL);
    if (ann->unitExprText && (!ann->unitResolved || ann->unitDuplicateCount > 1)) {
        fact.provenanceComplete = false;
        fact.reason = FISICS_COVERAGE_UNRESOLVED_ANNOTATION;
    }
    return fact;
}

FisicsCoverageFact fisics_coverage_expression(const CompilerContext* ctx,
    const ASTNode* node) {
    FisicsCoverageFact fact = unknown(node, FISICS_COVERAGE_NO_INFERENCE);
    if (!cc_overlay_physics_units_enabled(ctx)) {
        fact.reason = FISICS_COVERAGE_OVERLAY_DISABLED;
        return fact;
    }
    if (!node) return fact;
    const FisicsUnitsAnnotation* ann = fisics_extension_lookup_units_annotation_binding(ctx, node);
    if (ann && ann->resolved && ann->dimDuplicateCount <= 1) {
        declared(&fact, ann->node, ann->dim, ann->unitResolved ? ann->unitDef : NULL);
        if (ann->node->type == AST_VARIABLE_DECLARATION && ann->node->varDecl.varNames &&
            ann->declaratorIndex < ann->node->varDecl.varCount) {
            fact.originNode = ann->node->varDecl.varNames[ann->declaratorIndex];
            if (fact.originNode) fact.originLocation = fact.originNode->location;
        }
        if (ann->unitExprText && (!ann->unitResolved || ann->unitDuplicateCount > 1)) {
            fact.provenanceComplete = false;
            fact.reason = FISICS_COVERAGE_UNRESOLVED_ANNOTATION;
        }
        return fact;
    }
    if (node->type == AST_IDENTIFIER) fact.reason = ann
        ? FISICS_COVERAGE_UNRESOLVED_ANNOTATION : FISICS_COVERAGE_MISSING_ANNOTATION;
    if (node->type == AST_DOT_ACCESS || node->type == AST_POINTER_ACCESS ||
        node->type == AST_ARRAY_ACCESS) {
        const ParsedType* type = fisics_extension_lookup_units_symbol_type_binding(ctx, node);
        bool resolved = false, unitResolved = false;
        FisicsDim8 dim;
        const FisicsUnitDef* unit = NULL;
        if (fisics_units_parse_type_contract(type, &dim, &resolved, &unit, &unitResolved) && resolved) {
            /* Type snapshots preserve the fact but not the original field AST.
             * Do not fabricate complete provenance from a matching field name. */
            declared(&fact, NULL, dim, unitResolved ? unit : NULL);
            fact.reason = FISICS_COVERAGE_UNTRACKED_PROVENANCE;
            return fact;
        }
        fact.reason = FISICS_COVERAGE_UNANNOTATED_STORAGE;
    }
    if (node->type == AST_FUNCTION_CALL) {
        const ASTNode* callee = node->functionCall.callee;
        const ParsedType* type = fisics_extension_lookup_units_symbol_type_binding(ctx, callee);
        bool resolved = false, unitResolved = false;
        FisicsDim8 dim;
        const FisicsUnitDef* unit = NULL;
        if (fisics_units_parse_type_contract(type, &dim, &resolved, &unit, &unitResolved) && resolved) {
            declared(&fact, fisics_extension_lookup_units_function_binding(ctx, callee),
                     dim, unitResolved ? unit : NULL);
            /* A signature is an interface assertion even when a body is present.
             * No body/check-edge provenance is stored by the existing table. */
            fact.trusted = true;
            fact.reason = FISICS_COVERAGE_DECLARED_INTERFACE;
            return fact;
        }
        fact.reason = FISICS_COVERAGE_UNCONTRACTED_CALL;
    }
    const FisicsUnitsExprResult* result = fisics_extension_lookup_units_expr_result(ctx, node);
    if (result && result->resolved) {
        fact.origin = FISICS_COVERAGE_INFERRED;
        fact.hasDimension = true;
        fact.dimension = result->dim;
        fact.hasUnit = result->unitResolved && result->unitDef;
        fact.unit = fact.hasUnit ? result->unitDef : NULL;
        /* Existing results lack dependency edges and can include annotation-
         * adopted literals. A resolved result is not a complete proof chain. */
        fact.reason = FISICS_COVERAGE_UNTRACKED_PROVENANCE;
    }
    return fact;
}

FisicsCoverageAssessment fisics_coverage_assess(FisicsCoverageObligation obligation) {
    FisicsCoverageAssessment row = {0};
    row.obligation = obligation;
    row.state = FISICS_COVERAGE_UNCHECKED;
    const FisicsCoverageFact *a = &obligation.expected, *b = &obligation.actual;
    if (!a->hasDimension || !b->hasDimension) {
        const FisicsCoverageFact* missing = !a->hasDimension ? a : b;
        row.reason = missing->reason ? missing->reason : FISICS_COVERAGE_NO_INFERENCE;
        row.lossNode = missing->site;
        return row;
    }
    row.dimensionCompared = true;
    if (!fisics_dim_equal(a->dimension, b->dimension)) {
        row.state = FISICS_COVERAGE_CONTRADICTORY;
        row.reason = FISICS_COVERAGE_DIMENSION_MISMATCH;
        return row;
    }
    if (a->hasUnit && b->hasUnit) {
        row.unitCompared = true;
        if (a->unit != b->unit) {
            row.state = FISICS_COVERAGE_CONTRADICTORY;
            row.reason = FISICS_COVERAGE_UNIT_MISMATCH;
            return row;
        }
    } else if (obligation.requireConcreteUnit) {
        row.reason = FISICS_COVERAGE_MISSING_UNIT;
        row.lossNode = !a->hasUnit ? a->site : b->site;
        return row;
    }
    if (a->trusted || b->trusted) {
        row.state = FISICS_COVERAGE_TRUSTED_BOUNDARY;
        row.reason = a->trusted ? a->reason : b->reason;
    } else if (!a->provenanceComplete || !b->provenanceComplete) {
        const FisicsCoverageFact* incomplete = !a->provenanceComplete ? a : b;
        row.reason = incomplete->reason ? incomplete->reason : FISICS_COVERAGE_UNTRACKED_PROVENANCE;
        row.lossNode = incomplete->site;
    } else row.state = FISICS_COVERAGE_CHECKED;
    return row;
}

bool fisics_coverage_summarize(const FisicsCoverageAssessment* rows, size_t count,
    FisicsCoverageSummary* out) {
    if (!out) return false;
    *out = (FisicsCoverageSummary){0};
    if (count && !rows) return false;
    FisicsCoverageSummary summary = {0};
    for (size_t i = 0; i < count; ++i) {
        const FisicsCoverageObligation* key = &rows[i].obligation;
        if (!key->site || key->boundary < FISICS_COVERAGE_INITIALIZER ||
            key->boundary > FISICS_COVERAGE_OPERATION) return false;
        for (size_t j = 0; j < i; ++j)
            if (rows[j].obligation.site == key->site && rows[j].obligation.boundary == key->boundary &&
                rows[j].obligation.slot == key->slot) return false;
        /* Recompute instead of trusting mutable caller-provided verdict counts. */
        FisicsCoverageAssessment assessment = fisics_coverage_assess(*key);
        switch (assessment.state) {
            case FISICS_COVERAGE_CHECKED: ++summary.checked; break;
            case FISICS_COVERAGE_TRUSTED_BOUNDARY: ++summary.trusted; break;
            case FISICS_COVERAGE_UNCHECKED: ++summary.unchecked; break;
            case FISICS_COVERAGE_CONTRADICTORY: ++summary.contradictory; break;
        }
        ++summary.total;
    }
    *out = summary;
    return true;
}
