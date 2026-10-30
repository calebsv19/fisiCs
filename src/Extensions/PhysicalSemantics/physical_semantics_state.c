// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_energy_semantics.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "Extensions/PhysicalSemantics/physical_provenance.h"
#include "Extensions/extension_hooks.h"
#include "Extensions/extension_units_call_contracts.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/PhysicalSemantics/physical_frame_binding.h"
#include "Extensions/PhysicalSemantics/physical_frame_semantics.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
bool fisics_physical_enabled(const CompilerContext *ctx) {
  return cc_has_overlay_feature(ctx, FISICS_OVERLAY_PHYSICS_SEMANTICS);
}
FisicsPhysicalState *physical_state(const CompilerContext *ctx) {
  return ctx && ctx->extensionState ? ctx->extensionState->physicalState : NULL;
}
PhysicalFact physical_unknown(const ASTNode *node, const char *reason) {
  return (PhysicalFact){.state = PHYSICAL_UNKNOWN,
                        .origin = node,
                        .loss = node,
                        .reason = reason};
}
bool physical_known(PhysicalFact f) {
  return f.type.kind &&
         (f.state == PHYSICAL_KNOWN || f.state == PHYSICAL_LITERAL_TRUST ||
          f.state == PHYSICAL_DECLARED_TRUST);
}
void physical_error(CompilerContext *ctx, const ASTNode *node, int code,
                    const char *message) {
  SourceRange loc = node ? node->location : (SourceRange){0};
  compiler_report_diag(
      ctx, loc, DIAG_ERROR, code,
      "physical kinds are independent of dimensional compatibility", "%s",
      message);
}
static const char *kind(FisicsPhysicalState *s, uint32_t k) {
  return k && k <= s->policy.kind_count ? s->policy.kinds[k - 1].id
                                        : "<unknown>";
}
static const char *origin(FisicsPhysicalState *s, uint32_t o) {
  return o && o <= s->policy.origin_count ? s->policy.origins[o - 1].id
                                          : "<none>";
}
static const char *file(const ASTNode *n) {
  return n && n->location.start.file ? n->location.start.file : "<unavailable>";
}
void physical_mismatch(CompilerContext *ctx, const ASTNode *node, int code,
                       const char *op, PhysicalFact a, PhysicalFact b) {
  FisicsPhysicalState *s = physical_state(ctx);
  if (!s)
    return;
  SourceRange loc = node ? node->location : (SourceRange){0};
  if(a.type.representation != b.type.representation) {
    compiler_report_diag(ctx,loc,DIAG_ERROR,code,"use an explicit C/physical representation adapter",
      "physical %s representation mismatch: expected handle %u, actual handle %u (policy-local, see representation model)",op,a.type.representation,b.type.representation);
    return;
  }
  compiler_report_diag(
      ctx, loc, DIAG_ERROR, code,
      "use a declared physical transformation; a C cast does not relabel a "
      "quantity",
      "physical %s mismatch: expected %s (role=%u, origin=%s) from %s:%d; "
      "actual %s (role=%u, origin=%s) from %s:%d",
      op, kind(s, a.type.kind),
      a.type.kind ? (unsigned)s->policy.models[a.type.kind - 1].role : 0,
      origin(s, a.type.origin), file(a.origin), a.origin ? a.origin->line : 0,
      kind(s, b.type.kind),
      b.type.kind ? (unsigned)s->policy.models[b.type.kind - 1].role : 0,
      origin(s, b.type.origin), file(b.origin), b.origin ? b.origin->line : 0);
}
void fisics_physical_state_free(FisicsPhysicalState *s) {
  if (!s)
    return;
  physical_sources_free(s);
  fisics_physical_policy_free(&s->policy);
  free(s->declared_contexts);
  free(s->entries);
  free(s->index);
  free(s->boundaries);
  free(s);
}
bool fisics_physical_prepare(CompilerContext *ctx, const char *path) {
  if (!fisics_physical_enabled(ctx)) {
    if (path) {
      physical_error(ctx, NULL, 4301,
                     "physical policy requires --overlay=physics-semantics");
      return false;
    }
    return true;
  }
  if (!path) {
    physical_error(ctx, NULL, 4301,
                   "physics-semantics requires --physical-semantics-policy");
    return false;
  }
  if (!fisics_extension_state_ensure(ctx)) {
    physical_error(ctx, NULL, 4305, "physical state allocation failed");
    return false;
  }
  FisicsPhysicalState *s = calloc(1, sizeof(*s));
  if (!s) {
    physical_error(ctx, NULL, 4305, "physical state allocation failed");
    return false;
  }
  s->fail_after = SIZE_MAX;
  FisicsPhysicalPolicyDiagnostic d = {0};
  if (!fisics_physical_policy_load(path, &s->policy, &d)) {
    physical_error(ctx, NULL, d.code, d.message);
    free(s);
    return false;
  }
  fisics_physical_state_free(ctx->extensionState->physicalState);
  ctx->extensionState->physicalState = s;
  return true;
}
void *physical_state_grow(CompilerContext *ctx, void *old, size_t bytes) {
  FisicsPhysicalState *s = physical_state(ctx);
  if (s->failed)
    return NULL;
  void *p = s->allocations++ >= s->fail_after ? NULL : realloc(old, bytes);
  if (!p) {
    s->failed = true;
    physical_error(ctx, NULL, 4305,
                   "physical facts allocation failed; analysis is incomplete");
  }
  return p;
}
static size_t hash(const ASTNode *n, unsigned domain, size_t slot) {
  return (((uintptr_t)n >> 4) * UINT64_C(11400714819323198485)) ^
         (slot * 33u + domain);
}
static size_t find(FisicsPhysicalState *s, const ASTNode *n, unsigned domain,
                   size_t slot) {
  if (!s || !s->index_capacity)
    return SIZE_MAX;
  size_t p = hash(n, domain, slot) & (s->index_capacity - 1);
  while (s->index[p]) {
    size_t i = s->index[p] - 1;
    PhysicalEntry *e = &s->entries[i];
    if (e->node == n && e->domain == domain && e->slot == slot)
      return i;
    p = (p + 1) & (s->index_capacity - 1);
  }
  return SIZE_MAX;
}
bool physical_put(CompilerContext *ctx, const ASTNode *n, unsigned domain,
                  size_t slot, PhysicalFact f) {
  FisicsPhysicalState *s = physical_state(ctx);
  if (!s || !n || s->failed)
    return false;
  size_t i = find(s, n, domain, slot);
  if (i != SIZE_MAX) {
    s->entries[i].fact = f;
    return true;
  }
  if (s->count >= PHYSICAL_MAX_FACTS) {
    s->failed = true;
    physical_error(ctx, n, 4304,
                   "physical fact limit exceeded; analysis is incomplete");
    return false;
  }
  if ((s->count + 1) * 2 > s->index_capacity) {
    size_t cap = s->index_capacity ? s->index_capacity * 2 : 32;
    size_t *index = physical_state_grow(ctx, NULL, cap * sizeof(*index));
    if (!index)
      return false;
    memset(index, 0, cap * sizeof(*index));
    for (size_t k = 0; k < s->count; k++) {
      PhysicalEntry *e = &s->entries[k];
      size_t p = hash(e->node, e->domain, e->slot) & (cap - 1);
      while (index[p])
        p = (p + 1) & (cap - 1);
      index[p] = k + 1;
    }
    free(s->index);
    s->index = index;
    s->index_capacity = cap;
  }
  if (s->count == s->capacity) {
    size_t cap = s->capacity ? s->capacity * 2 : 16;
    PhysicalEntry *p = physical_state_grow(ctx, s->entries, cap * sizeof(*p));
    if (!p)
      return false;
    s->entries = p;
    s->capacity = cap;
  }
  i = s->count++;
  s->entries[i] = (PhysicalEntry){n, slot, domain, f};
  size_t p = hash(n, domain, slot) & (s->index_capacity - 1);
  while (s->index[p])
    p = (p + 1) & (s->index_capacity - 1);
  s->index[p] = i + 1;
  return true;
}
PhysicalFact physical_get(const CompilerContext *ctx, const ASTNode *n,
                          unsigned domain, size_t slot) {
  FisicsPhysicalState *s = physical_state(ctx);
  size_t i = find(s, n, domain, slot);
  return i == SIZE_MAX ? physical_unknown(n, "missing physical metadata")
                       : s->entries[i].fact;
}
void physical_event(CompilerContext *ctx, const ASTNode *n, const char *op,
                    PhysicalFact a, PhysicalFact b, PhysicalFact result) {
  FisicsPhysicalState *s = physical_state(ctx);
  if (!s || s->failed)
    return;
  if (s->boundary_count >= PHYSICAL_MAX_FACTS) {
    s->failed = true;
    physical_error(ctx, n, 4304,
                   "physical boundary limit exceeded; analysis is incomplete");
    return;
  }
  if (s->boundary_count == s->boundary_capacity) {
    size_t cap = s->boundary_capacity ? s->boundary_capacity * 2 : 16;
    PhysicalBoundary *p = physical_state_grow(ctx, s->boundaries, cap * sizeof(*p));
    if (!p)
      return;
    s->boundaries = p;
    s->boundary_capacity = cap;
  }
  s->boundaries[s->boundary_count++] = (PhysicalBoundary){n, op, a, b, result};
}
PhysicalFact physical_boundary(CompilerContext *ctx, const ASTNode *n,
                               const char *op, PhysicalFact a, PhysicalFact b,
                               bool literal) {
  PhysicalFact f = b;
  if (a.state == PHYSICAL_ERROR || b.state == PHYSICAL_ERROR)
    f.state = PHYSICAL_ERROR;
  else if (physical_known(a) && literal) {
    f = a;
    f.state = PHYSICAL_LITERAL_TRUST;
    f.reason = "authored literal representation";
    f.loss = n;
  } else if (a.type.kind && b.type.kind &&
             (a.type.kind != b.type.kind || a.type.origin != b.type.origin || a.type.representation != b.type.representation)) {
    physical_mismatch(ctx, n, a.type.kind != b.type.kind ? 4316 : 4318, op, a,
                      b);
    f.state = PHYSICAL_ERROR;
    f.reason = "contradictory physical boundary";
  } else if (physical_known(a) && physical_known(b)) {
    if (a.type.kind != b.type.kind || a.type.origin != b.type.origin || a.type.representation != b.type.representation) {
      physical_mismatch(ctx, n, a.type.kind != b.type.kind ? 4316 : 4318, op, a,
                        b);
      f.state = PHYSICAL_ERROR;
      f.reason = "contradictory physical boundary";
    }
  } else {
    f = physical_unknown(n, "unverified physical boundary");
    if (b.loss)
      f.loss = b.loss;
  }
  f=physical_frame_boundary(ctx,n,op,a,b,f,literal);
  f=physical_energy_boundary(ctx,n,a,b,f,literal);
  physical_event(ctx, n, op, a, b, f);
  return f;
}
void fisics_physical_capture_call(CompilerContext *ctx, const ASTNode *n,
                                  size_t slot, const ParsedType *type) {
  if (!fisics_physical_enabled(ctx) || !physical_state(ctx))
    return;
  const ASTNode *owner = n;
  if (n && n->type == AST_FUNCTION_CALL) {
    owner = NULL;
    ASTNode *fn = fisics_extension_lookup_units_function_binding(
        ctx, n->functionCall.callee);
    if (fn && (fn->type == AST_FUNCTION_DEFINITION ||
               fn->type == AST_FUNCTION_DECLARATION)) {
      if (slot == FISICS_UNITS_RETURN_CONTRACT_INDEX)
        owner = fn;
      else {
        ASTNode **params = fn->type == AST_FUNCTION_DEFINITION
                               ? fn->functionDef.parameters
                               : fn->functionDecl.parameters;
        size_t count = fn->type == AST_FUNCTION_DEFINITION
                           ? fn->functionDef.paramCount
                           : fn->functionDecl.paramCount;
        size_t index = 0;
        for (size_t i = 0; i < count; i++)
          if (params[i] && params[i]->type == AST_VARIABLE_DECLARATION)
            for (size_t j = 0; j < params[i]->varDecl.varCount; j++, index++)
              if (index == slot)
                owner = params[i]->varDecl.varNames
                            ? params[i]->varDecl.varNames[j]
                            : params[i];
      }
    }
  }
  PhysicalFact fact = physical_type(ctx, type,
      physical_state(ctx)->policy.schema_version>=6 && owner?owner:n, false, true);
  fact.origin = owner;
  physical_put(ctx, n, PHYSICAL_CALL, slot, fact);
}
void fisics_physical_dump(const CompilerContext *ctx, FILE *out) {
  FisicsPhysicalState *s = physical_state(ctx);
  if (!s || !out)
    return;
  fprintf(out,
          "Physical Semantics: policy=%s; units dependency=enabled; "
          "storage-complete=%s\n",
          s->policy.policy_id, s->failed ? "false" : "true");
  if(s->policy.schema_version>=6){
    fputs("Frame model (static propagation; numeric geometry unassessed): ",out);
    physical_frames_write(out,&s->policy);fputc('\n',out);
    fprintf(out,"Interned declaration contexts: %zu\n",s->declared_context_count);
  }
  fprintf(out, "Physical Facts: %zu\n", s->count);
  const char *states[] = {"unknown",        "known",   "literal-trust",
                          "declared-trust", "control", "contradictory"};
  for (size_t i = 0; i < s->count; i++) {
    const PhysicalEntry *e = &s->entries[i];
    const PhysicalFact *f = &e->fact;
    fprintf(out,
            "  - line %d: domain=%u slot=%zu kind=%s origin=%s state=%s "
            "reason=%s loss-line=%d\n",
            e->node->line, e->domain, e->slot, kind(s, f->type.kind),
            origin(s, f->type.origin), states[f->state],
            f->reason ? f->reason : "none", f->loss ? f->loss->line : 0);
    if(s->policy.schema_version>=6 && f->declared_context){
      PhysicalDeclaredContext v=physical_frame_context((CompilerContext*)ctx,*f);
      fprintf(out,"    context: frame=%s axis=%s compatibility=%s invariant=%s applied-at=%s between=%s->%s moment=%s\n",
        v.frame?s->policy.frames[v.frame-1].id:"<none>",v.axis==1?"axis.x":v.axis==2?"axis.y":v.axis==3?"axis.z":"<none>",
        v.flags&PHYSICAL_FRAME_CHECKED?"checked":"unassessed",v.flags&PHYSICAL_FRAME_INVARIANT?"true":"false",
        v.applied_at?s->policy.reference_points[v.applied_at-1].id:"<unknown>",
        v.displacement_from?s->policy.reference_points[v.displacement_from-1].id:"<unknown>",
        v.displacement_to?s->policy.reference_points[v.displacement_to-1].id:"<unknown>",
        v.moment_reference?s->policy.reference_points[v.moment_reference-1].id:"<unknown>");
    }
  }
  if(s->policy.schema_version>=6)for(size_t i=0;i<s->count;i++){
    const PhysicalEntry *e=&s->entries[i];if(e->domain!=PHYSICAL_DECL)continue;
    const PhysicalDeclaredContext *v=physical_declared_context(s,e->fact.declared_context);if(!v || !v->frame)continue;
    fprintf(out,"  declaration line=%d slot=%zu frame=%s axis=%u context-handle=%u (authored only)\n",e->node->line,e->slot,s->policy.frames[v->frame-1].id,v->axis,e->fact.declared_context);
  }
  fprintf(out, "Physical Boundaries: %zu\n", s->boundary_count);
  for (size_t i = 0; i < s->boundary_count; i++) {
    const PhysicalBoundary *b = &s->boundaries[i];
    fprintf(out, "  - line %d: %s expected=%s actual=%s state=%s reason=%s\n",
            b->node ? b->node->line : 0, b->operation,
            kind(s, b->expected.type.kind), kind(s, b->actual.type.kind),
            states[b->result.state],
            b->result.reason ? b->result.reason : "none");
  }
}
