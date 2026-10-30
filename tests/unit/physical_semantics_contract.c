// SPDX-License-Identifier: Apache-2.0
#include "Compiler/pipeline.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "Extensions/extension_hooks.h"
#include "Syntax/semantic_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "line %d: %s\n", __LINE__, #x);                          \
      return 1;                                                                \
    }                                                                          \
  } while (0)
static bool frontend(CompilerContext *ctx, const char *source, ASTNode **ast,
                     SemanticModel **model, size_t *errors,
                     FisicsOverlayFeatures flags) {
  return compiler_run_frontend(ctx, "physical-native.c", source, strlen(source),
                               false, false, NULL, 0, NULL, 0, false, false,
                               flags, false, false, false, ast, model, errors);
}
static bool code(CompilerContext *ctx, int expected) {
  size_t n = 0;
  const FisicsDiagnostic *d = compiler_diagnostics_data(ctx, &n);
  for (size_t i = 0; i < n; i++)
    if (d[i].code == expected && d[i].kind == DIAG_ERROR)
      return true;
  return false;
}
int main(int argc, char **argv) {
  CHECK(argc == 2);
  FisicsOverlayFeatures flags =
      FISICS_OVERLAY_PHYSICS_SEMANTICS | FISICS_OVERLAY_PHYSICS_UNITS;
  char source[32768] =
      "double energy [[fisics::dim(energy)]] "
      "[[fisics::quantity_kind(\"quantity.energy\")]]=1; void f(void){";
  for (size_t i = 0; i < 96; i++)
    strcat(source, "energy=energy+energy;");
  strcat(source, "}");
  size_t reached = 0;
  for (size_t fail = 0; fail < 100; fail++) {
    CompilerContext *ctx = cc_create();
    CHECK(ctx);
    cc_set_overlay_features(ctx, flags);
    CHECK(fisics_physical_prepare(ctx, argv[1]));
    FisicsPhysicalState *state = physical_state(ctx);
    state->fail_after = fail;
    ASTNode *ast = NULL;
    SemanticModel *model = NULL;
    size_t errors = 0;
    CHECK(frontend(ctx, source, &ast, &model, &errors, flags));
    if (state->failed) {
      CHECK(code(ctx, 4305) && compiler_diagnostics_error_count(ctx) > 0 &&
            errors > 0);
    } else {
      CHECK(!errors && state->count > 300 && state->boundary_count > 100);
      reached = fail;
    }
    if (model)
      semanticModelDestroy(model);
    cc_destroy(ctx);
    if (reached)
      break;
  }
  CHECK(reached > 8 && reached < 100);
  /* Independent live models own separate tables and remain usable after peer
   * destruction. */
  CompilerContext *a = cc_create(), *b = cc_create();
  CHECK(a && b);
  cc_set_overlay_features(a, flags);
  cc_set_overlay_features(b, flags);
  CHECK(fisics_physical_prepare(a, argv[1]) &&
        fisics_physical_prepare(b, argv[1]));
  CHECK(physical_state(a) != physical_state(b) &&
        physical_state(a)->policy.kinds != physical_state(b)->policy.kinds);
  cc_destroy(a);
  CHECK(fisics_physical_policy_find_kind(&physical_state(b)->policy,
                                         "quantity.energy"));
  cc_destroy(b);
  /* The old public frontend has no policy field: refuse, do not silently ignore
   * the bit. */
  CompilerContext *legacy = cc_create();
  ASTNode *ast = NULL;
  SemanticModel *model = NULL;
  size_t errors = 0;
  CHECK(!frontend(legacy, "int x;", &ast, &model, &errors, flags) &&
        code(legacy, 4301));
  cc_destroy(legacy);
  CompilerContext *plain = cc_create();
  CHECK(
      frontend(plain, "int x=1;", &ast, &model, &errors, FISICS_OVERLAY_NONE));
  CHECK(!physical_state(plain));
  semanticModelDestroy(model);
  cc_destroy(plain);
  printf("physical native lifetime/failure controls: PASS (%zu allocation "
         "thresholds)\n",
         reached);
  return 0;
}
