// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_frame_binding.h"
#include <string.h>
uint32_t physical_frame_intern(CompilerContext *c, const ASTNode *n,
                               PhysicalDeclaredContext v, PhysicalFact *fact) {
  FisicsPhysicalState *s = physical_state(c);
  if (s->failed) {
    fact->state = PHYSICAL_ERROR;
    return 0;
  }
  for (size_t i = 0; i < s->declared_context_count; i++)
    if (!memcmp(&v, &s->declared_contexts[i], sizeof(v)))
      return (uint32_t)i + 1;
  if (s->declared_context_count >= PHYSICAL_MAX_DECLARED_CONTEXTS) {
    s->failed = true;
    physical_error(c, n, 4304, "declared physical context limit exceeded");
    fact->state = PHYSICAL_ERROR;
    return 0;
  }
  if (s->declared_context_count == s->declared_context_capacity) {
    size_t cap =
        s->declared_context_capacity ? s->declared_context_capacity * 2 : 16;
    PhysicalDeclaredContext *next =
        physical_state_grow(c, s->declared_contexts, cap * sizeof(*next));
    if (!next) {
      fact->state = PHYSICAL_ERROR;
      return 0;
    }
    s->declared_contexts = next;
    s->declared_context_capacity = cap;
  }
  s->declared_contexts[s->declared_context_count++] = v;
  return (uint32_t)s->declared_context_count;
}
