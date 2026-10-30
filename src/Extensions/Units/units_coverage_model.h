// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "Extensions/Units/units_registry.h"
#include "Lexer/source_range.h"
struct ASTNode;
struct CompilerContext;

/* Internal P3.1 contract. All AST, source and registry references are borrowed
 * for one frontend lifetime. No public API/schema, persistence or diagnostics. */
typedef enum {
    FISICS_COVERAGE_UNKNOWN, FISICS_COVERAGE_DECLARED, FISICS_COVERAGE_INFERRED
} FisicsCoverageOrigin;
typedef enum {
    FISICS_COVERAGE_NONE, FISICS_COVERAGE_OVERLAY_DISABLED,
    FISICS_COVERAGE_MISSING_ANNOTATION, FISICS_COVERAGE_UNANNOTATED_STORAGE,
    FISICS_COVERAGE_UNCONTRACTED_CALL, FISICS_COVERAGE_UNRESOLVED_ANNOTATION,
    FISICS_COVERAGE_NO_INFERENCE, FISICS_COVERAGE_UNTRACKED_PROVENANCE,
    FISICS_COVERAGE_DECLARED_INTERFACE, FISICS_COVERAGE_ASSUMED_REPRESENTATION,
    FISICS_COVERAGE_DIMENSION_MISMATCH,
    FISICS_COVERAGE_UNIT_MISMATCH, FISICS_COVERAGE_MISSING_UNIT
} FisicsCoverageReason;
typedef struct {
    const struct ASTNode *site, *originNode;
    SourceRange location, originLocation;
    FisicsCoverageOrigin origin;
    FisicsCoverageReason reason;
    bool hasDimension, hasUnit, provenanceComplete, trusted;
    FisicsDim8 dimension; /* Meaningful only when hasDimension is true. */
    const FisicsUnitDef* unit;
} FisicsCoverageFact;
typedef enum {
    FISICS_COVERAGE_CHECKED, FISICS_COVERAGE_TRUSTED_BOUNDARY,
    FISICS_COVERAGE_UNCHECKED, FISICS_COVERAGE_CONTRADICTORY
} FisicsCoverageState;
typedef enum {
    FISICS_COVERAGE_INITIALIZER, FISICS_COVERAGE_ASSIGNMENT,
    FISICS_COVERAGE_ARGUMENT, FISICS_COVERAGE_RETURN, FISICS_COVERAGE_OPERATION
} FisicsCoverageBoundary;
typedef struct {
    const struct ASTNode* site;
    FisicsCoverageBoundary boundary;
    size_t slot; /* Declarator/argument index, as applicable. */
    bool requireConcreteUnit;
    FisicsCoverageFact expected, actual;
} FisicsCoverageObligation;
typedef struct {
    FisicsCoverageObligation obligation;
    FisicsCoverageState state;
    FisicsCoverageReason reason;
    const struct ASTNode* lossNode;
    bool dimensionCompared, unitCompared;
} FisicsCoverageAssessment;
typedef struct {
    size_t total, checked, trusted, unchecked, contradictory;
} FisicsCoverageSummary;

FisicsCoverageFact fisics_coverage_declaration(const struct CompilerContext* ctx,
    const struct ASTNode* declaration, size_t declaratorIndex);
FisicsCoverageFact fisics_coverage_expression(const struct CompilerContext* ctx,
    const struct ASTNode* expression);
/* Performs only this selected boundary comparison. It does not assert that an
 * earlier semantic pass checked the operation or the producing implementation. */
FisicsCoverageAssessment fisics_coverage_assess(FisicsCoverageObligation obligation);
/* Finite explicit caller-selected denominator; duplicate (site,boundary,slot)
 * keys, invalid sites and invalid kinds fail atomically. Empty is not 100%. */
bool fisics_coverage_summarize(const FisicsCoverageAssessment* rows, size_t count,
    FisicsCoverageSummary* out);
