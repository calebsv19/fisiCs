// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_representation_binding.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"
#include "Extensions/Units/units_literal_semantics.h"
#include "Extensions/extension_units_call_contracts.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/PhysicalSemantics/physical_frame_semantics.h"
#include <stdlib.h>
#include <string.h>
/* Only fixed direct storage is admitted. Array parameter syntax has already
 * decayed and is deliberately rejected, including a written static bound. */
static bool scalar(const ParsedType *t, bool single) {
  return t && t->kind == TYPE_PRIMITIVE &&
         t->primitiveType == (single ? TOKEN_FLOAT : TOKEN_DOUBLE) &&
         !t->isLong && !t->isShort && !t->isSigned && !t->isUnsigned &&
         !t->isComplex && !t->isImaginary && !t->isFunctionPointer;
}
static bool array(const ParsedType *t, unsigned rank, unsigned extent,
                  bool single) {
  if (!scalar(t, single) || t->hasParamArrayInfo || t->isVLA ||
      t->derivationCount != rank || t->pointerDepth != 0)
    return false;
  for (unsigned i = 0; i < rank; i++) {
    const TypeDerivation *a = &t->derivations[i];
    if (a->kind != TYPE_DERIVATION_ARRAY || a->as.array.isVLA ||
        a->as.array.isFlexible || !a->as.array.hasConstantSize ||
        a->as.array.constantSize != (long long)extent)
      return false;
  }
  return true;
}
static const ASTNode *definition(CompilerContext *c, const ParsedType *t) {
  if (!t)
    return NULL;
  const ASTNode *n = t->inlineStructOrUnionDef;
  if (!n && t->kind == TYPE_STRUCT && t->userTypeName)
    n = cc_tag_definition(c, CC_TAG_STRUCT, t->userTypeName);
  return n && n->type == AST_STRUCT_DEFINITION ? n : NULL;
}
static const ParsedType *field(const ASTNode *d, unsigned index,
                               const ASTNode **owner, const char **name) {
  unsigned ordinal = 0;
  for (size_t i = 0; i < d->structDef.fieldCount; i++) {
    const ASTNode *f = d->structDef.fields[i];
    if (!f || f->type != AST_VARIABLE_DECLARATION)
      continue;
    for (size_t k = 0; k < f->varDecl.varCount; k++, ordinal++)
      if (ordinal == index) {
        const ASTNode *id = f->varDecl.varNames[k];
        *owner = id;
        *name = id ? id->valueNode.value : NULL;
        return astVarDeclTypeAt(f, k);
      }
  }
  return NULL;
}
bool physical_representation_bind(CompilerContext *c, const ParsedType *t,
                                  const ASTNode *n, uint32_t id,
                                  uint32_t expected_kind, uint32_t expected_origin, bool diagnose) {
  const FisicsPhysicalPolicy *p = &physical_state(c)->policy;
  if (!id || id > p->representation_count)
    return false;
  const FisicsPhysicalRepresentation *r = &p->representations[id - 1];
  const char *reason = NULL;
  const char *kind_name = p->kinds[expected_kind - 1].id;
  if (r->rank == 1 &&
      (((!strcmp(kind_name, "quantity.torque") || physical_cosmic_vector_kind(p,expected_kind,true)) && !r->axial) ||
       (r->axial && (!strcmp(kind_name, "quantity.force") ||
                     !strcmp(kind_name, "quantity.displacement") ||
                     !strcmp(kind_name, "quantity.velocity") ||
                     !strcmp(kind_name, "quantity.acceleration"))))) {
    if (diagnose)
      physical_error(
          c, n, 4312,
          "representation parity contradicts canonical vector quantity kind");
    return false;
  }
  if (t && t->hasAlignOverride) {
    if (diagnose)
      physical_error(c, n, 4312,
                     "aligned representation adapters are unsupported");
    return false;
  }
  if (!strcmp(kind_name,"quantity.position") &&
      (p->schema_version<6 || !r->record || r->single_precision ||
       r->rank!=1 || r->extent!=3 || r->axial)) {
    if(diagnose) physical_error(c,n,4312,"position requires v6 3D double polar record storage");
    return false;
  }
  unsigned index = 0;
  if (!r->record) {
    if (!array(t, r->rank, r->extent, r->single_precision))
      reason =
          "expected fixed direct array with exact rank, extent and precision";
  } else {
    const ASTNode *d = definition(c, t);
    if (!t || t->kind != TYPE_STRUCT || t->pointerDepth || t->derivationCount ||
        t->isFunctionPointer || !d)
      reason = "expected complete non-pointer C struct (resolved record "
               "aliases allowed)";
    else if (d->attributeCount) {
      reason = "attributed record layout is unsupported";
    } else {
      for (size_t i = 0; i < d->structDef.fieldCount; i++) {
        const ASTNode *entry = d->structDef.fields[i];
        if (!entry || entry->type != AST_VARIABLE_DECLARATION ||
            !entry->varDecl.varCount || entry->varDecl.bitFieldWidth) {
          reason =
              "anonymous, bit-field or non-field record storage is unsupported";
          break;
        }
        for (size_t j = 0; j < entry->varDecl.varCount; j++) {
          const ParsedType *ft = astVarDeclTypeAt(entry, j);
          for (size_t k = 0; ft && k < ft->attributeCount; k++) {
            const ASTAttribute *at = ft->attributes[k];
            if (!at || at->syntax != AST_ATTRIBUTE_SYNTAX_CXX || !at->payload ||
                strncmp(at->payload, "fisics::", 8))
              reason = "attributed component layout is unsupported";
          }
        }
      }
      for (index = 0; !reason && index < r->field_count; index++) {
        const ASTNode *owner = NULL;
        const char *name = NULL;
        const ParsedType *ft = field(d, index, &owner, &name);
        if (!ft || !name || strcmp(name, r->fields[index]) ||
            ft->hasAlignOverride ||
            (r->rank == 1 ? (!scalar(ft, r->single_precision) ||
                             ft->pointerDepth || ft->derivationCount)
                          : !array(ft, 2, r->extent, r->single_precision))) {
          reason =
              "component field order/type/extent disagrees with representation";
          break;
        }
        /* Independent field annotations may refine but cannot contradict the
         * enclosing homogeneous quantity; checked below by physical_type. */
        PhysicalFact component = physical_type(c, ft, owner, false, true);
        FisicsDim8 dim = {0};
        bool resolved = false, ur = false;
        const FisicsUnitDef *u = NULL;
        fisics_units_parse_type_contract(ft, &dim, &resolved, &u, &ur);
        if ((component.type.kind &&
             (component.type.kind != expected_kind || component.type.origin != expected_origin)) ||
            (resolved &&
             memcmp(dim.e, p->kinds[expected_kind - 1].dimension, 8)) ||
            component.state == PHYSICAL_ERROR ||
            component.type.representation) {
          reason = "nested or invalid component physical contract";
          break;
        }
      }
      const ASTNode *owner = NULL;
      const char *name = NULL;
      if (!reason && field(d, r->field_count, &owner, &name))
        reason =
            "extra record field: heterogeneous records are not shape carriers";
    }
  }
  if (reason && diagnose)
    compiler_report_diag(
        c, n ? n->location : (SourceRange){0}, DIAG_ERROR, 4312,
        "use an explicit supported representation adapter",
        "representation %s component %u (%s): %s", r->id, index,
        index < r->field_count ? r->fields[index] : "array", reason);
  return !reason;
}
char *physical_shape_attribute(const ParsedType *t, const char *name) {
  char *result = NULL;
  if (!t)
    return NULL;
  size_t len = strlen(name);
  for (size_t i = 0; i < t->attributeCount; i++) {
    const ASTAttribute *a = t->attributes[i];
    if (!a || !a->payload || strncmp(a->payload, name, len) ||
        a->payload[len] != '(')
      continue;
    char mapped[1024];
    if (strlen(a->payload) >= sizeof(mapped) - 20) {
      free(result);
      return NULL;
    }
    snprintf(mapped, sizeof(mapped), "fisics::id%s", a->payload + len);
    ASTAttribute proxy = {AST_ATTRIBUTE_SYNTAX_CXX, mapped};
    char *v = NULL;
    const char *detail = NULL;
    if (result ||
        !fisics_semantic_metadata_extract_string(
            &proxy, FISICS_METADATA_ATTR_ID, &v, &detail) ||
        !fisics_semantic_metadata_is_canonical_name(v, &detail)) {
      free(v);
      free(result);
      return NULL;
    }
    result = v;
  }
  return result;
}
bool physical_representation_access(CompilerContext *c, ASTNode *n,
                                    PhysicalFact *out) {
  ASTNode *base = n->type == AST_ARRAY_ACCESS ? n->arrayAccess.array
                  : n->type == AST_DOT_ACCESS ? n->memberAccess.base
                                              : NULL;
  if (!base)
    return false;
  PhysicalFact b = physical_get(c, base, PHYSICAL_EXPR, 0);
  if (!physical_known(b) || !b.type.representation)
    return false;
  const FisicsPhysicalRepresentation *r =
      &physical_state(c)->policy.representations[b.type.representation - 1];
  unsigned axis=0;
  if (n->type == AST_DOT_ACCESS) {
    if (!r->record)
      return false;
    bool found = false;
    for (unsigned i = 0; i < r->field_count; i++)
      if (!strcmp(r->fields[i], n->memberAccess.field))
        {found = true;axis=i+1;}
    if (!found)
      return false;
    if (r->rank == 2) {
      b.component_depth = 0;
      b.carrier = true;
    } else {
      b.type.representation = 0;
      b.carrier = false;
    }
  } else {
    if (r->record && !b.carrier)
      return false;
    b.component_depth++;
    if (b.component_depth == r->rank) {
      b.type.representation = 0;
      b.carrier = false;
      b.component_depth = 0;
    } else
      b.carrier = true;
  }
  if(physical_state(c)->policy.schema_version>=6 && r->rank==1){
    if(n->type==AST_ARRAY_ACCESS){
      const ASTNode *ix=n->arrayAccess.index;
      if(ix&&ix->type==AST_NUMBER_LITERAL&&ix->valueNode.value){
        char *end=NULL;unsigned long v=strtoul(ix->valueNode.value,&end,0);
        if(end&&!*end&&v<r->extent)axis=(unsigned)v+1;
      }
    }
    b=physical_frame_project(c,n,b,axis);
  }
  b.reason = b.state==PHYSICAL_UNKNOWN?b.reason:
      b.type.representation
          ? "validated homogeneous array slice; no geometric reinterpretation"
          : "validated carrier scalar component";
  *out = b;
  return true;
}
static PhysicalFact initializer(CompilerContext *c, ASTNode *owner, ASTNode *v,
                                PhysicalFact expected, unsigned rank,
                                unsigned extent, unsigned depth) {
  if (!v || depth > 3)
    return physical_unknown(v, "unsupported shape initializer");
  if (!rank) {
    return physical_boundary(c, v, "initializer", expected,
                             physical_get(c, v, PHYSICAL_EXPR, 0),
                             fisics_units_is_literal_value(v));
  }
  if (v->type != AST_COMPOUND_LITERAL ||
      v->compoundLiteral.entryCount != extent)
    return physical_unknown(v,
                            "shape initializer must enumerate every component");
  PhysicalFact result = expected;
  for (unsigned i = 0; i < extent; i++) {
    DesignatedInit *e = v->compoundLiteral.entries[i];
    if (!e || e->fieldName || e->indexExpr)
      return physical_unknown(
          v, "designated shape initializer requires an explicit adapter");
    PhysicalFact component=expected;
    if(physical_state(c)->policy.schema_version>=6 && rank==1 && !depth)
      component=physical_frame_project(c,e->expression,expected,i+1);
    PhysicalFact f = initializer(c, owner, e->expression, component, rank - 1,
                                 extent, depth + 1);
    if (!physical_known(f))
      return f;
    result = physical_merge_trust(result, f);
  }
  return result;
}
bool physical_representation_initializer(CompilerContext *c,
                                         const ParsedType *t, ASTNode *owner,
                                         ASTNode *v) {
  PhysicalFact whole = physical_type(c, t, owner, false, false);
  if (!physical_known(whole) || !whole.type.representation || !v ||
      v->type != AST_COMPOUND_LITERAL)
    return false;
  const FisicsPhysicalRepresentation *r =
      &physical_state(c)->policy.representations[whole.type.representation - 1];
  PhysicalFact scalar_fact = whole;
  scalar_fact.type.representation = 0;
  scalar_fact.carrier = false;
  PhysicalFact result = whole;
  if (r->record) {
    if (v->compoundLiteral.entryCount != r->field_count)
      result = physical_unknown(
          v, "record initializer must enumerate every representation field");
    else
      for (unsigned i = 0; i < r->field_count; i++) {
        DesignatedInit *e = v->compoundLiteral.entries[i];
        const ASTNode *fo = NULL;
        const char *name = NULL;
        const ParsedType *ft = field(definition(c, t), i, &fo, &name);
        if (!e || e->indexExpr ||
            (e->fieldName && strcmp(e->fieldName, r->fields[i]))) {
          result = physical_unknown(
              v, "reordered/designated shape initializer is unsupported");
          break;
        }
        PhysicalFact declared = physical_type(c, ft, fo, false, true);
        if (declared.type.kind && (declared.type.kind != whole.type.kind ||
                                   declared.type.origin != whole.type.origin)) {
          physical_error(c, e->expression, 4316,
                         "record component kind/origin contradicts its "
                         "homogeneous representation");
          result = physical_unknown(v, "contradictory component");
          result.state = PHYSICAL_ERROR;
          break;
        }
        scalar_fact.origin = fo ? fo : owner;
        if(physical_state(c)->policy.schema_version>=6 && r->rank==1)
          scalar_fact=physical_frame_project(c,e->expression,whole,i+1);
        scalar_fact.type.representation=0;scalar_fact.carrier=false;
        PhysicalFact f =
            initializer(c, (ASTNode *)fo, e->expression, scalar_fact,
                        r->rank == 2 ? 2 : 0, r->extent, 0);
        if (!physical_known(f)) {
          result = f;
          break;
        }
        result = physical_merge_trust(result, f);
      }
  } else
    result = initializer(c, owner, v, scalar_fact, r->rank, r->extent, 0);
  if (physical_known(result)) {
    result.type = whole.type;
    if(physical_state(c)->policy.schema_version>=6)result.declared_context=whole.declared_context;
    result.carrier = false;
    result.component_depth = 0;
    result.reason =
        "checked representation initializer; component provenance retained";
  }
  physical_put(c, v, PHYSICAL_EXPR, 0, result);
  physical_event(c, v, "initializer", whole, result, result);
  return true;
}
