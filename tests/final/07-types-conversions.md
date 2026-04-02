# Type System & Conversions

## Status Snapshot (2026-04-01)

- Active tests: `16`
  (`tests/final/meta/07-types-conversions.json` +
  `tests/final/meta/07-types-conversions-wave2-semantic.json`)
- Current bucket run:
  - `make final-prefix PREFIX=07__` -> all pass
  - `PROBE_FILTER=07__probe_*` -> `resolved=7`, `blocked=0`, `skipped=0`

## Scope
Integer promotions, usual arithmetic conversions, and pointer rules.

## Validate
- Promotions for integer types and _Bool.
- Usual arithmetic conversions for mixed signedness.
- Pointer arithmetic scaling and comparisons.

## Acceptance Checklist
- Integer promotions and usual arithmetic conversions are applied.
- Mixed signed/unsigned comparisons follow C99 rules.
- Pointer arithmetic scales by element size.
- Pointer comparisons and null checks are accepted with correct types.

## Test Cases (initial list)
1) `07__integer_promotions`
   - _Bool/char/short promoted in expressions.
2) `07__usual_arith_conversions`
   - signed/unsigned mix with long/ulong.
3) `07__pointer_arithmetic`
   - ptr +/- int and ptr - ptr.
4) `07__pointer_comparisons`
   - null pointer checks and relational comparisons.
5) `07__mixed_signed_unsigned_compare`
   - signed/unsigned comparison rules.
6) `07__array_decay_param`
   - Array parameter usage validates array-to-pointer decay.
7) `07__function_decay_call`
   - Function designator used via function pointer parameter.
8) `07__void_pointer_roundtrip`
   - Object pointer to `void*` and back.
9) `07__void_pointer_arith_reject`
   - `void*` arithmetic rejected as invalid.

## Expected Outputs
- AST/Diagnostics goldens for type conversion behavior.

## Probe Backlog
- No open probes in this bucket at the current baseline.

## Wave 2 Additions (Semantic Expansion)
1) `07__assign_struct_to_int_reject`
   - Reject assigning struct value to scalar int object.
2) `07__agg__member_access`
   - Nested member access using both `.` and `->`.
3) `07__agg__offsets`
   - Aggregate member-offset ordering via `offsetof`.
4) `07__agg__invalid_member_reject`
   - Reject unknown struct member access.
5) `07__constexpr__array_size_reject`
   - Reject non-constant file-scope array size.
6) `07__constexpr__case_label_reject`
   - Reject non-constant `case` label expression.
7) `07__constexpr__static_init_reject`
   - Reject non-constant static-storage initializer.
