# Physics Sim Compiler Error Triage

Date: 2026-02-24  
Project under test: `/Users/calebsv/Desktop/CodeWork/physics_sim`

This document groups current `physics_sim` diagnostics into likely root causes, separates primary vs cascade failures, and defines a fix order in `fisiCs`.

## Update (2026-02-26)

Implemented in `fisiCs` since this report was created:

- Include resolver fallback now searches workspace `shared/**/include` roots relative to the including file path (addresses missing shared headers like `core_trace.h` and `kit_viz.h` when project include-path config is incomplete).
- Constant-eval now accepts recorded constant integer variables (not just enum constants).
- Declaration analysis now records `hasConstValue` for `const` integer variables with constant initializers.
- Static initializer validation now accepts simple floating constant expressions that reference `const` variables.
- Unreachable warning emission was de-noised to one warning per contiguous unreachable region.

Next check requested: rerun `physics_sim` scan in the IDE and refresh this doc with remaining diagnostics after these compiler changes.

## 1) Primary parser/sema gap: `for` init with multiple declarators

### Symptoms
- `src/import/shape_import.c:147,149`
- `src/geo/shape_asset.c:355,357`
- `src/physics/rigid/collider_geom.c:45,47`
- `src/physics/rigid/rigid2d_collision.c:22,23`
- `src/app/scene_masks.c:752,755,756`
- `src/tools/cli/shape_sanity_tool.c:355,357` (same shape code path)
  - `Undeclared identifier`
  - `Left operand of '=' must be a modifiable lvalue`
  - `Incompatible assignment operands`

### Context inspected
- Repeated pattern:
  - `for (int i = 0, j = count - 1; i < count; j = i++) { ... }`
  - `for (size_t i = 0, j = count - 1; i < count; j = i++) { ... }`

### Likely cause
- Declaration-form `for` initializers with comma-separated declarators are being misparsed or semantically mis-bound, so `i/j` become undeclared and trigger assignment/lvalue cascades.

---

## 2) Primary constant-expression classification issue (`static const`/array sizes)

### Symptoms
- `src/render/particle_overlay.c`
  - `:30,31,47,48` `Variable-length array not allowed in struct/union field`
  - `:41` `Initializer for static variable 'g_flow_alpha' is not a constant expression`

### Context inspected
- File defines:
  - `static const int FLOW_TRAIL_MAX_POINTS = 16;`
  - struct fields like `float trail_x[FLOW_TRAIL_MAX_POINTS];`
  - `static float g_flow_alpha = FLOW_ALPHA;` where `FLOW_ALPHA` is `static const float`.

### Likely cause
- Compiler is not treating these compile-time constants as integer/floating constant expressions in all required contexts:
  - array bounds in struct fields
  - static-duration initializers.

---

## 3) Tooling/header resolution gap: `physics_trace_tool.c`

### Symptoms
- `src/tools/cli/physics_trace_tool.c:7`
  - warning: `skipping missing local include 'core_trace.h'`
- then many errors from `:70`, `:228+`:
  - `Undeclared identifier`
  - `Operator '&' requires lvalue operand`
  - builtin object-size/memset argument incompatibilities

### Context inspected
- Top includes:
  - `#include "core_scene.h"`
  - `#include "core_trace.h"`
  - `#include "cJSON.h"`

### Likely cause
- `core_trace.h` is not being found for this TU under current include resolution; once missing, most `core_trace_*` symbols become undeclared and generate long cascades.
- This appears to be either:
  - project header path configuration mismatch for this tool, or
  - include resolver gap for this layout.

---

## 4) Secondary/cascade shape tool cluster (`shape_sanity_tool.c`)

### Symptoms
- `src/tools/cli/shape_sanity_tool.c:1224+`
  - cJSON argument type mismatches
  - many `->` on non-pointer
  - many assignment/type cascades

### Context inspected
- TU includes multiple implementation `.c` files directly from shared shape/cJSON paths.

### Likely cause
- This cluster is likely mostly downstream of Sections 1 and 2 in current runs, plus include-identity fragility for aggregator TUs.

---

## 5) Smaller isolated signal

### Symptoms
- `src/render/debug_draw_objects.c:42:22` `Undeclared identifier`

### Context inspected
- Line is in allocation for `KitVizVecSegment`:
  - `seg_count * sizeof(KitVizVecSegment)`

### Likely cause
- Type visibility issue from `kit_viz.h` parse/resolve for this TU (may clear once upstream parse state issues are fixed).

---

## Fix order

1. Fix declaration-form `for` initializer handling with multiple declarators.
2. Fix constant-expression treatment for `static const` values used in:
   - struct array bounds
   - static-duration initializers.
3. Re-run `physics_sim` diagnostics.
4. Fix/include-path resolution for `physics_trace_tool.c` (`core_trace.h` pathing).
5. Re-run and then triage residual `shape_sanity_tool.c` / `debug_draw_objects.c` leftovers.

---

## Expected impact

- Step 1 should remove a large cross-file cluster (`shape_import`, `shape_asset`, rigid collision files, `scene_masks`, and related shape tool lines).
- Step 2 should clear `particle_overlay.c` blocker diagnostics.
- Step 4 should collapse most `physics_trace_tool.c` cascades.
