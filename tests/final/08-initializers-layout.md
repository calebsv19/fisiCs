# Initializers & Object Layout

## Scope
Scalar vs aggregate initialization and layout-sensitive rules.

## Validate
- Brace elision and designator reset rules.
- Zero-init for omitted fields.
- Constant expression constraints for static storage.
- Struct/union layout offsets and sizes match target layout.

## Acceptance Checklist
- Scalar and aggregate initializers parse and type-check.
- Nested braces elide correctly and fill omitted fields with zero.
- Designators reset the “current position” as per C99.
- Static storage initializers require constant expressions.

## Test Cases (initial list)
1) `08__scalar_initializers`
   - Basic scalar init and implicit conversions.
2) `08__aggregate_brace_elision`
   - Nested aggregates with omitted braces.
3) `08__designator_reset`
   - Mix of designators and sequential inits.
4) `08__static_constexpr`
   - Invalid non-constant initializer for static storage.
5) `08__array_string_init`
   - char[] initialized from string literal.
6) `08__nested_designators`
   - Designators with reordered indices.
7) `08__zero_init_aggregate`
   - Omitted fields default to zero.
8) `08__string_init_too_long`
   - String initializer exceeds array bounds.
9) `08__layout_offsets`
   - Struct/union layout offsets and sizes via pointer comparisons and `sizeof`.
10) `08__array_excess_elements_reject`
   - Reject too many elements in a fixed-size array initializer.
11) `08__mixed_designated_sequence`
   - Mixed designated/non-designated initializer sequencing.
12) `08__union_designated_init`
   - Union member initialized through designated aggregate initializer.
13) `08__partial_designated_zero_fill`
   - Omitted designated fields are zero-filled.

## Expected Outputs
- AST/Diagnostics goldens for initializer shape and errors.

## Promotion Audit Closure

As of 2026-06-02, bucket `08` is closed against the resolved-probe promotion
audit. The stable final inventory contains `152` bucket tests, including the
probe-backed closure shard
`tests/final/meta/08-initializers-layout-wave69-probe-promotion-audit-closure.json`.

That shard promotes the remaining `10` probe ownership cases for this bucket:
`1` text diagnostic for direct unknown-field designator rejection and `9`
differential runtime cases covering bitfield rewrites, designator overlap
dominance, file-scope pointer/address initializers, and nested union string
member resets. The refreshed audit reports bucket `08` at `promoted=124`,
`probe_only=40`, and `missing_promotion_candidate=0`.

## Probe Backlog
- No open promotion candidates remain in this bucket at the current baseline.
- The remaining `40` resolved probes are explicitly classified as probe-only
  frontier / reduced-threshold coverage by the promotion audit.
