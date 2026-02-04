# Type System & Conversions

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

## Expected Outputs
- AST/Diagnostics goldens for type conversion behavior.
