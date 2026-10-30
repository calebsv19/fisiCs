// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/PhysicalSemantics/physical_policy.h"
#include <stdio.h>
struct CompilerContext;
struct ASTNode;
struct ASTAttribute;
struct ParsedType;
struct Symbol;
typedef enum {
  PHYSICAL_UNKNOWN,
  PHYSICAL_KNOWN,
  PHYSICAL_LITERAL_TRUST,
  PHYSICAL_DECLARED_TRUST,
  PHYSICAL_CONTROL,
  PHYSICAL_ERROR
} PhysicalFactState;
typedef struct PhysicalFact {
  FisicsPhysicalType type;
  PhysicalFactState state;
  uint32_t declared_context; /* state-owned declaration/expression context handle */
  unsigned component_depth; /* supported rank2 slices only */
  bool storage_structure; /* aggregate/reference relationship, not quantity */
  bool storage_component; /* validated finite immutable storage relationship */
  bool
      carrier; /* Homogeneous array contents, never a physical pointer value. */
  const struct ASTNode *origin, *loss;
  const char *reason;
  const char *operation_rule; /* Policy-owned identity for an admitted helper. */
} PhysicalFact;
typedef struct PhysicalEntry {
  const struct ASTNode *node;
  size_t slot;
  unsigned domain;
  PhysicalFact fact;
} PhysicalEntry;
typedef struct PhysicalBoundary {
  const struct ASTNode *node;
  const char *operation;
  PhysicalFact expected, actual, result;
} PhysicalBoundary;
typedef struct FisicsPhysicalState {
  FisicsPhysicalPolicy policy;
  struct PhysicalDeclaredContext *declared_contexts;
  size_t declared_context_count, declared_context_capacity;
  PhysicalEntry *entries;
  size_t count, capacity;
  size_t *index;
  size_t index_capacity;
  PhysicalBoundary *boundaries;
  size_t boundary_count, boundary_capacity;
  size_t allocations,
      fail_after; /* Internal deterministic failure test seam. */
  struct PhysicalSourceReceipt *sources;
  size_t source_count;
  char **effective_include_paths;
  size_t effective_include_count;
  bool capture_sources, closure_complete;
  bool failed;
} FisicsPhysicalState;
bool fisics_physical_enabled(const struct CompilerContext *);
bool fisics_physical_prepare(struct CompilerContext *, const char *policy_path);
void fisics_physical_state_free(FisicsPhysicalState *);
void fisics_physical_run(struct ASTNode *, struct CompilerContext *);
void fisics_physical_dump(const struct CompilerContext *, FILE *);
/* Parser hook diagnoses recognized physical attributes with the overlay off. */
void fisics_physical_attribute_seen(struct CompilerContext *,
                                    const struct ASTAttribute *, SourceRange);
void fisics_physical_capture_call(struct CompilerContext *,
                                  const struct ASTNode *, size_t,
                                  const struct ParsedType *);
void fisics_physical_redeclare(struct CompilerContext *, const struct ASTNode *,
                               struct ParsedType *previous,
                               struct ParsedType *current,
                               const struct ASTNode *previous_owner);
void fisics_physical_redeclare_function(struct CompilerContext *,
                                        struct ASTNode *,
                                        struct Symbol *previous);

void fisics_physical_attach_prefix(struct CompilerContext *, struct ASTNode *,
                                   struct ASTAttribute **, size_t);

void fisics_physical_redeclare_variable(struct CompilerContext *,
                                        const struct ASTNode *, struct Symbol *,
                                        struct ParsedType *);
