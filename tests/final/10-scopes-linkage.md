# Scoping, Symbol Table, Linkage

## Scope
Namespaces, redeclarations, and linkage behavior.

## Validate
- Block vs file scope shadowing.
- typedef namespace vs identifiers vs tags.
- Tentative definitions and extern rules.

## Acceptance Checklist
- Block scope shadows file scope where allowed.
- typedef names do not collide with tags and ordinary identifiers.
- Tentative definitions are accepted and resolved.
- Incompatible redeclarations are diagnosed.

## Test Cases (initial list)
1) `10__shadowing_rules`
   - Block scope shadows file scope.
2) `10__typedef_tag_namespace`
   - typedef vs struct tag namespace separation.
3) `10__tentative_definition`
   - Multiple tentative defs and one definition.
4) `10__redeclaration_mismatch`
   - Incompatible redeclaration diagnostic.
5) `10__extern_static_mismatch`
   - Extern vs static linkage conflict.
6) `10__conflicting_extern_definition`
   - extern declaration with definition consistency.
7) `10__multiple_static_defs`
   - Multiple static definitions in one TU.
8) `10__extern_type_mismatch`
   - extern declarations with conflicting types.
9) `10__function_redecl_conflict`
   - Function redeclaration incompatibility.
10) `10__tentative_static_conflict`
   - Tentative definition conflicts with static.
11) `10__extern_array_mismatch`
   - extern array size mismatch.
12) `10__tentative_definition_multi_tu`
   - Multiple tentative definitions across translation units link cleanly.
13) `10__block_extern_reference`
   - Block-scope `extern` binds to file-scope symbol while inner shadow remains local.
14) `10__block_scope_conflicting_types`
   - Redeclaration with conflicting type in same block scope is rejected.
15) `10__var_function_name_conflict`
   - Variable/function name collision in ordinary identifier namespace is rejected.
16) `10__extern_array_consistent_definition`
   - File-scope `extern` declaration matched by consistent array definition.

## Expected Outputs
- AST/Diagnostics goldens for scope and linkage behavior.

## Probe Backlog
- `tests/final/probes/diagnostics/10__probe_block_extern_different_type_reject.c`
  currently accepts block-scope `extern` redeclaration with conflicting type.
- `tests/final/probes/runtime/10__probe_static_function_then_extern_decl_ok.c`
  currently rejects valid file-scope `static` function followed by `extern`
  declaration.
