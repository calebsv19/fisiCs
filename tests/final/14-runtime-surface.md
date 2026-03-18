# Runtime / Library Surface

## Status Snapshot (2026-03-18)

- Active test count: 38 (`tests/final/meta/14-runtime-surface*.json`)
- Runtime+differential tests: 33
- Current bucket run: wave6 promoted ids pass exact-id validation
- Next expansion plan: `docs/plans/runtime_bucket_14_execution_plan.md`

## Scope
Minimal headers and builtin surface to compile real programs.

## Validate
- Provided headers: stddef.h, stdint.h, stdbool.h, limits.h (as supported).
- Builtins/intrinsics you choose to implement.
- offsetof macro expansion via system headers.

## Acceptance Checklist
- System-like headers compile and expose expected typedefs/macros.
- stdbool.h defines bool/true/false appropriately.
- stdint.h defines fixed-width types.
- stddef.h exposes size_t and NULL.
- offsetof computes field offsets in headers/macros.
- limits.h defines key integer limits.

## Test Cases (initial list)
1) `14__header_stdbool`
   - bool/true/false compile.
2) `14__header_stdint`
   - int8_t/uint32_t usage.
3) `14__header_stddef`
   - size_t and NULL usage.
4) `14__header_limits`
   - integer limits macros usable.
5) `14__offsetof`
   - offsetof macro expands and folds to a constant.
6) `14__runtime_div_mod_edges`
   - Runtime division/modulo edge behavior (`7/3`, `7%3`, `-7/3`).
7) `14__runtime_pointer_arith`
   - Runtime pointer-difference and indexed load behavior.
8) `14__runtime_recursion`
   - Recursive call lowering and runtime stack behavior.
9) `14__runtime_struct_return`
   - Struct return ABI/runtime behavior.
10) `14__runtime_function_pointer`
   - Function pointer call runtime behavior.
11) `14__runtime_float_precision`
   - Floating precision surface behavior (`0.1 + 0.2` formatting).
12) `14__runtime_float_infinity`
   - Runtime infinity comparison behavior (`1.0/0.0 > 1e300`).
13) `14__runtime_large_stack`
   - Larger stack frame handling.
14) `14__runtime_switch_simple`
   - Runtime switch dispatch correctness for direct case matches.
15) `14__runtime_args_env`
   - Harness runtime `run_args` and `run_env` behavior.
16) `14__runtime_stdin`
   - Harness runtime `run_stdin` behavior.
17) `14__runtime_variadic_calls`
   - Variadic call-site behavior through libc `printf`.
18) `14__runtime_deep_recursion`
   - Deep recursive call-path stress (`tri(300)`).
19) `14__runtime_struct_layout_alignment`
   - Runtime struct layout and alignment-offset checks.

## Open Gaps (Tracked)
- No active blockers in the current probe lane (`14__probe_*` set is green).

## Wave 2 Additions (Codegen Probe Promotion)
20) `14__runtime_loop_continue_break_phi`
   - Loop-carried accumulator with `continue`/`break` edges.
21) `14__runtime_short_circuit_chain_effects`
   - Nested short-circuit chain side effects and evaluation ordering.
22) `14__runtime_struct_copy_update`
   - Struct return + by-value copy/update runtime behavior.
23) `14__runtime_pointer_stride_long_long`
   - Pointer-difference scaling and pointer-index stride on `long long*`.
24) `14__runtime_global_partial_init_zerofill`
   - Global partial initializer zero-fill semantics.

## Wave 3 Additions (ABI + Control Flow)
25) `14__runtime_struct_mixed_width_pass_return`
   - Mixed-width struct parameter/return ABI path (`int`, `long long`, `double`).
26) `14__runtime_struct_nested_copy_chain`
   - Nested aggregate copy/update chain semantics.
27) `14__runtime_global_designated_sparse`
   - Sparse designated global initializer + zero-fill verification.

## Wave 4 Additions (Control-Flow Promotion)
28) `14__runtime_switch_loop_control_mix`
   - Promoted from probe lane after `continue` target resolution fix in
     `switch` inside loop lowering.

## Wave 5 Additions (Arithmetic + Floating Start)
29) `14__runtime_unsigned_wraparound`
   - Promoted from `14__probe_unsigned_wrap` into active differential runtime.
30) `14__runtime_nan_propagation`
   - Promoted from `14__probe_float_nan` into active differential runtime.
31) `14__runtime_signed_div_mod_sign_matrix`
   - Added signed-operand division/modulo sign matrix and promoted into active differential runtime.
32) `14__runtime_nan_comparisons`
   - Added NaN comparison-matrix anchor and promoted into active differential runtime.

## Wave 5 Promotion
- `14__runtime_float_cast_roundtrip`
  - Promoted after codegen cast/unary-float fixes; now matches clang.

## Wave 6 Additions (ABI / Call Surface)
33) `14__runtime_many_args_mixed_width`
   - Promoted from `14__probe_many_args_mixed_width` into active differential runtime.
34) `14__runtime_variadic_promotions_matrix`
   - Promoted after signedness-correct vararg integer promotion fixes.
35) `14__runtime_struct_with_array_pass_return`
   - Promoted after array-element pointer/load lowering fixes.
36) `14__runtime_union_payload_roundtrip`
   - Promoted after semantic long-width and union layout fixes.
37) `14__runtime_fnptr_dispatch_table_mixed`
   - Promoted after function-pointer array element-type GEP normalization fix.
38) `14__runtime_float_cast_roundtrip`
   - Added to active runtime list as wave5 closure item.

## Next Route (13/14)
1) Add IR-shape anchors in `13` for pointer arithmetic:
   - `13__ir_ptrdiff_long_long`
   - `13__ir_ptr_add_scaled_i64`
2) Expand `13` IR coverage depth:
   - branch/phi shape checks for short-circuit + ternary merges
   - pointer-offset IR checks across `int*` and `long long*`
3) Add remaining runtime ABI stress:
   - structs containing arrays/unions
   - function-pointer dispatch over mixed-width aggregate arguments
4) Add control-flow stress follow-ups:
   - nested switch ladders with fallthrough under loop-carried state
   - conditional operator + logical chaining with side-effect counters
5) Add safe differential expansion rules:
   - keep UB/impl-defined tests tagged and excluded from strict diff assertions
   - prefer deterministic integer/struct/control-flow surfaces first

## Expected Outputs
- AST/Diagnostics/IR goldens for compile-surface checks.
- Runtime stdout/stderr/exit expectations for executable checks.
