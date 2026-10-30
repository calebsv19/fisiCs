// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Compiler/physical_catalog_io.h"
#include "Compiler/semantic_project_manifest.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define FISICS_PHYSICAL_MAX_KINDS 65536u
#define FISICS_PHYSICAL_MAX_CONTRACTS 262144u
typedef struct FisicsPhysicalQuantityKind {
  char *id;
  char *dimension_system;
  int8_t dimension[8];
  char *dimension_text;
  char *catalog_path;
} FisicsPhysicalQuantityKind;

typedef enum FisicsPhysicalSubjectKind {
  FISICS_PHYSICAL_SUBJECT_SEMANTIC_ID = 0,
  FISICS_PHYSICAL_SUBJECT_SOURCE_IDENTITY
} FisicsPhysicalSubjectKind;

typedef enum FisicsPhysicalUnitPolicy {
  FISICS_PHYSICAL_UNIT_DIMENSION_ONLY = 0,
  FISICS_PHYSICAL_UNIT_EXACT,
  FISICS_PHYSICAL_UNIT_CONVERTIBLE
} FisicsPhysicalUnitPolicy;

typedef enum FisicsPhysicalContractState {
  FISICS_PHYSICAL_STATE_SOURCE_COMPATIBLE = 0,
  FISICS_PHYSICAL_STATE_SOURCE_UNVERIFIED,
  FISICS_PHYSICAL_STATE_EXTERNAL_DECLARED
} FisicsPhysicalContractState;

typedef struct FisicsPhysicalContract {
  char *contract_id;
  FisicsPhysicalSubjectKind subject_kind;
  char *subject_id;
  char *quantity_kind;
  FisicsPhysicalUnitPolicy unit_policy;
  char *expected_unit;
  char *value_form;
  size_t component_count;
  bool has_component_count;
  size_t tensor_rank;
  bool has_tensor_rank;
  size_t *tensor_shape;
  size_t tensor_shape_count;
  char *reference_kind;
  char *reference_subject;
  char *period_binary64;
  char *frame_id;
  char *catalog_path;
  FisicsPhysicalContractState state;
  size_t record_index;
  bool has_record_index;
  size_t binding_index;
  bool has_binding_index;
  char *conversion_model;
  char *conversion_scale_binary64;
  char *conversion_offset_binary64;
} FisicsPhysicalContract;

/* Internal reusable catalog v1 API; no graph joins or compiler frontend ABI.
 * Initialize diagnostic to zero. append rolls back counts/new entries on
 * failure, but may have reallocated an array: always retain and free the
 * returned pointers. canonicalize may partially compact on failure; all slots
 * up to the unchanged count remain safe to free. The policy loader wraps these
 * mutations in an owned temporary model to provide atomic replacement of a
 * previously valid model. */
bool fisics_physical_catalog_append(
    const char *path, FisicsPhysicalQuantityKind **kinds, size_t *kind_count,
    FisicsPhysicalContract **contracts, size_t *contract_count,
    FisicsPhysicalLoadContext *load, FisicsPhysicalInputReceipt *receipt,
    FisicsSemanticProjectDiagnostic *diagnostic);
void fisics_physical_quantity_free(FisicsPhysicalQuantityKind *kind);
void fisics_physical_contract_free(FisicsPhysicalContract *contract);
bool fisics_physical_catalog_canonicalize(
    FisicsPhysicalQuantityKind *kinds, size_t *count,
    FisicsSemanticProjectDiagnostic *diagnostic);
const FisicsPhysicalQuantityKind *
fisics_physical_catalog_find(const FisicsPhysicalQuantityKind *kinds,
                             size_t count, const char *id);
/* Pure catalog validation; frame/reference subject existence remains
 * graph-owned. */
bool fisics_physical_catalog_validate(
    const FisicsPhysicalQuantityKind *kinds, size_t kind_count,
    FisicsPhysicalContract *contracts, size_t contract_count,
    FisicsSemanticProjectDiagnostic *diagnostic);

bool fisics_physical_catalog_unique_contracts(
    FisicsPhysicalContract *contracts, size_t count,
    FisicsSemanticProjectDiagnostic *diagnostic);
