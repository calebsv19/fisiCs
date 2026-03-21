# Compiler Codegen Bucket Report

## Scope

Wave 0/1 kickoff for Phase 8 (`codegen-ir` + runtime cross-checks).

## Wave 0 Baseline Snapshot (2026-03-11)

Targeted active-suite sweep for `13__*` and `14__*`:

- total selected: 33
- pass: 33
- fail: 0
- skip: 0

This confirms the current baseline passes but does not yet stress deeper
pointer-scaling and loop-phi runtime shapes beyond the existing active cases.

## Wave 1 Probe Expansion (Initial)

Added new runtime differential probes:

- `13__probe_phi_continue_runtime`
- `13__probe_short_circuit_chain_runtime`
- `13__probe_struct_copy_runtime`
- `13__probe_ptr_stride_runtime`
- `13__probe_global_init_runtime`

Probe run (`PROBE_FILTER=13__probe_`):

- blocked: 0
- resolved: 5
- skipped: 0

## Current Blocker Inventory

- No open codegen blockers in the current Wave 1 probe set.

## Resolved In This Wave

`13__probe_ptr_stride_runtime` is now resolved.

- Previous mismatch:
  - `fisics`: `8 6 42949672960`
  - `clang`: `4 8 6`
- Current result:
  - `fisics`: `4 8 6`
  - `clang`: `4 8 6`

Fix summary:
- Pointer arithmetic and pointer-difference scaling now use an element-size
  chooser that prefers LLVM ABI size when semantic size disagrees.
- This avoids mis-scaling on non-`int` pointer element types while preserving
  current behavior for already-correct pointer math cases.

## Promotion Status

Wave 1 runtime probes are now promoted into active runtime-surface wave 2:

- `14__runtime_loop_continue_break_phi`
- `14__runtime_short_circuit_chain_effects`
- `14__runtime_struct_copy_update`
- `14__runtime_pointer_stride_long_long`
- `14__runtime_global_partial_init_zerofill`

Wave 3 runtime expansion (ABI + control-flow) is now added as active suite:

- `14__runtime_struct_mixed_width_pass_return`
- `14__runtime_struct_nested_copy_chain`
- `14__runtime_global_designated_sparse`

Wave 3 probe blocker (not promoted yet):

- `14__probe_switch_loop_control_mix`
  - current mismatch: `fisics=2278`, `clang=2266`

Current targeted baseline after Wave 3:

- `13__*` + `14__*` selected: `41`
- pass: `41`
- fail: `0`

## Next Route (Tomorrow)

1. `13` IR-shape additions for pointer lowering:
   - add explicit IR checks for pointer-difference divisor width and scaled
     pointer add on 64-bit elements.
2. `13` IR-shape depth:
   - short-circuit/ternary merge block and phi shape checks.
3. `14` runtime ABI widening:
   - structs with nested arrays/unions and function-pointer dispatch variants.
4. `13/14` control-flow lowering stress:
   - nested switch+loop `continue`/`break` plus ternary/short-circuit side effects.
5. `14` global/static initialization expansion:
   - sparse designated initializers and nested aggregate zero-fill matrix.
6. Differential discipline:
   - keep UB/implementation-defined cases tagged and excluded from strict
     differential assertions by default.
