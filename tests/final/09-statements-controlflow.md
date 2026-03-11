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

## Probe Backlog
- `tests/final/probes/diagnostics/09__probe_goto_into_vla_scope_reject.c`
  currently compiles without an error; should reject jumping into VLA scope.
- `tests/final/probes/diagnostics/09__probe_switch_float_condition_reject.c`
  currently accepts non-integer `switch` condition (`float`).
- `tests/final/probes/diagnostics/09__probe_switch_pointer_condition_reject.c`
  currently accepts non-integer `switch` condition (pointer).
- `tests/final/probes/diagnostics/09__probe_switch_string_condition_reject.c`
  currently accepts non-integer `switch` condition (string literal).
- `tests/final/probes/diagnostics/09__probe_switch_double_condition_reject.c`
  currently accepts non-integer `switch` condition (`double`).
- `tests/final/probes/diagnostics/09__probe_if_void_condition_reject.c`
  currently accepts `if` condition with `void` expression.
- `tests/final/probes/diagnostics/09__probe_if_struct_condition_reject.c`
  currently accepts `if` condition with struct expression.
- `tests/final/probes/diagnostics/09__probe_while_void_condition_reject.c`
  currently accepts `while` condition with `void` expression.
- `tests/final/probes/diagnostics/09__probe_do_struct_condition_reject.c`
  currently accepts `do-while` condition with struct expression.
- `tests/final/probes/diagnostics/09__probe_for_void_condition_reject.c`
  currently accepts `for` condition with `void` expression.
- `tests/final/probes/diagnostics/09__probe_for_struct_condition_reject.c`
  currently accepts `for` condition with struct expression.
