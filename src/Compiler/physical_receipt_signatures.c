// SPDX-License-Identifier: Apache-2.0
#include "Compiler/physical_energy_receipt.h"
#include "Compiler/physical_receipt.h"
#include "Extensions/PhysicalSemantics/physical_representation_binding.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "Extensions/extension_units_call_contracts.h"
#include <stdlib.h>
#include <string.h>
typedef struct {
  const ASTNode *node;
  const char *name;
  size_t slot;
  bool function;
} Binding;
static int order(const void *a, const void *b) {
  return strcmp(((const Binding *)a)->name, ((const Binding *)b)->name);
}
static void slot(FILE *out, CompilerContext *c, const ParsedType *type,
                 const ASTNode *n, const char *role, size_t index) {
  PhysicalFact f = physical_type(c, type, n, false, false);
  fputs("{\"role\":", out);
  physical_json_string(out, role);
  fprintf(out, ",\"index\":%zu,\"fact\":", index);
  physical_json_fact(out, c, f);
  FisicsDim8 dim = {0};
  bool resolved = false, unitResolved = false;
  const FisicsUnitDef *unit = NULL;
  fisics_units_parse_type_contract(type, &dim, &resolved, &unit, &unitResolved);
  const char *surface = "unsupported";
  if (f.type.representation && physical_known(f))
    surface = "representation";
  else if (f.carrier && physical_known(f))
    surface = "array";
  else if (type && type->kind == TYPE_PRIMITIVE && !type->isFunctionPointer &&
           !type->isComplex && !type->isImaginary && !type->pointerDepth)
    surface = parsedTypeIsDirectArray(type) ? "array" : "scalar";
  if (physical_state(c)->policy.schema_version >= 5) {
    const char *scalar = type && type->kind == TYPE_PRIMITIVE &&
                                 !type->isLong && !type->isComplex &&
                                 !type->isImaginary
                             ? type->primitiveType == TOKEN_FLOAT    ? "float"
                               : type->primitiveType == TOKEN_DOUBLE ? "double"
                                                                     : NULL
                             : NULL;
    fputs(",\"c_scalar\":", out);
    physical_json_string(out, scalar);
    fputs(",\"c_record_tag\":", out);
    physical_json_string(
        out, type && type->kind == TYPE_STRUCT ? type->userTypeName : NULL);
  }
  fputs(",\"surface\":", out);
  physical_json_string(out, surface);
  fputs(",\"dimension\":", out);
  if (resolved) {
    fputc('[', out);
    for (int k = 0; k < 8; k++)
      fprintf(out, "%s%d", k ? "," : "", dim.e[k]);
    fputc(']', out);
  } else
    fputs("null", out);
  fputs(",\"unit\":", out);
  physical_json_string(out, unitResolved && unit ? unit->name : NULL);
  fputc('}', out);
}
bool physical_receipt_signatures(FILE *out, CompilerContext *c,
                                 const ASTNode *root) {
  if (!root || root->type != AST_PROGRAM)
    return false;
  Binding *bindings = calloc(16384, sizeof(*bindings));
  if (!bindings)
    return false;
  size_t count = 0;
  for (size_t i = 0; i < root->block.statementCount; i++) {
    const ASTNode *n = root->block.statements[i];
    if (!n)
      continue;
    bool fn = n->type == AST_FUNCTION_DECLARATION ||
              n->type == AST_FUNCTION_DEFINITION;
    if (!fn && n->type != AST_VARIABLE_DECLARATION)
      continue;
    size_t limit = fn ? 1 : n->varDecl.varCount;
    for (size_t j = 0; j < limit; j++) {
      const ParsedType *t = fn ? (n->type == AST_FUNCTION_DEFINITION
                                      ? &n->functionDef.returnType
                                      : &n->functionDecl.returnType)
                               : astVarDeclTypeAt(n, j);
      const ASTNode *id =
          fn ? (n->type == AST_FUNCTION_DEFINITION ? n->functionDef.funcName
                                                   : n->functionDecl.funcName)
             : n->varDecl.varNames[j];
      if (!t || t->isStatic || !id || !id->valueNode.value)
        continue;
      const char *name = id->valueNode.value;
      size_t k = 0;
      while (k < count && strcmp(bindings[k].name, name))
        k++;
      if (k == count) {
        if (count == 16384) {
          free(bindings);
          return false;
        }
        count++;
      } else if (bindings[k].node->type == AST_FUNCTION_DEFINITION)
        continue;
      bindings[k] = (Binding){n, name, j, fn};
    }
  }
  qsort(bindings, count, sizeof(*bindings), order);
  fputc('[', out);
  for (size_t i = 0; i < count; i++) {
    const Binding *b = &bindings[i];
    const ASTNode *n = b->node;
    if (i)
      fputc(',', out);
    fputs("{\"name\":", out);
    physical_json_string(out, b->name);
    fprintf(out,
            ",\"entity\":\"%s\",\"variadic\":%s,\"prototype\":%s,\"slots\":[",
            b->function ? "function" : "variable",
            b->function && (n->type == AST_FUNCTION_DEFINITION
                                ? n->functionDef.isVariadic
                                : n->functionDecl.isVariadic)
                ? "true"
                : "false",
            !b->function || (n->type == AST_FUNCTION_DEFINITION
                                 ? n->functionDef.hasPrototype
                                 : n->functionDecl.hasPrototype)
                ? "true"
                : "false");
    if (!b->function)
      slot(out, c, astVarDeclTypeAt(n, b->slot), n, "value", 0);
    else {
      bool def = n->type == AST_FUNCTION_DEFINITION;
      slot(out, c,
           def ? &n->functionDef.returnType : &n->functionDecl.returnType, n,
           "return", 0);
      ASTNode **ps =
          def ? n->functionDef.parameters : n->functionDecl.parameters;
      size_t pc = def ? n->functionDef.paramCount : n->functionDecl.paramCount,
             ordinal = 0;
      for (size_t j = 0; j < pc; j++)
        if (ps[j] && ps[j]->type == AST_VARIABLE_DECLARATION)
          for (size_t k = 0; k < ps[j]->varDecl.varCount; k++) {
            fputc(',', out);
            slot(out, c, astVarDeclTypeAt(ps[j], k), ps[j], "parameter",
                 ordinal++);
          }
    }
    if (physical_state(c)->policy.schema_version >= 5) {
      const ParsedType *ret = b->function ? (n->type == AST_FUNCTION_DEFINITION
                                                 ? &n->functionDef.returnType
                                                 : &n->functionDecl.returnType)
                                          : NULL;
      char *helper =
          physical_shape_attribute(ret, "fisics::physical_operation");
      fputs("],\"shape_helper\":", out);
      physical_json_string(out, helper);
      free(helper);
      if(physical_state(c)->policy.schema_version>=6){
        char *transform=physical_shape_attribute(ret,"fisics::frame_transform");
        fputs(",\"frame_transform\":",out);physical_json_string(out,transform);free(transform);
      }
      if(physical_state(c)->policy.schema_version==7){
        char *transfer=physical_shape_attribute(ret,"fisics::energy_transfer");
        fputs(",\"energy_transfer\":",out);physical_json_string(out,transfer);free(transfer);
      }
      fputc('}', out);
    } else
      fputs("]}", out);
  }
  fputc(']', out);
  free(bindings);
  return !ferror(out);
}
