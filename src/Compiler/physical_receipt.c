#include "Extensions/PhysicalSemantics/physical_operations.h"
#include "Extensions/PhysicalSemantics/physical_representations.h"
// SPDX-License-Identifier: Apache-2.0
#include "Compiler/physical_energy_receipt.h"
#include "Compiler/physical_receipt.h"
#include "Extensions/PhysicalSemantics/physical_frames.h"
#include "Compiler/units_coverage_output.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
void physical_json_string(FILE *out, const char *s) {
  if (!s) {
    fputs("null", out);
    return;
  }
  fputc('"', out);
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    if (*p == '"' || *p == '\\') {
      fputc('\\', out);
      fputc(*p, out);
    } else if (*p < 32)
      fprintf(out, "\\u%04x", *p);
    else
      fputc(*p, out);
  }
  fputc('"', out);
}
void physical_json_location(FILE *out, const ASTNode *n) {
  if (!n) {
    fputs("null", out);
    return;
  }
  fputs("{\"file\":", out);
  physical_json_string(out, n->location.start.file);
  fprintf(out, ",\"line\":%d,\"column\":%d}", n->line,
          n->location.start.column);
}
void physical_json_fact(FILE *out, CompilerContext *c, PhysicalFact f) {
  const FisicsPhysicalPolicy *p=&physical_state(c)->policy;
  fputs("{\"kind\":", out);
  physical_json_string(out, f.type.kind ? p->kinds[f.type.kind - 1].id : NULL);
  fprintf(out, ",\"role\":%u,\"origin\":",
          f.type.kind ? (unsigned)p->models[f.type.kind - 1].role : 0);
  physical_json_string(out,
                       f.type.origin ? p->origins[f.type.origin - 1].id : NULL);
  fputs(",\"definition_fingerprint\":", out);
  if (f.type.kind)
    fprintf(out, "\"%016" PRIx64 "\"",
            p->models[f.type.kind - 1].definition_fingerprint);
  else
    fputs("null", out);
  if(p->schema_version>=5){
    fputs(",\"representation\":",out);
    physical_json_string(out,f.type.representation?p->representations[f.type.representation-1].id:NULL);
    fputs(",\"representation_fingerprint\":",out);
    if(f.type.representation)fprintf(out,"\"%016" PRIx64 "\"",p->representations[f.type.representation-1].fingerprint);else fputs("null",out);
    fprintf(out,",\"component_depth\":%u",f.component_depth);
    if(p->schema_version>=6){fputs(",\"physical_context\":",out);physical_frame_fact_write(out,c,f);}
    else fputs(",\"frames\":\"unassessed\"",out);
    if(p->schema_version==7)physical_energy_fact_write(out,c,f);
  }
  fprintf(out, ",\"carrier\":%s,\"verified\":%s}", f.carrier ? "true" : "false",
          physical_known(f) ? "true" : "false");
}
static bool equal_file(const char *path, FisicsPhysicalInputReceipt expected) {
  FisicsPhysicalInputReceipt actual;
  return physical_file_receipt(path, &actual) && actual.hash == expected.hash &&
         actual.bytes == expected.bytes;
}
static bool aliases(const char *a, const char *b) {
  struct stat x, y;
  if (!stat(a, &x) && !stat(b, &y) && x.st_dev == y.st_dev &&
      x.st_ino == y.st_ino)
    return true;
  char *ra = realpath(a, NULL), *rb = realpath(b, NULL);
  bool same = ra && rb && !strcmp(ra, rb);
  free(ra);
  free(rb);
  return same;
}
static void input(FILE *out, const char *path, FisicsPhysicalInputReceipt r) {
  fputs("{\"path\":", out);
  physical_json_string(out, path);
  fprintf(out, ",\"hash\":\"%016" PRIx64 "\",\"bytes\":%zu}", r.hash, r.bytes);
}
static void strings(FILE *out, const char *const *s, size_t n) {
  fputc('[', out);
  for (size_t i = 0; i < n; i++) {
    if (i)
      fputc(',', out);
    physical_json_string(out, s[i]);
  }
  fputc(']', out);
}
static bool document(FILE *out, const CompileResult *result,
                     const CompileOptions *options, const PhysicalCoverage *r) {
  FisicsPhysicalState *s = physical_state(result->compilerCtx);
  const FisicsPhysicalPolicy *p = &s->policy;
  fprintf(out,"{\"schema\":\"fisiCs.physical_semantics\",\"schema_version\":%u,\"compiler\":",p->schema_version);
  physical_json_string(out, FISICS_VERSION);
  fprintf(out,
          ",\"analysis_valid\":%s,\"dimension_system\":\"fisiCs.si_dim8.v1\","
          "\"registry_fingerprint\":\"%016" PRIx64 "\",\"policy\":{\"id\":",
          !result->semanticErrors ? "true" : "false", p->registry_fingerprint);
  physical_json_string(out, p->policy_id);
  fprintf(out,
          ",\"fingerprint\":\"%016" PRIx64 "\",\"input\":", p->fingerprint);
  input(out, p->policy_path, p->policy_receipt);
  fputs(",\"catalogs\":[", out);
  for (size_t i = 0; i < p->catalog_count; i++) {
    if (i)
      fputc(',', out);
    input(out, p->catalogs[i].path, p->catalogs[i].receipt);
  }
  fputs("]}", out);
  if(p->schema_version>=2){fputs(",\"operation_model\":",out);physical_operations_write(out,p);}
  if(p->schema_version>=5){fputs(",\"representation_model\":",out);physical_representations_write(out,p);}
  if(p->schema_version>=6){fputs(",\"frame_model\":",out);physical_frames_write(out,p);}
  if(p->schema_version==7){fputs(",\"energy_model\":",out);physical_energy_write(out,p);}
  fputs(",\"dependencies\":{\"complete\":", out);
  fputs(s->closure_complete ? "true" : "false", out);
  fputs(",\"files\":[", out);
  for (size_t i = 0; i < s->source_count; i++) {
    if (i)
      fputc(',', out);
    input(out, s->sources[i].path, s->sources[i].receipt);
  }
  fputs("]}", out);
  fprintf(
      out,
      ",\"options\":{\"dialect\":%u,\"compatibility\":%u,\"overlays\":%" PRIu64
      ",\"trigraphs\":%s,\"target\":",
      (unsigned)options->dialect, (unsigned)options->compatFeatures,
      (uint64_t)options->overlayFeatures,
      options->enableTrigraphs ? "true" : "false");
  physical_json_string(out, cc_get_target_triple(result->compilerCtx));
  fputs(",\"data_layout\":", out);
  physical_json_string(out, options->dataLayout);
  char *source = realpath(options->inputPath, NULL);
  if (!source)
    return false;
  fputs(",\"source\":", out);
  physical_json_string(out, source);
  free(source);
  fputs(",\"defines\":", out);
  strings(out, options->macroDefines, options->macroDefineCount);
  fputs(",\"include_paths\":", out);
  strings(out, options->includePaths, options->includePathCount);
  fputs(",\"effective_include_paths\":", out);
  strings(out, (const char *const *)s->effective_include_paths,
          s->effective_include_count);
  fputs("}", out);
  fputs(",\"declarations\":", out);
  if (!physical_receipt_signatures(out, result->compilerCtx, result->ast))
    return false;
  fputs(",\"declaration_bindings\":[", out);
  bool first = true;
  for (size_t i = 0; i < s->count; i++)
    if (s->entries[i].domain == PHYSICAL_DECL) {
      const PhysicalEntry *e = &s->entries[i];
      if (!first)
        fputc(',', out);
      first = false;
      fprintf(out, "{\"slot\":%zu,\"site\":", e->slot);
      physical_json_location(out, e->fact.origin ? e->fact.origin : e->node);
      fputs(",\"fact\":", out);
      physical_json_fact(out, result->compilerCtx, e->fact);
      fputc('}', out);
    }
  fputc(']', out);
  fputs(",\"selection\":{\"function\":", out);
  physical_json_string(out, r->ast.functionName);
  fprintf(
      out,
      ",\"contract\":\"selected_physical_function_v1\",\"strict\":%s,\"allow_"
      "literal_assumptions\":%s,\"allow_declared_boundaries\":%s,\"enumeration_"
      "complete\":%s,\"accepted\":%s,\"total\":%zu,\"required\":%zu,\"ordinary_"
      "controls\":%zu,\"checked\":%zu,\"permitted_trust\":%zu,\"blocked\":%zu,"
      "\"ordinary_controls_valid\":%s}",
      r->policy.strict ? "true" : "false",
      r->policy.allow_literals ? "true" : "false",
      r->policy.allow_declared ? "true" : "false",
      r->complete ? "true" : "false",
      r->accepted && !result->semanticErrors && s->closure_complete ? "true"
                                                                    : "false",
      r->ast.rowCount, r->required, r->controls, r->checked, r->permitted,
      r->blocked, r->controls_valid ? "true" : "false");
  /* Keep control-role conflicts outside the required-row denominator, while
     refusing strict acceptance. Roles themselves do not depend on metadata. */
  fputs(",\"expressions\":[", out);
  for (size_t i = 0; i < r->ast.expressionCount; i++) {
    if (i)
      fputc(',', out);
    const FisicsCoverageExpression *e = &r->ast.expressions[i];
    fprintf(out, "{\"id\":\"e%zu\",\"site\":", i);
    physical_json_location(out, e->node);
    fputs(",\"fact\":", out);
    physical_json_fact(out, result->compilerCtx, r->expressions[i]);
    fputs(",\"reason\":", out);
    physical_json_string(out, r->expressions[i].reason);
    fprintf(out, ",\"trust_mask\":%u", r->trust[i]);
    if(p->schema_version>=2){fputs(",\"operation_rule\":",out);physical_json_string(out,r->expressions[i].operation_rule);}
    fputs(",\"declaration_origin\":", out);
    physical_json_location(out, r->expressions[i].origin);
    fputs(",\"loss\":", out);
    physical_json_location(out, r->expressions[i].loss);
    fputs(",\"dependencies\":[", out);
    for (size_t j = 0; j < e->dependencyCount; j++) {
      size_t k = 0;
      while (k < r->ast.expressionCount &&
             r->ast.expressions[k].node != e->dependencies[j])
        k++;
      if (j)
        fputc(',', out);
      if (k < r->ast.expressionCount)
        fprintf(out, "\"e%zu\"", k);
      else
        fputs("null", out);
    }
    fputs("]}", out);
  }
  fputs("],\"obligations\":[", out);

  for (size_t i = 0; i < r->ast.rowCount; i++) {
    if (i)
      fputc(',', out);
    const PhysicalCoverageRow *row = &r->rows[i];
    const FisicsCoverageRow *a = &r->ast.rows[i];
    fprintf(out,
            "{\"id\":\"o%zu\",\"boundary\":%u,\"slot\":%zu,\"operation\":", i,
            (unsigned)a->assessment.obligation.boundary,
            a->assessment.obligation.slot);
    physical_json_string(out, a->operation);
    fputs(",\"site\":", out);
    physical_json_location(out, a->assessment.obligation.site);
    fprintf(
        out,
        ",\"strict_required\":%s,\"checked\":%s,\"trust_mask\":%u,\"state\":",
        row->required ? "true" : "false", row->checked ? "true" : "false",
        row->trust);
    physical_json_string(
        out, row->control                        ? "ordinary_control"
             : row->checked                      ? "checked"
             : row->fact.state == PHYSICAL_ERROR ? "contradictory"
             : row->trust && (physical_known(row->fact) ||
                              (row->fact.state == PHYSICAL_CONTROL ||
                               row->fact.state == PHYSICAL_DECLARED_TRUST))
                 ? "trusted_boundary"
                 : "unknown");
    fputs(",\"reason\":", out);
    physical_json_string(out, row->fact.reason);
    fputs(",\"loss\":", out);
    physical_json_location(out, row->fact.loss);
    fputs(",\"fact\":", out);
    physical_json_fact(out, result->compilerCtx, row->fact);
    fputs(",\"expression\":", out);
    if (a->expression < r->ast.expressionCount)
      fprintf(out, "\"e%zu\"", a->expression);
    else
      fputs("null", out);
    fputc('}', out);
  }
  fputs("]}\n", out);
  return !ferror(out);
}
bool physical_receipt_write(const char *path, const CompileResult *result,
                            const CompileOptions *options,
                            const PhysicalCoverage *r) {
  FisicsPhysicalState *s = physical_state(result->compilerCtx);
  if (!s || s->count > 131072)
    return false;
  if (!fisics_units_coverage_destination(path, NULL, options->inputPath,
                                         result))
    return false;
  if (aliases(path, s->policy.policy_path) ||
      !equal_file(s->policy.policy_path, s->policy.policy_receipt))
    return false;
  for (size_t i = 0; i < s->policy.catalog_count; i++)
    if (aliases(path, s->policy.catalogs[i].path) ||
        !equal_file(s->policy.catalogs[i].path, s->policy.catalogs[i].receipt))
      return false;
  for (size_t i = 0; i < s->source_count; i++)
    if (aliases(path, s->sources[i].path) ||
        !equal_file(s->sources[i].path, s->sources[i].receipt))
      return false;
  size_t n = strlen(path) + sizeof(".tmp.XXXXXX");
  char *tmp = malloc(n);
  if (!tmp)
    return false;
  snprintf(tmp, n, "%s.tmp.XXXXXX", path);
  int fd = mkstemp(tmp);
  if (fd < 0) {
    free(tmp);
    return false;
  }
  FILE *out = fdopen(fd, "w");
  bool ok = out && document(out, result, options, r);
  if (out && ftell(out) > (long)FISICS_PHYSICAL_INPUT_MAX_BYTES)
    ok = false;
  if (getenv("FISICS_TEST_FAIL_PHYSICAL_RECEIPT_WRITE"))
    ok = false;
  if (out) {
    if (ok && fflush(out))
      ok = false;
    if (ok && fsync(fd))
      ok = false;
    if (fclose(out))
      ok = false;
  } else
    close(fd);
  if (ok && rename(tmp, path))
    ok = false;
  if (!ok)
    unlink(tmp);
  free(tmp);
  return ok;
}
