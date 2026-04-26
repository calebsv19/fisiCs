# Type System & Conversions

This bucket covers C99 type semantics that sit between expression parsing and
runtime behavior.

## Scope

Use bucket `07` for:

- integer promotions and usual arithmetic conversions
- signed/unsigned comparison behavior
- pointer arithmetic and pointer-difference rules
- null-pointer constants and pointer compatibility
- constant-expression legality when the main question is conversion or type
  interpretation

## Current Use

- focused bucket run: `make final-bucket BUCKET=types-conversions`
- focused ids: `make final-id ID=<07__test_id>`
- broader integration checkpoint: `make final`

The probe lane exists for pre-promotion reduction work only. Public stable use
should route through the canonical `tests/final/` bucket commands above.

## Acceptance Checklist

- promotions and usual arithmetic conversions follow C99 rules
- mixed signed/unsigned comparisons behave correctly
- pointer arithmetic scales by element size
- pointer subtraction and comparisons are type-correct
- invalid conversion cases fail closed with deterministic diagnostics
- constant-expression conversion rules stay stable where this bucket owns them

## Representative Coverage Areas

- `_Bool`, `char`, `short`, and integer-promotion paths
- signed/unsigned boundary comparisons
- pointer-to-`void *` compatibility and round-trips
- pointer arithmetic and pointer-difference scaling
- aggregate/member conversion edge cases
- constexpr legality tied to conversion/type interpretation
- diagnostics JSON and text parity for conversion rejections

## Boundary With Neighbor Buckets

- use bucket `05` when the core question is expression parsing or operator
  semantics
- use bucket `06` when the core question is lvalue/modifiable-lvalue behavior
- use bucket `08` when the core question is initializer legality or object
  layout
- use bucket `14` when runtime execution is the main truth signal

## Public Note

This file is intentionally a stable public bucket reference.

Wave-by-wave expansion history, probe-triage batches, and active maintainer
sequencing belong in private maintainer docs rather than this bucket summary.
