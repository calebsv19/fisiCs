# Torture / Differential

## Scope
Stress parser, semantic, and runtime paths with large or pathological shapes.

## Validate
- Long-expression parsing and lowering under heavy operator chains.
- Deeply nested statement handling.
- Many declarations in a single scope.
- Large aggregate layout and access behavior.
- Pathological declarator parsing in non-trivial function-pointer forms.
- Malformed input robustness (diagnose, fail closed, no crash).

## Acceptance Checklist
- Runtime stress tests compile, execute, and match expected stdout/stderr.
- Differential checks against clang run when available.
- ABI-sensitive checks are tagged explicitly.
- Malformed input tests report deterministic diagnostics and do not crash.

## Test Cases (initial list)
1) `15__torture__long_expr`
   - 100-term arithmetic expression runtime result.
2) `15__torture__deep_nesting`
   - 12-level nested `if` chain runtime result.
3) `15__torture__many_decls`
   - High local declaration count and aggregate usage.
4) `15__torture__many_globals`
   - High global declaration density and runtime aggregate usage.
5) `15__torture__many_params`
   - Multi-parameter call ABI path stress.
6) `15__torture__large_struct`
   - Larger struct field layout/readback runtime behavior.
7) `15__torture__large_array_stress`
   - Larger local-array and indexed-access stress path.
8) `15__torture__pathological_decl`
   - Complex function-pointer typedef/declarator parse path.
9) `15__torture__path_decl_nested`
   - Runtime nested function-pointer declarator/call path (`choose` + indirect call).
10) `15__torture__malformed_no_crash`
   - Invalid source characters/syntax produce diagnostics without crashing.
11) `15__torture__malformed_unclosed_comment_no_crash`
   - Unterminated block-comment path reports lexer diagnostic without crashing.
12) `15__torture__malformed_unbalanced_block_no_crash`
   - Unbalanced block recovery path reports parser diagnostics without crashing.

## Open Gaps (Tracked)
- `15__torture__deep_switch` probe currently mismatches clang runtime output
  (`fisics=15`, `clang=60`) and is tracked for control-flow fix work.

## Expected Outputs
- Runtime stdout/stderr/exit expectations for executable stress tests.
- AST/diagnostic expectations for compile-surface stress tests.
