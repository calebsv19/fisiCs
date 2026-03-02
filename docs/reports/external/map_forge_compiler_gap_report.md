# Map Forge Compiler Gap Report

Date: 2026-02-23  
Repos:
- Compiler: `/Users/calebsv/Desktop/CodeWork/fisiCs`
- Target project: `/Users/calebsv/Desktop/CodeWork/map_forge`

## Map Forge layout (quick)
- Core code: `map_forge/src/{app,camera,core,map,render,route,ui}`
- Public headers: `map_forge/include/...`
- IDE compile metadata: `map_forge/ide_files/build_flags.json`
- Build flags include Vulkan and SDL defines (notably `MAPFORGE_HAVE_VK=1`, `VK_RENDERER_SHADER_ROOT=...`).

## Error/Warning clusters from your log

## 1) Multi-declarator local init parsing bug
Symptoms:
- `font.c` at lines `218`, `267`, `342`, `395`:
  - `Undeclared identifier`
  - `Left operand of '=' must be a modifiable lvalue`
  - cascades into `&h` errors and bad arg types
- Triggering pattern is `int w = 0, h = 0;`.

Likely compiler cause:
- In `src/Parser/parser_decl.c:750`, local initializers use `parseExpression(parser)`.
- `parseExpression` consumes comma expressions, so `int w = 0, h = 0;` is parsed as:
  - `w` initialized with comma-expression `(0, h = 0)`
  - `h` never declared
- Reproduced directly with `./fisics_shim_shadow` on a minimal probe.

Fix target:
- `src/Parser/parser_decl.c:750`
- Parse initializer with assignment-expression (no top-level comma), not full comma-expression.

## 2) Compound literal nested-brace parsing gap
Symptoms:
- `route_render.c:64-74` and `road_renderer.c:179-189`:
  - `expected 'Unexpected token at start of expression', got '{'`
  - then cascading undeclared/lvalue/control-flow errors.
- Triggering pattern:
  - `(SDL_Vertex){a0, color, {0.0f, 0.0f}}`
  - inner `{0.0f, 0.0f}` in compound literal entry.

Likely compiler cause:
- `parseCompoundLiteralPratt` (`src/Parser/Expr/parser_expr_pratt.c:791+`) only accepts each entry as an expression.
- Nested brace sub-initializers inside compound literals are not accepted in expression context.
- Reproduced with local typedef-based probe.

Fix target:
- `src/Parser/Expr/parser_expr_pratt.c` compound-literal entry parsing.
- Reuse/align with initializer-list logic used in declaration initializers (`parser_array.c` / `parseInitializerList` path).

## 3) Static float/double constant-expression rejection
Symptoms:
- `time.c:7`, `map_space.c:3`, `mercator.c:5-6`:
  - `Initializer for static variable ... is not a constant expression`
- These are valid C initializers like `static const float x = 4096.0f;`.

Likely compiler cause:
- `src/Syntax/analyze_decls.c:1187` checks static init constness via `constEvalInteger(...)` only.
- Floating constant expressions are rejected by design of current check.
- Reproduced with minimal static `float`/`double` probe.

Fix target:
- Extend static initializer const-eval path in `analyze_decls.c` to accept arithmetic constant expressions for floating types (and other non-integer scalar constants as applicable).

## 4) Missing builtin macro coverage for float limits
Symptoms:
- `astar.c:143-144`, `route.c:228`:
  - `Undeclared identifier` on `FLT_MAX`.

Likely compiler cause:
- `float.h` maps `FLT_MAX` to `__FLT_MAX__` on this toolchain.
- Preprocessor builtins currently define `__FLT_EPSILON__` etc, but not `__FLT_MAX__`.
- See `src/Preprocessor/preprocessor.c:626+`.
- Reproduced: `FLT_MAX` becomes unresolved `__FLT_MAX__`.

Fix target:
- Add builtin numeric macros for max/min family used by system headers:
  - at least `__FLT_MAX__`, `__DBL_MAX__`, `__LDBL_MAX__` (and likely min-norm/subnorm companions used by headers).

## 5) Restrict-alias warnings are over-eager (likely false positives)
Status: Completed in `fisiCs` on 2026-02-23.

Symptoms:
- Warnings in:
  - `app_route.c` (`50`, `189`, `259`, `296`)
  - `app_tile_pipeline.c` (`376`, `494`, `557`, `732`)
  - `tile_loader.c` (`178`, `277`)
- Common pattern is pthread APIs like `pthread_cond_wait(&obj->cond, &obj->mutex)`.

Likely compiler cause:
- Alias check keys by base identifier only (`baseIdentifierName`) in `src/Syntax/analyze_expr.c:171`.
- Both args share base `app` or `loader`, so warning fires even for distinct fields.

Fix target:
- Tighten heuristic:
  - distinguish member-access paths (`app->route_worker_cond` vs `app->route_worker_mutex`)
  - suppress for known library APIs where parameter aliasing model is not this simplistic.
Implemented:
- Restrict call-arg alias heuristic now compares full access paths (`accessPath`) instead of only the base identifier.
- Added regression test `tests/syntax/semantic_restrict_alias_fields.c` to ensure distinct members (e.g. `&p.cond`, `&p.mutex`) do not warn.
- Added runner `tests/syntax/run_semantic_restrict_alias_fields.sh` and wired target `semantic-restrict-alias-fields` into `syntax-tests`.

## 6) Switch fallthrough warnings are broad/noisy
Status: Completed in `fisiCs` on 2026-02-23 (grouped-empty-case suppression).

Symptoms:
- Multiple `Switch case may fall through` warnings across render/map files.

Likely compiler cause:
- `src/Syntax/control_flow.c:106-109` warns on every reachable non-stopping case.
- No suppression for intentional fallthrough patterns/comments/attributes.

Fix target:
- Add intent-aware suppression:
  - grouped empty labels
  - explicit fallthrough marker comments/attributes
  - or reduce warning level/severity for now.
Implemented:
- `control_flow` fallthrough warning now requires a non-empty case body before warning.
- This suppresses warnings for intentional grouped labels (`case A: case B: ...`) while preserving warnings for real body-to-next-case fallthrough.
- Added regression:
  - `tests/syntax/semantic_switch_grouped_cases.c`
  - `tests/syntax/run_semantic_switch_grouped_cases.sh`
  - `tests/spec/goldens/syntax/semantic_switch_grouped_cases.golden`
  - `makefile` target `semantic-switch-grouped-cases` in `syntax-tests`.

## 7) Unreachable-statement warnings likely CFG false positives
Status: Completed in `fisiCs` on 2026-02-23 (goto-target label reachability).

Symptoms:
- `mft_loader.c:26`, `265`, `266` marked unreachable.
- Source at those lines is normally reachable in valid control flow.

Likely compiler cause:
- Current control-flow pass (`src/Syntax/control_flow.c`) is simplified and can mis-handle complex `goto fail`/label cleanup shapes.

Fix target:
- Improve CFG handling for labels/goto cleanup paths before emitting unreachable diagnostics there.
Implemented:
- Control-flow pass now pre-collects goto target labels per function.
- During statement-list traversal, a label that is a goto target re-enables reachability, preventing false-positive unreachable diagnostics on cleanup blocks.
- Goto statements are treated as stopping control-flow in the local linear pass.
- Added regression:
  - `tests/syntax/semantic_goto_reachable_label.c`
  - `tests/syntax/run_semantic_goto_reachable_label.sh`
  - `tests/spec/goldens/syntax/semantic_goto_reachable_label.golden`
  - `makefile` target `semantic-goto-reachable-label` in `syntax-tests`.

## 8) `VK_RENDERER_SHADER_ROOT` undeclared (`sdl_renderer.c:116`)
Status: Completed in `fisiCs` on 2026-02-23.

Notes:
- Map Forge Makefile defines this macro (`-DVK_RENDERER_SHADER_ROOT=\"...\"`).
- `ide_files/build_flags.json` currently includes both unresolved and resolved forms:
  - `VK_RENDERER_SHADER_ROOT=\"$(VK_RENDERER_RESOLVED_DIR)\"`
  - `VK_RENDERER_SHADER_ROOT=\"/Users/.../shared/vk_renderer\"`

Likely issue:
- Could be compile-command ingestion/selection order issue in IDE pipeline, not core parser/sema.
- Keep as integration check after the parser/sema fixes above.
Implemented:
- Fixed macro-define value tokenization in preprocessor initialization:
  - Macro values from frontend/CLI are now lexed into real replacement-token streams (instead of first-char guessing).
  - This preserves string literals correctly for defines like `VK_RENDERER_SHADER_ROOT="..."`.
- Added robustness for frontend macro entries that include raw `-D` prefixes (now accepted).
- Added CLI `-D` ingestion end-to-end:
  - `main.c` now parses `-DNAME` / `-DNAME=VALUE`
  - `CompileOptions` now carries `macroDefines` into frontend preprocessing.
- Added regressions:
  - `tests/preprocessor/cli_define_string_macro.c`
  - `tests/preprocessor/run_pp_cli_define_string_macro.sh`
  - `tests/unit/frontend_api_macro_define_string.c`
  - `makefile` updated to run CLI regression in `preprocessor-tests`.

## Priority fix order (recommended)
1. `parser_decl` initializer parse (`int w = 0, h = 0` bug).
2. Compound literal nested-brace parsing in expression context.
3. Static float/double constant-expression acceptance.
4. Preprocessor builtin macro coverage (`__FLT_MAX__` family).
5. Restrict alias warning precision.
6. Control-flow warning precision (`fallthrough`, `unreachable`).
7. Verify IDE macro ingestion for `VK_RENDERER_SHADER_ROOT`.

## High-value regression tests to add in `fisiCs`
- `int a=0, b=0;` local declaration parses as two declarators.
- `(T){x, y, {u, v}}` nested compound literal entries.
- `static const float f = 1.0f;` and `static const double d = 2.0;`.
- `#include <float.h>` + `FLT_MAX` usage resolves.
- `pthread_cond_wait(&s->cond, &s->mutex)` does not trigger restrict-alias warning.
- switch with intentional grouped cases does not warn unless real fallthrough risk.
- `goto fail` cleanup pattern does not spuriously mark reachable cleanup lines unreachable.
