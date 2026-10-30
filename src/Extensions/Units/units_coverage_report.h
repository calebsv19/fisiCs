// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/Units/units_coverage_model.h"
#include <stdio.h>

typedef struct {
    const struct ASTNode* node;
    FisicsCoverageFact fact;
    const struct ASTNode* loss;
    const struct ASTNode** dependencies;
    size_t dependencyCount;
} FisicsCoverageExpression;
typedef struct {
    FisicsCoverageAssessment assessment;
    const char* operation;
    bool cInterface;
    const char* controlReason;
    bool scalarControl; /* Proven narrow ordinary loop header; policy exclusion only. */
    size_t expression; /* SIZE_MAX when not an expression result. */
} FisicsCoverageRow;
typedef struct {
    const struct ASTNode* root;
    const struct CompilerContext* ctx;
    const struct ASTNode* function;
    const char* functionName;
    FisicsCoverageExpression* expressions;
    size_t expressionCount, expressionCapacity;
    FisicsCoverageRow* rows;
    size_t rowCount, rowCapacity;
    FisicsCoverageSummary summary;
    bool enumerationComplete, failed;
    bool structuralControls; /* Physical policy only; old units caller stays false. */
    size_t unsupportedNodes;
} FisicsCoverageReport;
/* Exactly one source-defined function; all data is borrowed until destroy/context end. */
bool fisics_units_collect_coverage(const struct CompilerContext* ctx,
    const struct ASTNode* root,const char* function,FisicsCoverageReport* report);
void fisics_units_coverage_destroy(FisicsCoverageReport* report);

/* Same AST keys, annotation-independent C roles for physical policy. */
bool fisics_units_collect_coverage_structural_controls(const struct CompilerContext*,
    const struct ASTNode*,const char*,FisicsCoverageReport*);
