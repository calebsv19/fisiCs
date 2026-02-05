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

## Expected Outputs
- AST/Diagnostics goldens for statement structure and errors.
