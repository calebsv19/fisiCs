# Statements & Control Flow

## Scope
Statement grammar, scoping, and control flow legality.

## Validate
- Dangling-else binding and switch semantics.
- for-declarations (C99), break/continue legality.
- goto and label restrictions (including redefinition).

## Acceptance Checklist
- Dangling else binds to the nearest if.
- switch/case/default parse and scope correctly.
- for-declarations are accepted and scoped.
- break/continue legality enforced.
- goto/labels are parsed and resolved.

## Test Cases (initial list)
1) `09__dangling_else`
   - else binds to nearest if.
2) `09__switch_rules`
   - switch with case/default and fallthrough.
3) `09__for_decl_scope`
   - for with declaration and scope rules.
4) `09__break_continue_errors`
   - break/continue used outside loops.
5) `09__goto_labels`
   - label usage and goto resolution.
6) `09__switch_duplicate_case`
   - Duplicate case label diagnostics.
7) `09__goto_cross_init`
   - goto crossing initialization boundaries.
8) `09__label_redefinition`
   - duplicate label warning.
9) `09__while_loop_basic`
   - baseline `while` loop control flow.
10) `09__do_while_loop_basic`
   - baseline `do-while` execution shape.
11) `09__for_forms`
   - `for` variants including omitted clauses.
12) `09__switch_nonconst_case`
   - non-constant `case` label rejected.

## Expected Outputs
- AST/Diagnostics goldens for statement structure and errors.

## Wave 2 Additions
13) `09__stmt__expr`
   - Expression statement and empty statement baseline.
14) `09__stmt__compound`
   - Nested compound block statement baseline.
15) `09__stmt__return_basic`
   - Basic `return` statement baseline in helper + caller flow.
16) `09__switch_multiple_default_reject`
   - Reject duplicate `default` labels in a switch.
17) `09__continue_in_switch_loop`
   - `continue` inside `switch` nested in loop targets loop continue path.
18) `09__switch_case_range_reject`
   - Reject GNU case-range syntax (`case 1 ... 3`) as unsupported.

## Wave 3 Additions
19) `09__if_decl_condition_reject`
   - Reject C++-style declaration in `if` condition.
20) `09__while_decl_condition_reject`
   - Reject declaration in `while` condition.
21) `09__switch_decl_condition_reject`
   - Reject declaration in `switch` controlling expression.
22) `09__case_outside_switch_reject`
   - Reject `case` label outside a switch statement.
23) `09__default_outside_switch_reject`
   - Reject `default` label outside a switch statement.

## Wave 4 Additions
24) `09__do_while_missing_semicolon_reject`
   - Reject missing trailing semicolon in `do-while` statements.
25) `09__switch_case_decl_block`
   - Confirm declaration after `case` is accepted when wrapped in a block statement.

## Wave 5 Additions
26) `09__goto_undefined_label_reject`
   - Reject `goto` references to labels that do not exist in function scope.
27) `09__label_before_decl_reject`
   - Reject ordinary labels directly before declarations (requires statement).
28) `09__switch_case_decl_no_block_reject`
   - Reject `case` labels directly before declarations unless wrapped by a block.

## Wave 6 Additions
29) `09__continue_in_switch_reject`
   - Reject `continue` inside `switch` when no enclosing loop exists.
30) `09__switch_duplicate_case_folded_reject`
   - Reject duplicate `case` values after constant-expression folding.

## Wave 7 Additions
31) `09__diag__goto_into_vla_scope_reject`
   - Reject `goto` jumping into VLA scope.
32) `09__diag__switch_float_condition_reject`
   - Reject `switch` controlling expression of type `float`.
33) `09__diag__switch_pointer_condition_reject`
   - Reject `switch` controlling expression of pointer type.
34) `09__diag__switch_string_condition_reject`
   - Reject `switch` controlling expression as string literal.
35) `09__diag__switch_double_condition_reject`
   - Reject `switch` controlling expression of type `double`.
36) `09__diag__if_void_condition_reject`
   - Reject `if` controlling expression of type `void`.
37) `09__diag__if_struct_condition_reject`
   - Reject `if` controlling expression of non-scalar struct type.
38) `09__diag__while_void_condition_reject`
   - Reject `while` controlling expression of type `void`.
39) `09__diag__do_struct_condition_reject`
   - Reject `do-while` controlling expression of non-scalar struct type.
40) `09__diag__for_void_condition_reject`
   - Reject `for` controlling expression of type `void`.
41) `09__diag__for_struct_condition_reject`
   - Reject `for` controlling expression of non-scalar struct type.

## Wave 8 Additions
42) `09__runtime__switch_fallthrough_order`
   - Runtime/differential switch fallthrough order validation.
43) `09__runtime__nested_loop_switch_control`
   - Runtime/differential nested loop + switch `break`/`continue` behavior.
44) `09__runtime__while_for_side_effects`
   - Runtime/differential side-effect flow through `while` and `for`.
45) `09__runtime__goto_forward_backward`
   - Runtime/differential forward and backward `goto` label flow.
46) `09__runtime__switch_default_selection`
   - Runtime/differential default-vs-case selection behavior.

## Wave 9 Additions
47) `09__runtime__do_while_side_effects`
   - Runtime/differential `do-while` side-effect and condition re-evaluation behavior.

## Probe Backlog
- No open probes in this bucket at the current baseline.
- Probe-to-active promotions completed in wave 7 for the full 09 diagnostics probe set.
- `09__probe_do_while_runtime_codegen_crash` now resolves and its behavior is promoted
  into active suite via `09__runtime__do_while_side_effects`.

## Next Expansion Targets (Post-Cleanup Plan)
1) Add control-flow edge diagnostics not yet covered
   - `goto` to missing label across nested scopes (complex multi-label shape).
   - duplicate `default` + folded duplicate `case` in mixed constant-expression forms.
   - label/statement boundary edge cases in nested blocks and after empty statements.

2) Add recovery-focused malformed-statement checks (09 + 12 boundary)
   - malformed loop headers followed by valid statements (recover and continue).
   - malformed `switch` body with later valid `case`/`default` recovery.
   - verify diagnostic category + line anchors remain stable under recovery.
