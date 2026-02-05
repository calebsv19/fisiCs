# Diagnostics & Error Recovery

## Scope
Recovery points and diagnostic quality for IDE usability.

## Validate
- Synchronization at ;, }, and ) where appropriate.
- Suppress cascaded errors after a primary failure.
- Useful error context (expected vs actual tokens).

## Acceptance Checklist
- Parser recovers at statement boundaries and continues.
- Diagnostics include useful location info and messages.
- Cascading error counts are controlled.

## Test Cases (initial list)
1) `12__missing_semicolon`
   - Recover after missing ; in declaration.
2) `12__bad_paren_expr`
   - Unbalanced parentheses in expression.
3) `12__bad_decl_recovery`
   - Malformed declaration followed by valid one.
4) `12__bad_stmt_recovery`
   - Malformed statement followed by valid one.
5) `12__extra_paren_recovery`
   - Extra ')' recovers to following statement.
6) `12__missing_rbrace_recovery`
   - Missing '}' recovers at end of file.
7) `12__cascading_errors`
   - Multiple nearby errors do not cascade excessively.
8) `12__typedef_recovery`
   - Malformed typedef followed by valid declaration.
9) `12__stray_else_recovery`
   - Stray else recovers to following statement.
10) `12__for_missing_paren`
   - for missing ')' recovers.
11) `12__decl_init_recovery`
   - Missing initializer recovers and subsequent declaration remains visible.
12) `12__macro_callsite_dedup`
   - Macro expansion diagnostics point to call site and de-dup identical errors.

## Expected Outputs
- Diagnostics expectations (`.diag`) for error cases.
