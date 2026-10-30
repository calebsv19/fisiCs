// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/Units/units_coverage_report.h"
void fisics_units_coverage_mark_predicate(FisicsCoverageReport*,const struct ASTNode*,size_t);
void fisics_units_coverage_mark_status_return(FisicsCoverageReport*);
void fisics_units_coverage_mark_c_interface(FisicsCoverageReport*,const struct ASTNode*,size_t);
