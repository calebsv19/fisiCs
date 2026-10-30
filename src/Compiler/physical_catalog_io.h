// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Compiler/semantic_project_json.h"
#include "Compiler/semantic_project_manifest.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define FISICS_PHYSICAL_INPUT_MAX_BYTES (64u * 1024u * 1024u)
#define FISICS_PHYSICAL_TOTAL_MAX_BYTES (512u * 1024u * 1024u)
#define FISICS_PHYSICAL_POLICY_MAX_BYTES (1024u * 1024u)
/* Internal options may lower, never raise, production ceilings. Failure
 * injection covers allocations owned by this loader, not
 * JSON/parser/unit-registry internals. */
typedef struct FisicsPhysicalLoadOptions {
  size_t max_file_bytes, max_total_bytes, max_kinds, max_contracts;
  size_t max_catalogs, max_origins;
  size_t max_frame_contexts, max_reference_points, max_frames, max_frame_transforms;
  size_t fail_after; /* SIZE_MAX disables; zero fails first owned allocation. */
} FisicsPhysicalLoadOptions;
typedef enum {
  PHYSICAL_LOAD_OK,
  PHYSICAL_LOAD_ALLOCATION,
  PHYSICAL_LOAD_LIMIT,
  PHYSICAL_LOAD_IO
} FisicsPhysicalLoadFailure;
typedef struct FisicsPhysicalLoadContext {
  FisicsPhysicalLoadOptions options;
  size_t allocations, input_bytes;
  FisicsPhysicalLoadFailure failure;
} FisicsPhysicalLoadContext;
typedef struct FisicsPhysicalInputReceipt {
  uint64_t hash;
  size_t bytes;
} FisicsPhysicalInputReceipt;
FisicsPhysicalLoadOptions fisics_physical_load_defaults(void);
bool fisics_physical_load_options_valid(FisicsPhysicalLoadOptions options);
void physical_load_diag(FisicsSemanticProjectDiagnostic *d, const char *format,
                        ...);
void *physical_alloc(FisicsPhysicalLoadContext *load, size_t bytes);
void *physical_calloc(FisicsPhysicalLoadContext *load, size_t count,
                      size_t width);
void *physical_realloc(FisicsPhysicalLoadContext *load, void *old,
                       size_t bytes);
char *physical_dup_text(FisicsPhysicalLoadContext *load, const char *text);
bool physical_exact_fields(const FisicsJsonValue *value,
                           const char *const *names, size_t count);
bool fisics_physical_read_json(const char *path, size_t maximum,
                               FisicsJsonValue **out,
                               FisicsPhysicalInputReceipt *receipt,
                               FisicsPhysicalLoadContext *load,
                               FisicsSemanticProjectDiagnostic *diagnostic);
