# Lvalues, Rvalues, Addressability

## Scope
Semantic rules for assignability and addressability.

## Validate
- Modifiable lvalue rules.
- Array/function decay, string literal mutability.
- Address-of restrictions (bitfields, register, etc.).

## Acceptance Checklist
- Modifiable lvalue rules enforced (const, non-lvalues).
- Address-of restrictions enforced (bitfields, register, temporary).
- Array/function decay rules applied correctly.
- String literals are not modifiable lvalues.

## Test Cases (initial list)
1) `06__assignability_errors`
   - Assign to const and non-lvalues.
2) `06__address_of_restrictions`
   - & on bitfield/register/temporary.
3) `06__lvalue_edge_cases`
   - Bitfield address-of, const-field assignment, pointer/int compare warning, unsigned conversion.
4) `06__array_function_decay`
   - Array and function decay in expressions.
5) `06__string_literal_mutability`
   - Assign to string literal should error.
6) `06__increment_non_lvalue`
   - ++ applied to non-lvalue should error.

## Expected Outputs
- Diagnostics expectations (`.diag`) for invalid cases.
- AST/diag for decay tests.
