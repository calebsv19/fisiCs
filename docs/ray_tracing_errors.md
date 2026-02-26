# Ray Tracing Compiler Error Triage

Date: 2026-02-24  
Project under test: `/Users/calebsv/Desktop/CodeWork/ray_tracing`

This document groups the current ray-tracing diagnostics into likely root causes, marks probable cascades, and proposes a fix order in `fisiCs`.

## 1) Constant-expression gap (`M_PI` in static initializer)

### Symptoms
- `src/editor/camera_editor.c:37`
  - `Initializer for static variable 'kHalfPi' is not a constant expression`

### Context inspected
- `camera_editor.c:37`
  - `static const double kHalfPi = M_PI * 0.5;`

### Likely cause
- The constant-expression evaluator is treating `M_PI` macro usage as non-ICE in static initialization contexts.

### Notes
- Similar `M_PI` uses exist elsewhere and should be accepted if macro-expands to a numeric literal.

---

## 2) `for` init declarator parsing issue (primary shape cluster)

### Symptoms
- `src/import/shape_import.c:147,149`
- `src/geo/geolib/shape_asset.c:355,357`
- `src/tools/cli/shape_sanity_tool.c:355,357` (same pattern via included shape sources)
  - `Undeclared identifier`
  - `Left operand of '=' must be a modifiable lvalue`
  - `Incompatible assignment operands`

### Context inspected
- `shape_import.c:147`
  - `for (size_t i = 0, j = count - 1; i < count; j = i++) { ... }`

### Likely cause
- Parser/sema regression around declaration-form `for` initializers with multiple declarators (`size_t i = ..., j = ...`), causing `i/j` to be treated as undeclared and producing assignment/lvalue cascades.

---

## 3) Decimal literal parse edge case (`.8`)

### Symptoms
- `src/render/integrators/forward_light_integrator.c:355`
  - parse errors near `.` token (`Unexpected token at start of expression`)
- Follow-on at `:356` (`Undeclared identifier`, arithmetic operand mismatch)

### Context inspected
- `forward_light_integrator.c:355`
  - `... : .8;`

### Likely cause
- Lexer/parser path not accepting leading-dot floating literals (`.8`) in this context; cascade follows on the next line.

### Low-risk source-side workaround
- Rewrite `.8` to `0.8` in user code if needed, but compiler should accept both forms.

---

## 4) Function/variable “conflicting types” likely secondary

### Symptoms
- `src/render/ray_tracing2.c:509`
  - `Conflicting types for function`
- `src/app/animation.c:44`
  - `Conflicting types for variable`

### Context inspected
- `include/render/ray_tracing2.h` and `ray_tracing2.c` both show:
  - `void RenderRayTracingScene(SDL_Renderer* renderer);`
- `include/app/animation.h` and `animation.c` both show:
  - `extern SDL_Renderer* renderer;` / `SDL_Renderer* renderer = NULL;`

### Likely cause
- These look type-consistent in source; diagnostics are likely downstream of earlier parse/type-state damage (notably shape/for-init and literal parse issues), or SDL type resolution drift in this TU.

---

## 5) `shape_sanity_tool.c` large cJSON pointer/type cascade is secondary

### Symptoms
- `src/tools/cli/shape_sanity_tool.c:1224+`
  - `Argument ... has incompatible type`
  - many `Operator '->' requires pointer operand`
  - many assignment/type cascades

### Context inspected
- `shape_sanity_tool.c` is an aggregator TU including many `.c` files directly:
  - shared shape core/asset/json + `cJSON.c`

### Likely cause
- Once the primary shape parsing issue in Section 2 triggers, cJSON/shape types degrade and produce widespread pointer/argument cascades in the aggregated TU.
- Also consistent with include-identity/aggregation fragility seen in other shape sanity tool flows.

---

## 6) Warnings currently present

### Symptoms
- `ray_tracing2.c:867,893,894,895`
  - `Unreachable statement`

### Likely cause
- Potentially real control-flow dead code; treat after error blockers are cleared.

---

## Fix order

1. Fix declaration-form `for` parsing/sema for multi-declarator init (`size_t i=..., j=...`).
2. Fix leading-dot float literal parse (`.8`).
3. Re-run ray tracing diagnostics.
4. Re-check `ray_tracing2.c` and `animation.c` conflict reports (likely disappear if they are cascades).
5. Re-check `shape_sanity_tool.c` cJSON pointer cascades.
6. Triage remaining unreachable warnings as separate cleanup.

---

## Expected drop after step 1–2

- `shape_import.c`, `shape_asset.c`, and most of `shape_sanity_tool.c` should collapse significantly.
- Function/variable conflict reports in `ray_tracing2.c` / `animation.c` are likely to reduce if they were parser state cascades.
