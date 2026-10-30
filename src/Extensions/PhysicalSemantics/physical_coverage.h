// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/PhysicalSemantics/physical_semantics.h"
#include "Extensions/Units/units_coverage_report.h"
typedef struct {
  bool strict, allow_literals, allow_declared;
} PhysicalCoveragePolicy;
typedef struct {
  PhysicalFact fact;
  unsigned
      trust; /* bit1 literal, bit2 interface; both permissions may be needed. */
  bool control, checked, required;
} PhysicalCoverageRow;
typedef struct {
  FisicsCoverageReport ast;
  PhysicalCoverageRow *rows;
  PhysicalFact *expressions;
  unsigned *trust;
  size_t required, controls, checked, permitted, blocked;
  bool accepted, complete, controls_valid;
  PhysicalCoveragePolicy policy;
} PhysicalCoverage;
bool physical_coverage_collect(struct CompilerContext *, const struct ASTNode *,
                               const char *, PhysicalCoveragePolicy,
                               PhysicalCoverage *);
void physical_coverage_free(PhysicalCoverage *);
