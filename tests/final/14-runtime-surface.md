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

## Expected Outputs
- AST/Diagnostics goldens for header use.
