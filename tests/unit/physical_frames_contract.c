// SPDX-License-Identifier: Apache-2.0
#include "Compiler/pipeline.h"
#include "Extensions/PhysicalSemantics/physical_coverage.h"
#include "Extensions/PhysicalSemantics/physical_frame_binding.h"
#include "Extensions/extension_hooks.h"
#include "Syntax/semantic_model.h"
#undef NDEBUG
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
static void declarations(const char *path) {
  FisicsOverlayFeatures flags =
      FISICS_OVERLAY_PHYSICS_SEMANTICS | FISICS_OVERLAY_PHYSICS_UNITS;
  char source[24000] = "";
  for (unsigned i = 0; i < 18; i++) {
    char line[1024];
    const char *point[] = {"world_zero", "member_a_zero", "member_a_load"};
    snprintf(line, sizeof(line),
             "double v%u [[fisics::dim(force)]] "
             "[[fisics::quantity_kind(\"quantity.force\")]] "
             "[[fisics::frame(\"frame.%s\")]] "
             "[[fisics::component_axis(\"axis.%c\")]] "
             "[[fisics::applied_at(\"point.%s\")]];\n",
             i, i < 9 ? "world" : "member_a", 'x' + i % 3, point[(i / 3) % 3]);
    strcat(source, line);
  }
  strcat(source, "double a [[fisics::dim(force)]] "
                 "[[fisics::quantity_kind(\"quantity.force\")]] "
                 "[[fisics::frame(\"frame.world\")]] "
                 "[[fisics::component_axis(\"axis.x\")]], b;\n");
  strcat(source, "void check(void){v0=v0+v0;}\n");
  size_t reached = 0;
  for (size_t threshold = 0; threshold < 100; threshold++) {
    CompilerContext *c = cc_create();
    assert(c);
    cc_set_overlay_features(c, flags);
    assert(fisics_physical_prepare(c, path));
    FisicsPhysicalState *s = physical_state(c);
    s->fail_after = threshold;
    ASTNode *ast = NULL;
    SemanticModel *model = NULL;
    size_t errors = 0;
    assert(compiler_run_frontend(c, "frames-native.c", source, strlen(source),
                                 false, false, NULL, 0, NULL, 0, false, false,
                                 flags, false, false, false, &ast, &model,
                                 &errors));
    if (s->failed) {
      assert(errors && compiler_diagnostics_error_count(c));
    } else {
      assert(!errors && s->declared_context_count >= 19);
      bool isolated = false;
      for (size_t i = 0; i < s->count; i++) {
        PhysicalEntry *e = &s->entries[i];
        if (e->domain == PHYSICAL_DECL && e->slot == 1) {
          assert(!e->fact.declared_context && !e->fact.type.kind);
          isolated = true;
        }
      }
      assert(isolated);
      PhysicalCoverage coverage = {0};
      assert(physical_coverage_collect(
          c, ast, "check", (PhysicalCoveragePolicy){true, true, true},
          &coverage));
      assert(coverage.complete && coverage.checked > 0 && !coverage.blocked &&
             coverage.accepted);
      physical_coverage_free(&coverage);
      reached = threshold;
    }
    if (model)
      semanticModelDestroy(model);
    cc_destroy(c);
    if (reached)
      break;
  }
  assert(reached > 3 && reached < 100);
  printf("declaration allocation thresholds: %zu\n", reached);
}
int main(int argc, char **argv) {
  assert(argc >= 4);
  FisicsPhysicalPolicy p = {0}, q = {0};
  FisicsPhysicalPolicyDiagnostic d = {0};
  assert(fisics_physical_policy_load(argv[1], &p, &d));
  assert(p.schema_version == 6);
  FisicsPhysicalPolicy saved = p;
  if (!strcmp(argv[3], "fail")) {
    assert(argc == 5);
    assert(!fisics_physical_policy_load(argv[2], &p, &d));
    if ((int)d.code != atoi(argv[4]))
      fprintf(stderr, "unexpected %d %s\n", d.code, d.message);
    assert((int)d.code == atoi(argv[4]) && !memcmp(&p, &saved, sizeof(p)));
  } else if (!strcmp(argv[3], "lower")) {
    for (unsigned i = 0; i < 4; i++) {
      FisicsPhysicalLoadOptions o = fisics_physical_load_defaults();
      size_t *limits[] = {&o.max_frame_contexts, &o.max_reference_points,
                          &o.max_frames, &o.max_frame_transforms};
      *limits[i] = 1;
      assert(!fisics_physical_policy_load_with_options(argv[2], &p, o, &d));
      assert(d.code == 4304 && !memcmp(&p, &saved, sizeof(p)));
    }
  } else if (!strcmp(argv[3], "same") || !strcmp(argv[3], "changed")) {
    if (!fisics_physical_policy_load(argv[2], &q, &d)) {
      fprintf(stderr, "%d %s\n", d.code, d.message);
      return 1;
    }
    assert((q.fingerprint == p.fingerprint) == !strcmp(argv[3], "same"));
    assert(q.frames != p.frames);
    if (!strcmp(argv[3], "same"))
      assert(q.frame_fingerprint == p.frame_fingerprint);
  } else {
    assert(!strcmp(argv[3], "qualify"));
    size_t threshold = 0;
    for (; threshold < 2000; threshold++) {
      FisicsPhysicalLoadOptions o = fisics_physical_load_defaults();
      o.fail_after = threshold;
      if (fisics_physical_policy_load_with_options(argv[1], &p, o, &d))
        break;
      assert(d.code == 4305 && !memcmp(&p, &saved, sizeof(p)));
    }
    assert(threshold > 80 && threshold < 2000);
    for (unsigned i = 0; i < 8; i++) {
      FisicsPhysicalLoadOptions o = fisics_physical_load_defaults();
      size_t *limits[] = {&o.max_frame_contexts, &o.max_reference_points,
                          &o.max_frames, &o.max_frame_transforms};
      if (i < 4)
        *limits[i] = i == 0 ? 65 : i == 3 ? 513 : 257;
      else
        *limits[i - 4] = 1;
      /* Baseline has one context/transform: lower limits for those tested by
       * generated fixtures. */
      if (i == 4 || i == 7)
        continue;
      FisicsPhysicalPolicy before = p;
      assert(!fisics_physical_policy_load_with_options(argv[1], &p, o, &d));
      assert(d.code == (i < 4 ? 4301 : 4304) &&
             !memcmp(&p, &before, sizeof(p)));
    }
    FILE *f = tmpfile();
    assert(f);
    physical_frames_write(f, &p);
    long len = ftell(f);
    assert(len > 0);
    rewind(f);
    char *buf = malloc((size_t)len + 1);
    assert(buf && fread(buf, 1, (size_t)len, f) == (size_t)len);
    buf[len] = 0;
    fclose(f);
    FisicsJsonValue *j = NULL;
    FisicsJsonDiagnostic jd = {0};
    assert(fisics_json_parse(buf, (size_t)len, &j, &jd));
    assert(physical_frames_match(j, &p));
    p.frame_fingerprint++;
    assert(!physical_frames_match(j, &p));
    p.frame_fingerprint--;
    fisics_json_free(j);
    free(buf);
    declarations(argv[1]);
    printf(
        "{\"allocations\":%zu,\"fact_bytes\":%zu,\"context_bytes\":%zu,\"frame_"
        "bytes\":%zu,\"transform_bytes\":%zu,\"fingerprint\":\"%016" PRIx64
        "\"}\n",
        threshold, sizeof(PhysicalFact), sizeof(PhysicalDeclaredContext),
        sizeof(PhysicalFrame), sizeof(PhysicalFrameTransform),
        p.frame_fingerprint);
    assert(sizeof(PhysicalFact) <= 64);
  }
  fisics_physical_policy_free(&p);
  fisics_physical_policy_free(&q);
  return 0;
}
