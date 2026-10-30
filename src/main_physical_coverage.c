// SPDX-License-Identifier: Apache-2.0
#include "Compiler/physical_receipt.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "main_internal.h"
int main_run_physical_coverage(const MainDriverConfig *config,
                               const char *function, const char *path,
                               PhysicalCoveragePolicy policy) {
  CompileOptions options =
      main_make_compile_options(config, config->inputCFiles->items[0], NULL);
  options.enableCodegen = false;
  options.captureSourceContract = true;
  options.capturePhysicalClosure = true;
  CompileResult result = {0};
  PhysicalCoverage report = {0};
  bool ok = false;
  int compiled = compile_translation_unit(&options, &result);
  if (!result.ast || !result.semanticModel || !result.compilerCtx)
    goto done;
  if (!physical_coverage_collect(result.compilerCtx, result.ast, function,
                                 policy, &report))
    goto done;
  ok = physical_receipt_write(path, &result, &options, &report);
done:
  if (!ok)
    fprintf(stderr,
            "Error: physical coverage receipt was not published (requires one "
            "exact definition and complete collectable inputs)\n");
  bool accepted = ok && !compiled && !result.semanticErrors &&
                  report.accepted &&
                  physical_state(result.compilerCtx)->closure_complete;
  if (ok && policy.strict && !accepted) {
    fprintf(stderr, "Error [4319]: selected physical strict obligations were "
                    "not satisfied\n");
    compiler_report_diag(
        result.compilerCtx,
        report.ast.function ? report.ast.function->location : (SourceRange){0},
        DIAG_ERROR, FISICS_DIAG_CODE_PHYSICAL_INCOMPLETE,
        "inspect required obligations and loss locations in the physical "
        "receipt",
        "selected physical strict scope rejected (unknown, unsupported, "
        "unpermitted trust, empty checked work or incomplete analysis)");
  }
  if (ok)
    fprintf(stderr,
            "Physical selected scope: %s; required %zu; checked %zu; ordinary "
            "controls %zu; permitted trust %zu; blocked %zu\n",
            accepted ? "accepted" : "rejected", report.required, report.checked,
            report.controls, report.permitted, report.blocked);
  int exit_code =
      ok && !compiled && !result.semanticErrors && (!policy.strict || accepted)
          ? 0
          : 1;
  physical_coverage_free(&report);
  compile_result_destroy(&result);
  return exit_code;
}
