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
13) `12__diag_type_mismatch`
   - Non-scalar to scalar assignment reports type mismatch.
14) `12__diag_undeclared_identifier`
   - Undeclared identifier diagnostic.
15) `12__diag_redeclaration_conflict`
   - Conflicting redeclaration diagnostic.
16) `12__diag_invalid_storage_class`
   - Invalid storage class usage at file scope.
17) `12__diag_invalid_initializer`
   - Invalid scalar initializer diagnostics.
18) `12__diag_invalid_shift_width`
   - Constant shift-width constraint diagnostics (`< 0` or `>=` promoted LHS width).

## Wave 2 Additions
19) `12__diag_incompatible_ptr`
   - Incompatible pointer assignment (`int* = double*`) reports constraint diagnostic.
20) `12__diag_illegal_cast`
   - Illegal cast from non-scalar compound literal to scalar reports diagnostic.
21) `12__recovery__if_missing_rparen_then_followup_stmt`
   - Missing `)` in `if` condition recovers to following statement.
22) `12__recovery__while_missing_lparen_then_followup_stmt`
   - Missing `(` in `while` condition recovers to following statement.
23) `12__recovery__for_header_missing_semicolon_then_followup_decl`
   - Missing `;` in `for` header recovers and later declaration remains visible.
24) `12__recovery__do_while_missing_semicolon_then_followup_stmt`
   - Missing trailing `;` after `do-while` recovers to following statement.
25) `12__recovery__switch_case_missing_colon_then_followup_case`
   - Missing `:` after `case` label recovers to later `case/default`.
26) `12__recovery__switch_bad_case_expr_then_default`
   - Malformed `case` expression recovers to `default` arm.
27) `12__recovery__goto_missing_label_token_then_followup_label`
   - Missing label token after `goto` recovers to subsequent label block.
28) `12__recovery__case_outside_switch_then_next_stmt_ok`
   - `case` outside switch is diagnosed and recovery continues to next statement.

## Wave 3 Additions
29) `12__diag_line_anchor__undeclared_calls`
   - Two undeclared callsites anchor to their exact source lines in encounter order.
30) `12__diag_order__nonvoid_return_then_unreachable`
   - Primary error (`return;` in non-void function) appears before follow-on unreachable warning.
31) `12__diag_cascade_bound__single_unreachable_region`
   - One dead region with multiple following statements emits a bounded unreachable warning.
32) `12__diag_line_anchor__ptr_then_cast`
   - Sequential constraint failures keep stable line anchoring and emission order.
33) `12__diag_line_anchor__macro_two_callsites`
   - Macro-site diagnostics de-dup per callsite (two expansions => two anchored diagnostics).

## Wave 4 Additions (Parser Diagnostic Export Path)
34) `12__parserdiag__missing_semicolon`
   - Parser diagnostic export includes expected-semicolon + parser-generic codes at the callsite.
35) `12__parserdiag__for_missing_paren`
   - Parser diagnostic export anchors missing-`)` style parse failure in `for` header.
36) `12__parserdiag__bad_paren_expr`
   - Parser diagnostic export captures repeated parser-generic diagnostics for bad parenthesized expression.
37) `12__parserdiag__stray_else_recovery`
   - Parser diagnostic export captures stray-`else` parser-generic anchor.

## Wave 5 Additions (Parser Recovery Expansion)
38) `12__parserdiag__recovery_if_missing_rparen`
   - Parser diagnostic tuples for malformed `if` condition with follow-up statement recovery.
39) `12__parserdiag__recovery_switch_case_missing_colon`
   - Parser diagnostic tuples for missing `:` after `case` with continued switch recovery.
40) `12__parserdiag__recovery_switch_bad_case_expr`
   - Parser diagnostic tuples for malformed `case` expression with later `default` recovery.
41) `12__parserdiag__recovery_goto_missing_label`
   - Parser diagnostic tuples for `goto` missing label token and follow-up label recovery.
42) `12__parserdiag__recovery_case_outside_switch`
   - Parser diagnostic tuples for `case` outside switch with next-statement continuation.

## Wave 6 Additions (Mixed Parser + Semantic Ordering Expansion)
43) `12__parserdiag__mixed__if_missing_rparen_then_undeclared`
   - Parser diagnostic tuples for malformed `if` plus follow-on undeclared identifiers.
44) `12__parserdiag__mixed__bad_call_then_incompatible_ptr`
   - Parser diagnostic tuples for malformed call plus later pointer-compatibility diagnostics.
45) `12__parserdiag__mixed__switch_case_missing_colon_then_undeclared`
   - Parser diagnostic tuples for malformed `case` syntax plus follow-on semantic undeclared use.
46) `12__parserdiag__mixed__ternary_missing_colon_then_type_mismatch`
   - Parser diagnostic tuples for malformed ternary with subsequent semantic type mismatch.

## Wave 7 Additions (Parser Recovery Export Promotion)
47) `12__parserdiag__recovery_while_missing_lparen`
   - Parser diagnostic tuples for malformed `while` (missing `(`) are now exported and asserted.
48) `12__parserdiag__recovery_do_while_missing_semicolon`
   - Parser diagnostic tuples for malformed `do-while` (missing trailing `;`) are now exported and asserted.

## Expected Outputs
- AST and diagnostics expectations (`.ast` + `.diag`) for deterministic recovery checks.
- Parser diagnostics expectations (`.pdiag`) via `--emit-diags-json` export path, normalized to parser diagnostic tuples (`code`, `line`, `column`, `length`, `kind`).

## Open Gaps (Tracked)
- Active suite is stable for current promoted waves, including new fail-closed
  harness assertions that capture frontend parser diagnostics for selected
  malformed-recovery files (`capture_frontend_diag: true` in metadata).
- Diagnostics JSON export blockers are now resolved for statement-recovery
  parser errors:
  `12__probe_diagjson_while_missing_lparen` and
  `12__probe_diagjson_do_while_missing_semicolon` now both resolve.
- Mixed parser+semantic ordering anchors remain active for cross-error files.
