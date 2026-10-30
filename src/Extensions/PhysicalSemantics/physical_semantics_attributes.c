// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_energy_semantics.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"
#include "Extensions/Units/units_parser.h"
#include "Extensions/extension_hooks.h"
#include "Extensions/extension_units_call_contracts.h"
#include "Syntax/symbol_table.h"
#include "Extensions/PhysicalSemantics/physical_representation_binding.h"
#include "Extensions/PhysicalSemantics/physical_frame_binding.h"
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
static const char *const names[] = {"", "fisics::quantity_kind",
                                    "fisics::physical_origin", "fisics::representation",
                                    "fisics::physical_operation", "fisics::frame", "fisics::component_axis",
                                    "fisics::frame_transform", "fisics::point_id",
                                    "fisics::moment_reference", "fisics::applied_at", "fisics::between_points", "fisics::energy_form", "fisics::energy_transfer"};
static int attribute(const ASTAttribute *a) {
  if (!a || a->syntax != AST_ATTRIBUTE_SYNTAX_CXX || !a->payload)
    return 0;
  for (int k = 1; k <= 13; k++) {
    size_t n = strlen(names[k]);
    if (!strncmp(a->payload, names[k], n) &&
        (!a->payload[n] || a->payload[n] == '(' ||
         isspace((unsigned char)a->payload[n])))
      return k;
  }
  /* Combined attribute lists are not accepted by this declaration contract. */
  bool quoted = false;
  for (const char *p = a->payload; *p; p++) {
    if (*p == '"')
      quoted = !quoted;
    if (!quoted) for(int k=1;k<=13;k++)
      if(!strncmp(p,names[k],strlen(names[k])))return -1;
  }
  return 0;
}
void fisics_physical_attribute_seen(CompilerContext *ctx, const ASTAttribute *a,
                                    SourceRange loc) {
  if (attribute(a) < 0 && fisics_physical_enabled(ctx)) {
    compiler_report_diag(ctx, loc, DIAG_ERROR, 4311,
                         "use one physical attribute per bracket pair",
                         "combined physical attribute lists are unsupported");
    return;
  }
  if (attribute(a) && !fisics_physical_enabled(ctx))
    compiler_report_diag(
        ctx, loc, DIAG_ERROR, 4310,
        "enable --overlay=physics-semantics with an explicit policy",
        "physical quantity/origin annotation requires physics-semantics "
        "overlay");
}
static char *value(CompilerContext *ctx, const ASTAttribute *a, int k,
                   const ASTNode *node, bool diagnose) {
  char mapped[1024];
  const char *detail = NULL;
  char *v = NULL;
  if (strlen(a->payload) >= sizeof(mapped) - 20) {
    if (diagnose)
      physical_error(
          ctx, node, 4311,
          "physical annotation exceeds canonical-name syntax bounds");
    return NULL;
  }
  snprintf(mapped, sizeof(mapped), "fisics::id%s",
           a->payload + strlen(names[k]));
  ASTAttribute proxy = {AST_ATTRIBUTE_SYNTAX_CXX, mapped};
  if (!fisics_semantic_metadata_extract_string(&proxy, FISICS_METADATA_ATTR_ID,
                                               &v, &detail) ||
      !fisics_semantic_metadata_is_canonical_name(v, &detail)) {
    if (detail && !strcmp(detail, "out of memory")) {
      FisicsPhysicalState *s = physical_state(ctx);
      if (s)
        s->failed = true;
      physical_error(ctx, node, 4305, "physical annotation allocation failed");
    } else if (diagnose)
      physical_error(ctx, node, 4311,
                     detail ? detail : "invalid physical annotation");
    free(v);
    return NULL;
  }
  return v;
}
static size_t count(const ParsedType *t, int kind) {
  size_t n = 0;
  if (t)
    for (size_t i = 0; i < t->attributeCount; i++)
      if (attribute(t->attributes[i]) == kind)
        n++;
  return n;
}
PhysicalFact physical_type(CompilerContext *ctx, const ParsedType *t,
                           const ASTNode *node, bool diagnose,
                           bool scalar_result) {
  PhysicalFact f = physical_unknown(node, "missing physical kind");
  FisicsPhysicalState *s = physical_state(ctx);
  if (!t || !s)
    return f;
  size_t nk = count(t, 1), no = count(t, 2), nr = count(t, 3);
  if(count(t,4)) {
    char *id=physical_shape_attribute(t,"fisics::physical_operation");
    bool valid=count(t,4)==1 && s->policy.schema_version>=5 && id && (physical_shape_helper_find(&s->policy,id) || physical_constructor_rule(&s->policy,id));
    free(id);
    if(!valid){if(diagnose)physical_error(ctx,node,4312,"invalid or duplicate physical operation identity");f.state=PHYSICAL_ERROR;return f;}
  }
  if (!nk && !no && !nr) {
    (void)physical_frame_bind(ctx,t,node,&f,diagnose);
    physical_energy_bind(ctx,t,node,&f,diagnose);
    if (diagnose)
      for (size_t i = 0; i < t->attributeCount; i++)
        if (attribute(t->attributes[i]) < 0)
          physical_error(ctx, node, 4311,
                         "combined physical attribute lists are unsupported; "
                         "use separate attributes");
    return f;
  }
  if (nk != 1 || no > 1 || nr > 1) {
    if (diagnose)
      physical_error(
          ctx, node, 4311,
          nk > 1 || no > 1
              ? "duplicate physical kind/origin on one declaration slot"
              : "physical_origin requires quantity_kind");
    f.state = PHYSICAL_ERROR;
    return f;
  }
  char *k = NULL, *o = NULL;
  for (size_t i = 0; i < t->attributeCount; i++) {
    int a = attribute(t->attributes[i]);
    if (a == 1)
      k = value(ctx, t->attributes[i], a, node, diagnose);
    else if (a == 2)
      o = value(ctx, t->attributes[i], a, node, diagnose);
    else if (a < 0 && diagnose)
      physical_error(ctx, node, 4311,
                     "combined physical attributes are unsupported");
  }
  if (!k || (no && !o)) {
    f.state = PHYSICAL_ERROR;
    goto done;
  }
  f.type.kind = fisics_physical_policy_find_kind(&s->policy, k);
  f.type.origin = o ? fisics_physical_policy_find_origin(&s->policy, o) : 0;
  if (!f.type.kind || (o && !f.type.origin)) {
    if (diagnose)
      compiler_report_diag(
          ctx, node ? node->location : (SourceRange){0}, DIAG_ERROR, 4313,
          "declare the canonical identity in the explicit policy",
          "unknown physical %s '%s' in policy %s",
          !f.type.kind ? "kind" : "origin", !f.type.kind ? k : o,
          s->policy.policy_id);
    f.state = PHYSICAL_ERROR;
    goto done;
  }
  FisicsPhysicalPolicyDiagnostic d = {0};
  FisicsPhysicalType checked;
  if (!fisics_physical_policy_make_type(&s->policy, f.type.kind, f.type.origin,
                                        &checked, &d)) {
    if (diagnose)
      physical_error(ctx, node, 4318, d.message);
    f.state = PHYSICAL_ERROR;
    goto done;
  }
  if (nr) {
    char *id = physical_shape_attribute(t, "fisics::representation");
    uint32_t rid = id ? physical_representation_find(&s->policy, id) : 0;
    free(id);
    if (s->policy.schema_version < 5 || !rid ||
        (f.type.origin && !(s->policy.schema_version>=6 && !strcmp(s->policy.kinds[f.type.kind-1].id,"quantity.position"))) ||
        (s->policy.models[f.type.kind-1].role != FISICS_PHYSICAL_LINEAR &&
         s->policy.models[f.type.kind-1].role != FISICS_PHYSICAL_DIFFERENCE &&
         !(s->policy.schema_version>=6 && !strcmp(s->policy.kinds[f.type.kind-1].id,"quantity.position"))) ||
        !physical_representation_bind(ctx,t,node,rid,f.type.kind,f.type.origin,diagnose)) {
      if (diagnose) physical_error(ctx,node,4312,"invalid or unsupported physical representation binding (v5 linear/difference storage required)");
      f.state=PHYSICAL_ERROR; goto done;
    }
    f.type.representation=rid;
  }
  bool array = parsedTypeIsDirectArray(t) ||
               (t->hasParamArrayInfo && t->pointerDepth > 0);
  if (!nr && (t->kind != TYPE_PRIMITIVE || t->primitiveType == TOKEN_VOID ||
      t->isComplex || t->isImaginary || t->isFunctionPointer ||
      (!array && t->pointerDepth > 0))) {
    if (diagnose)
      physical_error(ctx, node, 4312,
                     "physical annotation requires a real scalar, scalar array "
                     "contents, or scalar function result; "
                     "pointer/record/typedef quantities are unsupported");
    f = physical_unknown(node, "unsupported physical carrier");
    goto done;
  }
  (void)scalar_result;
  f.carrier = array && !nr;
  FisicsDim8 dim;
  bool resolved = false, ur = false;
  const FisicsUnitDef *u = NULL;
  fisics_units_parse_type_contract(t, &dim, &resolved, &u, &ur);
  if (!resolved) {
    f.state = PHYSICAL_UNKNOWN;
    f.reason = "missing dimensional contract";
    goto done;
  }
  if (memcmp(dim.e, s->policy.kinds[f.type.kind - 1].dimension, 8)) {
    if (diagnose)
      physical_error(ctx, node, 4314,
                     "physical kind dimensions disagree with the declaration's "
                     "Dim8 contract");
    f.state = PHYSICAL_ERROR;
    goto done;
  }
  f.state = PHYSICAL_KNOWN;
  f.reason = "authored declaration contract";
  f.loss = NULL;
done:
  f.declared_context=physical_frame_bind(ctx,t,node,&f,diagnose);
  physical_frame_components(ctx,t,node,&f,diagnose);
  physical_energy_bind(ctx,t,node,&f,diagnose);
  free(k);
  free(o);
  return f;
}
void physical_validate_type(CompilerContext *ctx, const ParsedType *t,
                            const ASTNode *n, bool unsupported,
                            unsigned depth) {
  if (!t)
    return;
  if (depth > 64) {
    physical_error(ctx, n, 4312,
                   "physical type nesting exceeds supported scope");
    physical_state(ctx)->failed = true;
    return;
  }
  if(count(t,4) && (!n || (n->type!=AST_FUNCTION_DECLARATION && n->type!=AST_FUNCTION_DEFINITION)))
    physical_error(ctx,n,4312,"physical_operation requires a function result declaration");
  bool annotated=false;for(int k=1;k<=13;k++)if(count(t,k))annotated=true;
  if (unsupported && annotated)
    physical_error(
        ctx, n, 4312,
        "physical annotation is unsupported on this typedef/record target");
  else
    (void)physical_type(ctx, t, n, true, false);
  for (size_t i = 0; i < t->fpParamCount; i++)
    physical_validate_type(ctx, &t->fpParams[i], n, false, depth + 1);
  for (size_t i = 0; i < t->derivationCount; i++)
    if (t->derivations[i].kind == TYPE_DERIVATION_FUNCTION)
      for (size_t j = 0; j < t->derivations[i].as.function.paramCount; j++)
        physical_validate_type(ctx, &t->derivations[i].as.function.params[j], n,
                               false, depth + 1);
}
static bool append(CompilerContext *ctx, ParsedType *to, const ASTAttribute *a,
                   const ASTNode *n) {
  ASTAttribute *copy = astAttributeClone(a);
  ASTAttribute **list = malloc(sizeof(*list));
  if (!copy || !list) {
    astAttributeDestroy(copy);
    free(list);
    physical_error(ctx, n, 4305,
                   "physical declaration inheritance allocation failed");
    return false;
  }
  list[0] = copy;
  if (!astAttributeListAppend(&to->attributes, &to->attributeCount, list, 1)) {
    physical_error(ctx, n, 4305,
                   "physical declaration inheritance allocation failed");
    return false;
  }
  return true;
}
static void inherit(CompilerContext *ctx, ParsedType *to,
                    const ParsedType *from, const ASTNode *n) {
  if (!to || !from)
    return;
  bool have[14]={0};for(int k=1;k<=13;k++)have[k]=count(to,k)!=0;
  for(size_t i=0;i<from->attributeCount;i++){
    int a=attribute(from->attributes[i]);
    if(a>0&&!have[a])append(ctx,to,from->attributes[i],n);
  }
}
void physical_function_attributes(CompilerContext *ctx, ASTNode *n) {
  ParsedType *ret = n->type == AST_FUNCTION_DEFINITION
                        ? &n->functionDef.returnType
                        : &n->functionDecl.returnType;
  /* The parser mirrors prefix attributes on both lists; prefer that complete
   * owner list, while preserving duplicate suffix spellings. */
  ParsedType attrs = {.attributes = n->attributes,
                      .attributeCount = n->attributeCount};
  {
    for (int family = 1; family <= 13; family++)
      if (count(&attrs, family) > count(ret, family)) {
        size_t skip = count(ret, family);
        for (size_t i = 0; i < attrs.attributeCount; i++)
          if (attribute(attrs.attributes[i]) == family) {
            if (skip)
              skip--;
            else
              append(ctx, ret, attrs.attributes[i], n);
          }
      }
  }
}
static void redeclare_types(CompilerContext *ctx, const ASTNode *n,
                            ParsedType *previous, ParsedType *current,
                            const ASTNode *owner, unsigned depth) {
  if (depth > 64) {
    physical_error(ctx, n, 4312,
                   "physical signature nesting exceeds supported scope");
    FisicsPhysicalState *state = physical_state(ctx);
    if (state)
      state->failed = true;
    return;
  }
  if (!fisics_physical_enabled(ctx) || !previous || !current)
    return;
  PhysicalFact a = physical_type(ctx, previous, owner, false, false),
               b = physical_type(ctx, current, n, false, false);
  if (a.type.kind && b.type.kind &&
      (a.type.kind != b.type.kind || a.type.origin != b.type.origin || a.type.representation != b.type.representation))
    physical_mismatch(ctx, n, 4315, "redeclaration", a, b);
  for(int family=5;family<=11;family++)if(count(previous,family)&&count(current,family)){
    /* Compare exact argument syntax through the canonical declaration context;
     * two-point ordering is significant. Invalid declarations fail separately. */
    PhysicalFact pa=physical_type(ctx,previous,owner,false,false),pb=physical_type(ctx,current,n,false,false);
    const PhysicalDeclaredContext *ca=physical_declared_context(physical_state(ctx),pa.declared_context),*cb=physical_declared_context(physical_state(ctx),pb.declared_context);
    if(ca&&cb){
      bool conflict=family==5?ca->frame!=cb->frame:family==6?ca->axis!=cb->axis:family==7?ca->transform!=cb->transform:family==8?ca->point_subject!=cb->point_subject:family==9?ca->moment_reference!=cb->moment_reference:family==10?ca->applied_at!=cb->applied_at:ca->displacement_from!=cb->displacement_from||ca->displacement_to!=cb->displacement_to;
      if(conflict)physical_error(ctx,n,4315,"conflicting physical frame/reference/transform redeclaration");
    }
  }
  for(int family=12;family<=13;family++)if(count(previous,family)&&count(current,family)){
    const char *name=names[family];
    char *old=physical_shape_attribute(previous,name),*now=physical_shape_attribute(current,name);
    if(old&&now&&strcmp(old,now))physical_error(ctx,n,4315,"conflicting energy qualifier redeclaration");
    free(old);free(now);
  }
  fisics_units_inherit_return_attributes(current, previous);
  fisics_units_inherit_return_attributes(previous, current);
  char *old_helper=physical_shape_attribute(previous,"fisics::physical_operation"),
       *new_helper=physical_shape_attribute(current,"fisics::physical_operation");
  if(old_helper&&new_helper&&strcmp(old_helper,new_helper))
    physical_error(ctx,n,4315,"conflicting physical operation redeclaration");
  free(old_helper);free(new_helper);
  inherit(ctx, current, previous, n);
  inherit(ctx, previous, current, n);
  const ParsedType *ap = NULL, *bp = NULL;
  size_t an = 0, bn = 0;
  bool av = false, bv = false, ah = false, bh = false;
  if (parsedTypeGetEffectiveFunctionPointerSignature(previous, &ap, &an, &av,
                                                     &ah) &&
      parsedTypeGetEffectiveFunctionPointerSignature(current, &bp, &bn, &bv,
                                                     &bh) &&
      an == bn)
    for (size_t i = 0; i < an; i++)
      redeclare_types(ctx, n, (ParsedType *)&ap[i], (ParsedType *)&bp[i], owner,
                      depth + 1);
}
void fisics_physical_redeclare(CompilerContext *ctx, const ASTNode *n,
                               ParsedType *previous, ParsedType *current,
                               const ASTNode *owner) {
  redeclare_types(ctx, n, previous, current, owner, 0);
}
void fisics_physical_redeclare_function(CompilerContext *ctx, ASTNode *n,
                                        Symbol *prev) {
  if (!fisics_physical_enabled(ctx) || !physical_state(ctx))
    return;
  physical_function_attributes(ctx, n);
  ParsedType *ret = n->type == AST_FUNCTION_DEFINITION
                        ? &n->functionDef.returnType
                        : &n->functionDecl.returnType;
  if (prev && prev->kind == SYMBOL_FUNCTION) {
    fisics_physical_redeclare(ctx, n, &prev->type, ret, prev->definition);
    ASTNode **params = n->type == AST_FUNCTION_DEFINITION
                           ? n->functionDef.parameters
                           : n->functionDecl.parameters;
    size_t count = n->type == AST_FUNCTION_DEFINITION
                       ? n->functionDef.paramCount
                       : n->functionDecl.paramCount;
    size_t slot = 0;
    for (size_t p = 0; p < count; p++) {
      ASTNode *param = params[p];
      if (!param || param->type != AST_VARIABLE_DECLARATION)
        continue;
      for (size_t v = 0; v < param->varDecl.varCount; v++, slot++)
        if (slot < prev->signature.paramCount && prev->signature.params)
          fisics_physical_redeclare(ctx, param, &prev->signature.params[slot],
                                    param->varDecl.declaredTypes
                                        ? &param->varDecl.declaredTypes[v]
                                        : &param->varDecl.declaredType,
                                    prev->definition);
    }
  }
  fisics_physical_capture_call(ctx, n, FISICS_UNITS_RETURN_CONTRACT_INDEX, ret);
}

void fisics_physical_attach_prefix(CompilerContext *ctx, ASTNode *node,
                                   ASTAttribute **attrs, size_t n) {
  if (!fisics_physical_enabled(ctx) || !node)
    return;
  ParsedType *types = NULL;
  size_t slots = 1;
  if (node->type == AST_VARIABLE_DECLARATION) {
    types = node->varDecl.declaredTypes ? node->varDecl.declaredTypes
                                        : &node->varDecl.declaredType;
    slots = node->varDecl.declaredTypes ? node->varDecl.varCount : 1;
  } else if (node->type == AST_FUNCTION_DEFINITION)
    types = &node->functionDef.returnType;
  else if (node->type == AST_FUNCTION_DECLARATION)
    types = &node->functionDecl.returnType;
  else if (node->type == AST_TYPEDEF)
    types = &node->typedefStmt.baseType;
  for (size_t i = 0; i < n; i++) {
    bool selected = attribute(attrs[i]) != 0;
    char *text = NULL;
    if (fisics_units_dim_attribute_extract(attrs[i], &text)) {
      selected = true;
      free(text);
      text = NULL;
    }
    if (fisics_units_unit_attribute_extract(attrs[i], &text)) {
      selected = true;
      free(text);
    }
    if (selected && types)
      for (size_t j = 0; j < slots; j++)
        append(ctx, &types[j], attrs[i], node);
    else if (attribute(attrs[i]) && !types &&
             node->type != AST_STRUCT_DEFINITION &&
             node->type != AST_UNION_DEFINITION)
      physical_error(ctx, node, 4312,
                     "physical attribute cannot attach to this statement");
  }
  if (types && node->type != AST_TYPEDEF)
    fisics_extension_note_units_prefix(ctx, node, attrs, n);
}

/* Variable symbols borrow the declaration type. Reallocating a borrowed
 * attribute list through the symbol would leave the AST with a dangling list.
 * Mutate the actual declaration slot and then rebase the symbol's borrowed
 * view. Function signatures use their separate owned clones. */
void fisics_physical_redeclare_variable(CompilerContext *ctx, const ASTNode *n,
                                        Symbol *symbol, ParsedType *current) {
  if (!fisics_physical_enabled(ctx) || !symbol)
    return;
  ASTNode *owner = symbol->definition;
  ParsedType *previous = &symbol->type;
  if (owner && owner->type == AST_VARIABLE_DECLARATION)
    for (size_t i = 0; i < owner->varDecl.varCount; i++) {
      ASTNode *id = owner->varDecl.varNames ? owner->varDecl.varNames[i] : NULL;
      ParsedType *slot = owner->varDecl.declaredTypes
                             ? &owner->varDecl.declaredTypes[i]
                             : &owner->varDecl.declaredType;
      if (id && id->valueNode.value && symbol->name &&
          !strcmp(id->valueNode.value, symbol->name) &&
          slot->attributes == symbol->type.attributes) {
        previous = slot;
        break;
      }
    }
  fisics_physical_redeclare(ctx, n, previous, current, owner);
  if (previous != &symbol->type) {
    symbol->type.attributes = previous->attributes;
    symbol->type.attributeCount = previous->attributeCount;
  }
}
