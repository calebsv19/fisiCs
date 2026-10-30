// SPDX-License-Identifier: Apache-2.0
#include "Compiler/pipeline.h"
#include "Extensions/PhysicalSemantics/physical_coverage.h"
#include "Extensions/PhysicalSemantics/physical_frame_semantics.h"
#include "Extensions/extension_hooks.h"
#include "Syntax/semantic_model.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static char *read_source(const char *path) {
  FILE *f = fopen(path, "rb");
  assert(f);
  assert(!fseek(f, 0, SEEK_END));
  long n = ftell(f);
  assert(n >= 0);
  rewind(f);
  char *s = calloc((size_t)n + 1, 1);
  assert(s && fread(s, 1, (size_t)n, f) == (size_t)n);
  fclose(f);
  return s;
}
static bool has_code(CompilerContext *c, int code) {
  size_t n = 0;
  const FisicsDiagnostic *d = compiler_diagnostics_data(c, &n);
  for (size_t i = 0; i < n; i++)
    if (d[i].kind == DIAG_ERROR && (int)d[i].code == code)
      return true;
  return false;
}
int main(int argc, char **argv) {
  assert(argc == 4 || argc == 7);
  char *source = read_source(argv[2]);
  bool sweep = argc == 4;
  size_t threshold = 0;
  FisicsOverlayFeatures flags =
      FISICS_OVERLAY_PHYSICS_SEMANTICS | FISICS_OVERLAY_PHYSICS_UNITS;
  for (; threshold < 128; threshold++) {
    CompilerContext *c = cc_create();
    assert(c);
    cc_set_overlay_features(c, flags);
    assert(fisics_physical_prepare(c, argv[1]));
    FisicsPhysicalState *s = physical_state(c);
    if (sweep)
      s->fail_after = threshold;
    ASTNode *ast = NULL;
    SemanticModel *model = NULL;
    size_t errors = 0;
    assert(compiler_run_frontend(c, argv[2], source, strlen(source), false,
                                 false, NULL, 0, NULL, 0, false, false, flags,
                                 false, false, false, &ast, &model, &errors));
    bool failed = s->failed;
    if (sweep && failed)
      assert(errors && has_code(c, 4305));
    else {
      if (sweep)
        assert(!errors);
      else if (atoi(argv[3]))
        assert(errors && has_code(c, atoi(argv[3])));
      else
        assert(!errors);
      PhysicalCoverage report = {0};
      PhysicalCoveragePolicy policy = {.strict = true,
                                       .allow_literals = sweep || atoi(argv[5]),
                                       .allow_declared =
                                           sweep || atoi(argv[6])};
      assert(physical_coverage_collect(c, ast, "check", policy, &report));
      bool expected = sweep || atoi(argv[4]);
      if (report.accepted != expected)
        fprintf(stderr,
                "coverage expected=%u accepted=%u complete=%u checked=%zu "
                "blocked=%zu required=%zu\n",
                expected, report.accepted, report.complete, report.checked,
                report.blocked, report.required);
      assert(report.accepted == expected);
      assert(report.required ==
             report.checked + report.permitted + report.blocked);
      printf("{\"accepted\":%s,\"complete\":%s,\"required\":%zu,\"checked\":%"
             "zu,\"permitted\":%zu,\"blocked\":%zu,\"controls\":%zu,\"fact_"
             "bytes\":%zu,\"context_bytes\":%zu,\"contexts\":%zu,\"facts\":[",
             report.accepted ? "true" : "false",
             report.complete ? "true" : "false", report.required,
             report.checked, report.permitted, report.blocked, report.controls,
             sizeof(PhysicalFact), sizeof(PhysicalDeclaredContext),
             s->declared_context_count);
      bool first = true;
      for (size_t i = 0; i < s->count; i++) {
        const PhysicalEntry *e = &s->entries[i];
        if (e->domain != PHYSICAL_EXPR)
          continue;
        PhysicalDeclaredContext v = physical_frame_context(c, e->fact);
        const char *name =
            e->node->type == AST_IDENTIFIER   ? e->node->valueNode.value
            : e->node->type == AST_DOT_ACCESS ? e->node->memberAccess.field
            : e->node->type == AST_BINARY_EXPRESSION ? e->node->expr.op
                                                     : "";
        printf("%s{\"name\":\"%s\",\"line\":%d,\"state\":%u,\"kind\":\"%s\","
               "\"frame\":\"%s\",\"axis\":%u,\"flags\":%u,\"applied_at\":%u,"
               "\"from\":%u,\"to\":%u,\"moment\":%u,\"rule\":\"%s\"}",
               first ? "" : ",", name ? name : "", e->node->line, e->fact.state,
               e->fact.type.kind ? s->policy.kinds[e->fact.type.kind - 1].id
                                 : "",
               v.frame ? s->policy.frames[v.frame - 1].id : "", v.axis, v.flags,
               v.applied_at, v.displacement_from, v.displacement_to,
               v.moment_reference, e->fact.operation_rule ? e->fact.operation_rule : "");
        first = false;
      }
      printf("]}\n");
      physical_coverage_free(&report);
      assert(sizeof(PhysicalFact) <= 64);
    }
    if (model)
      semanticModelDestroy(model);
    cc_destroy(c);
    if (!sweep || !failed)
      break;
  }
  assert(threshold < 128);
  if (sweep)
    printf("allocation thresholds: %zu\n", threshold);
  free(source);
  return 0;
}
