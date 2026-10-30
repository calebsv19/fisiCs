// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/PhysicalSemantics/physical_policy.h"
#include "Extensions/Units/units_math_semantics.h"
#include <stdio.h>
typedef struct FisicsPhysicalOperationRule {
  char *id;
  FisicsUnitsMathOperation operation;
  FisicsPhysicalKindId kind; /* result; transforming math rules use input keys */
  FisicsPhysicalKindId left, right; /* v3 ordered scalar operands */
} FisicsPhysicalOperationRule;
#define FISICS_PHYSICAL_MAX_OPERATION_RULES 4096u
const FisicsPhysicalOperationRule *
physical_operation_rule(const FisicsPhysicalPolicy *, FisicsUnitsMathOperation,
                        FisicsPhysicalKindId);
bool physical_operations_load(const FisicsJsonValue *, FisicsPhysicalPolicy *,
                              FisicsPhysicalLoadContext *,
                              FisicsSemanticProjectDiagnostic *);
uint64_t physical_operations_fingerprint(const FisicsPhysicalPolicy *);
/* Canonical rule identity and contract; declaration signatures remain
 * unchanged. */
void physical_operations_write(FILE *, const FisicsPhysicalPolicy *);
bool physical_operations_match(const FisicsJsonValue *,
                               const FisicsPhysicalPolicy *);

/* Private operation codes deliberately separate from standard math identity. */
#define PHYSICAL_MULTIPLY ((FisicsUnitsMathOperation)100)
#define PHYSICAL_DIVIDE ((FisicsUnitsMathOperation)101)
#define PHYSICAL_COUPLING_TR ((FisicsUnitsMathOperation)102)
#define PHYSICAL_COUPLING_RT ((FisicsUnitsMathOperation)103)
#define PHYSICAL_TRANSLATION_WORK ((FisicsUnitsMathOperation)104)
#define PHYSICAL_ROTATION_WORK ((FisicsUnitsMathOperation)105)
#define PHYSICAL_LINE_MOMENT ((FisicsUnitsMathOperation)106)
bool physical_cosmic_math_operation(FisicsUnitsMathOperation);
bool physical_cosmic_math_contract(const FisicsPhysicalPolicy *, FisicsUnitsMathOperation, uint32_t, uint32_t, uint32_t);
bool physical_math_unary(FisicsUnitsMathOperation);
const char *physical_operation_origin(FisicsUnitsMathOperation);
bool physical_dynamic_contract(const FisicsPhysicalPolicy *, FisicsUnitsMathOperation,
                               uint32_t, uint32_t, uint32_t);
bool physical_cosmic_contract(const FisicsPhysicalPolicy *, FisicsUnitsMathOperation,
                              uint32_t, uint32_t, uint32_t);
bool physical_thermal_contract(const FisicsPhysicalPolicy *, FisicsUnitsMathOperation,
                               uint32_t, uint32_t, uint32_t);
bool physical_constructor_operation(FisicsUnitsMathOperation);
const FisicsPhysicalOperationRule *physical_constructor_rule(const FisicsPhysicalPolicy *, const char *);
bool physical_structural_contract(const FisicsPhysicalPolicy *, FisicsUnitsMathOperation,
                                 uint32_t, uint32_t, uint32_t);
const char *physical_operation_name(FisicsUnitsMathOperation);
bool physical_scalar_contract(const FisicsPhysicalPolicy *, FisicsUnitsMathOperation,
                              FisicsPhysicalKindId, FisicsPhysicalKindId,
                              FisicsPhysicalKindId);
const FisicsPhysicalOperationRule *physical_scalar_rule(const FisicsPhysicalPolicy *,
    FisicsUnitsMathOperation, FisicsPhysicalKindId, FisicsPhysicalKindId);

/* Called after handle, role and dimensional validation by scalar admission. */
bool physical_mechanical_contract(const FisicsPhysicalPolicy *,
    FisicsUnitsMathOperation, FisicsPhysicalKindId, FisicsPhysicalKindId,
    FisicsPhysicalKindId);
