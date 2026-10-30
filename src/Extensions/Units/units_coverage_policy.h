// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/Units/units_coverage_report.h"
typedef struct {
    bool enabled, allowDeclaredBoundaries, allowLiteralAssumptions;
} FisicsCoveragePolicy;
typedef struct {
    FisicsCoveragePolicy policy;
    size_t required, controls, checked, acceptedTrust, blocked;
    bool accepted;
} FisicsCoverageDecision;
/* No stored report counters are used to decide acceptance. */
FisicsCoverageDecision fisics_units_coverage_decide(const FisicsCoverageReport*,FisicsCoveragePolicy);
/* Narrow syntactic control proof; never promotes a row to checked physics. */
void fisics_units_coverage_mark_loop_controls(FisicsCoverageReport*,const struct ASTNode*,size_t,size_t);
