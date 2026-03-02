# FisiCs Self-Host Error Triage Plan

Source log: `docs/status/errors.txt`  
Scope: diagnostics emitted when IDE analyzed the `fisiCs` project itself.

## Snapshot Summary

- Total diagnostics: **485**
- Errors: **479**
- Warnings: **6**

Subsystem concentration:
- `src/Syntax/*`: **455** diagnostics (450E, 5W)
- `src/Compiler/*`: **24** diagnostics (24E)
- `src/fisics_frontend.c`: **3** diagnostics (3E)
- `src/main.c`: **2** diagnostics (2E)
- `src/Preprocessor/*`: **1** diagnostic (1W)

Top files by count:
1. `src/Syntax/analyze_decls.c` (155E)
2. `src/Syntax/analyze_expr.c` (133E)
3. `src/Syntax/const_eval.c` (44E)
4. `src/Syntax/control_flow.c` (40E)
5. `src/Syntax/layout.c` (31E, 3W)
6. `src/Compiler/pipeline.c` (24E)
7. `src/Syntax/analyze_core.c` (14E)

## Error Pattern Clusters

Top message families:
1. `Argument N of '...' has incompatible type` -> **214**
2. `Operator '[]' requires pointer or array base` -> **102**
3. `Incompatible types in ternary expression` -> **56**
4. `Operator '&' requires lvalue operand` -> **30**
5. `Incompatible assignment operands` -> **26**
6. `Operator '->' requires pointer operand` -> **17**
7. Scalar-op type failures (`&&`, `||`) -> **13**

Interpretation:
- This is **not** a random spread of independent bugs.
- The distribution is consistent with a **small number of core type/lvalue representation regressions** causing widespread cascade failures across semantic analysis.

## Likely Root Causes (Ordered)

## 1) AST node/value representation mismatch in semantic passes

Evidence:
- `[]`, `->`, `&`, ternary, and assignment failures all spike together in `analyze_*`, `const_eval`, `layout`, `control_flow`.
- High count of argument-type mismatch calls into semantic helpers (`analyzeExpression`, `hashAstNode`, `checkStatement`, etc.).

Hypothesis:
- Callers are passing the wrong node/value form (pointer vs value, array vs scalar, or wrong union member path) after recent refactors.

## 2) Shared helper signature drift across Syntax modules

Evidence:
- Repeated `Argument N ... incompatible type` on common helpers (`hashAstNode`, `addErrorWithRanges`, `resolveInScopeChain`, `identifier_name`, `collectGotoTargets`, etc.).
- Hits both `.c` and `.h` units under `src/Syntax/`, indicating API contract mismatches rather than one local typo.

Hypothesis:
- Prototypes and call sites diverged in one or more shared headers.

## 3) Conditional-expression merge/type-promotion logic regression

Evidence:
- 56 ternary mismatches across semantic and pipeline/frontend entry points.
- Usually appears when one side is pointer-like and the other side is int/null/identifier form that should have been normalized.

## 4) Secondary cascades in frontend/CLI wrappers

Evidence:
- `src/fisics_frontend.c` and `src/main.c` have small counts and mostly argument type mismatches.

Conclusion:
- These are likely downstream from Syntax API/type breakage.

## 5) Non-blocking warnings

- `Pointer comparison against non-null integer` (5)
- `Switch case may fall through` (1)

These should be cleaned up after error blockers are fixed.

## Execution Plan (Step-by-Step)

## Phase 0: Establish a stable baseline
1. Keep current `docs/status/errors.txt` as the baseline artifact.
2. Save grouped counts after each phase using the same scripts, so we can verify directional progress.

Gate:
- Baseline counts reproduced exactly (485 total, 479E/6W).

## Phase 1: Repair shared Syntax API contracts first
Focus files:
- `src/Syntax/analyze_expr.h`
- `src/Syntax/analyze_core.h`
- `src/Syntax/analyze_stmt.h`
- `src/Syntax/const_eval.h`
- `src/Syntax/layout.h`
- `src/Syntax/semantic_model*.h`
- `src/Syntax/builtins.h`
- `src/Syntax/scope.h`

Actions:
1. Compare every changed helper prototype to all call sites.
2. Normalize pointer/value qualifiers and expected ownership forms.
3. Rebuild and rerun analysis.

Expected effect:
- Large drop in `Argument N ... incompatible type`.

## Phase 2: Fix AST access/lvalue semantics in core analyzers
Focus files:
- `src/Syntax/analyze_expr.c`
- `src/Syntax/analyze_decls.c`
- `src/Syntax/analyze_core.c`
- `src/Syntax/control_flow.c`
- `src/Syntax/const_eval.c`
- `src/Syntax/layout.c`

Actions:
1. Audit all `[]`, `->`, and `&` uses at reported lines.
2. Verify each use operates on true pointer/array/lvalue expressions.
3. Ensure member/index access paths are derived from AST pointer nodes, not scalar IDs.

Expected effect:
- Drop in:
  - `Operator '[]' requires pointer or array base`
  - `Operator '->' requires pointer operand`
  - `Operator '&' requires lvalue operand`
  - `Incompatible assignment operands`

## Phase 3: Ternary/type merge normalization
Focus:
- Conditional merge helpers and caller sites in `analyze_expr.c`, `layout.c`, `pipeline.c`, `fisics_frontend.c`.

Actions:
1. Enforce canonical C conditional conversion rules.
2. Normalize null-pointer constant handling vs integer literals.
3. Ensure merged type preserves pointer depth and qualifiers correctly.

Expected effect:
- Significant drop in `Incompatible types in ternary expression`.

## Phase 4: Compiler/pipeline integration cleanup
Focus:
- `src/Compiler/pipeline.c`
- `src/fisics_frontend.c`
- `src/main.c`

Actions:
1. Fix remaining argument and identifier type uses after Syntax layer is stable.
2. Revalidate wrapper arguments passed into frontend/pipeline APIs.

Expected effect:
- Clear most remaining non-warning diagnostics.

## Phase 5: Warning cleanup and signal-to-noise hardening
Actions:
1. Resolve real fallthrough cases explicitly (`break`/`return` or documented fallthrough handling).
2. Resolve pointer-vs-int comparisons by correct null/pointer typing.
3. Keep warning policy strict; avoid blanket suppression.

## Validation Workflow Per Phase

Run after each phase:
1. `make -j4`
2. `make tests`
3. Re-run the IDE/self-analysis pass that generated `docs/status/errors.txt`
4. Regenerate grouped counts and compare against prior phase

Completion criteria:
- **0 errors**
- warnings either:
  - reduced to intentional/accepted set with rationale, or
  - fully resolved

## Suggested Work Order for Fastest Error Burn-Down

1. `src/Syntax/analyze_expr.c`
2. `src/Syntax/analyze_decls.c`
3. `src/Syntax/const_eval.c`
4. `src/Syntax/control_flow.c`
5. `src/Syntax/layout.c`
6. `src/Syntax/*.h` API contract sweep
7. `src/Compiler/pipeline.c`
8. `src/fisics_frontend.c`
9. `src/main.c`

Reason:
- Top 5 files account for the vast majority of errors and likely share the same broken assumptions.

## Notes on Cascade vs Primary

Primary-looking:
- Type/signature mismatches on core semantic helpers
- Pointer/array/lvalue misuse in analyzer internals
- Ternary merge/type normalization failures

Likely cascade:
- Wrapper-layer argument mismatches in `main.c` / `fisics_frontend.c`
- Duplicate low-count header diagnostics that mirror `.c` issues
