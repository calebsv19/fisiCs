// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Compiler/diagnostics.h"
#include "Compiler/physical_contract_catalog.h"
/* Private compiler model, not a public frontend ABI or expression type checker.
 * IDs are local to this owning model. Zero is unknown, never dimensionless. */
typedef uint32_t FisicsPhysicalKindId;
typedef uint32_t FisicsPhysicalOriginId;
typedef enum {
  FISICS_PHYSICAL_UNPROFILED = 0,
  FISICS_PHYSICAL_LINEAR,
  FISICS_PHYSICAL_ABSOLUTE,
  FISICS_PHYSICAL_DIFFERENCE
} FisicsPhysicalReferenceRole;
typedef struct FisicsPhysicalKindModel {
  FisicsPhysicalReferenceRole role;
  FisicsPhysicalKindId paired_kind;
  uint64_t definition_fingerprint;
} FisicsPhysicalKindModel;
typedef struct FisicsPhysicalOrigin {
  char *id;
  FisicsPhysicalKindId absolute_kind;
} FisicsPhysicalOrigin;
typedef struct FisicsPhysicalCatalogInput {
  char *path;
  FisicsPhysicalInputReceipt receipt;
} FisicsPhysicalCatalogInput;
typedef struct FisicsPhysicalType {
  FisicsPhysicalKindId kind;
  FisicsPhysicalOriginId origin;
  uint32_t representation; /* zero: legacy scalar/array-content contract */
} FisicsPhysicalType;
typedef struct FisicsPhysicalPolicy {
  unsigned schema_version;
  char **energy_forms; size_t energy_form_count;
  struct PhysicalEnergyTransfer *energy_transfers; size_t energy_transfer_count;
  uint64_t energy_fingerprint;
  struct PhysicalFrameContext *frame_contexts; size_t frame_context_count;
  struct PhysicalReferencePoint *reference_points; size_t reference_point_count;
  struct PhysicalFrame *frames; size_t frame_count;
  struct PhysicalFrameTransform *frame_transforms; size_t frame_transform_count;
  uint64_t frame_fingerprint;
  struct FisicsPhysicalRepresentation *representations;
  size_t representation_count;
  struct FisicsPhysicalShapeHelper *shape_helpers;
  size_t shape_helper_count;
  uint64_t representation_fingerprint;
  struct FisicsPhysicalOperationRule *operations;
  size_t operation_count;
  uint64_t operation_fingerprint;
  char *policy_id;
  char *policy_path;
  FisicsPhysicalInputReceipt policy_receipt;
  FisicsPhysicalCatalogInput *catalogs;
  size_t catalog_count;
  FisicsPhysicalQuantityKind *kinds;
  size_t kind_count;
  FisicsPhysicalContract *contracts;
  size_t contract_count;
  FisicsPhysicalKindModel *models;
  FisicsPhysicalOrigin *origins;
  size_t origin_count;
  uint64_t fingerprint, registry_fingerprint;
} FisicsPhysicalPolicy;
typedef struct FisicsPhysicalPolicyDiagnostic {
  FisicsDiagCode code;
  char message[512];
} FisicsPhysicalPolicyDiagnostic;
/* out must be zero-initialized or a live model owned by this API. Success
 * replaces/frees it; EVERY failure leaves it unchanged. No global mutable
 * state. */
bool fisics_physical_policy_load(const char *path, FisicsPhysicalPolicy *out,
                                 FisicsPhysicalPolicyDiagnostic *diagnostic);
bool fisics_physical_policy_load_with_options(
    const char *path, FisicsPhysicalPolicy *out,
    FisicsPhysicalLoadOptions options,
    FisicsPhysicalPolicyDiagnostic *diagnostic);
void fisics_physical_policy_free(FisicsPhysicalPolicy *policy);
FisicsPhysicalKindId
fisics_physical_policy_find_kind(const FisicsPhysicalPolicy *, const char *id);
FisicsPhysicalOriginId
fisics_physical_policy_find_origin(const FisicsPhysicalPolicy *,
                                   const char *id);
/* Validates declared model references only; not source assignment/operation
 * checking. */
bool fisics_physical_policy_make_type(
    const FisicsPhysicalPolicy *, FisicsPhysicalKindId, FisicsPhysicalOriginId,
    FisicsPhysicalType *out, FisicsPhysicalPolicyDiagnostic *diagnostic);
