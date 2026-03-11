# Runtime / Library Surface

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
- No open runtime-surface probe gaps at this baseline; carry deeper switch-loop
  stress in bucket 15 torture tracking.

## Expected Outputs
- AST/Diagnostics/IR goldens for compile-surface checks.
- Runtime stdout/stderr/exit expectations for executable checks.
