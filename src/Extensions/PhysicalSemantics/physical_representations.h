// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/PhysicalSemantics/physical_policy.h"
#include <stdio.h>
#define PHYSICAL_MAX_REPRESENTATIONS 256u
#define PHYSICAL_MAX_SHAPE_HELPERS 256u
/* Compiler-owned, interned by canonical ID. No runtime representation. */
typedef struct FisicsPhysicalRepresentation {
  char *id;
  bool record, single_precision, axial;
  unsigned rank, extent, field_count;
  char *fields[3]; /* vector x/y/z, or one rank2 array member */
  uint64_t fingerprint;
} FisicsPhysicalRepresentation;
typedef enum {
  PHYSICAL_SHAPE_ADD = 1,
  PHYSICAL_SHAPE_SUB,
  PHYSICAL_SHAPE_SCALE,
  PHYSICAL_SHAPE_DOT,
  PHYSICAL_SHAPE_CROSS,
  PHYSICAL_SHAPE_NORM
} PhysicalShapeOperation;
typedef struct FisicsPhysicalShapeHelper {
  char *id;
  PhysicalShapeOperation operation;
  uint32_t kinds[3], representations[3]; /* left, right, result; 0 = scalar */
  uint64_t fingerprint;
} FisicsPhysicalShapeHelper;
bool physical_representations_load(const FisicsJsonValue *,
                                   FisicsPhysicalPolicy *,
                                   FisicsPhysicalLoadContext *,
                                   FisicsSemanticProjectDiagnostic *);
void physical_representations_free(FisicsPhysicalPolicy *);
uint32_t physical_representation_find(const FisicsPhysicalPolicy *,
                                      const char *);
const FisicsPhysicalShapeHelper *
physical_shape_helper_find(const FisicsPhysicalPolicy *, const char *);
void physical_representations_write(FILE *, const FisicsPhysicalPolicy *);
bool physical_representations_match(const FisicsJsonValue *,
                                    const FisicsPhysicalPolicy *);

/* Schema7 finite binary64 Cosmic vector algebra; no generic Dim8 inference. */
bool physical_cosmic_shape_contract(const FisicsPhysicalPolicy *, const FisicsPhysicalShapeHelper *);
bool physical_cosmic_vector_kind(const FisicsPhysicalPolicy *, uint32_t, bool);
