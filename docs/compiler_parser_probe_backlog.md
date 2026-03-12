# Compiler Parser Probe Backlog

Last updated: 2026-03-11

Run command:

```bash
python3 tests/final/probes/run_probes.py
```

Current probe summary:
- blocked: `0`
- resolved: `98`
- skipped: `0`

## Remaining Blockers

- None currently.

Recently resolved:
- `12__probe_diagjson_while_missing_lparen`
- `12__probe_diagjson_do_while_missing_semicolon`

Fix landed in parser statement handling:
- missing `(` after `while` and missing `;` after `do-while` now record parser
  diagnostics via `compiler_report_diag` (without changing existing CLI text),
  so `--emit-diags-json` now serializes these errors instead of returning an
  empty diagnostics array.
- The previously blocked 05-expression probes were fixed and promoted into active suite coverage (`tests/final/meta/05-expressions-wave4.json`).
- `09__probe_do_while_runtime_codegen_crash` now resolves and is promoted into
  active suite as `09__runtime__do_while_side_effects`
  (`tests/final/meta/09-statements-controlflow-wave9.json`).
- New 09 parser-shape probes are now added and currently resolved:
  `09__probe_if_missing_then_stmt_reject`,
  `09__probe_else_missing_stmt_reject`,
  `09__probe_switch_missing_rparen_reject`,
  `09__probe_for_missing_first_semicolon_reject`,
  `09__probe_goto_undefined_label_nested_reject`.
- New 04 parser-shape declaration probes are now added and currently resolved:
  `04__probe_struct_member_missing_type_reject`,
  `04__probe_bitfield_missing_colon_reject`,
  `04__probe_enum_missing_rbrace_reject`,
  `04__probe_typedef_missing_identifier_reject`,
  `04__probe_declarator_unbalanced_parens_reject`.
- New 04 runtime/complex probes:
  `04__probe_union_overlap_runtime` (resolved),
  `04__probe_complex_int_reject` (resolved),
  `04__probe_complex_unsigned_reject` (resolved),
  `04__probe_complex_missing_base_reject` (resolved).

Latest additions (expressions diagnostics):
- `05__probe_shift_width_large_reject`
- `05__probe_bitwise_float_reject`
- `05__probe_relational_struct_reject`
- `05__probe_equality_struct_reject`
- `05__probe_add_void_reject`

Latest additions (expressions runtime):
- `05__probe_precedence_runtime`
- `05__probe_unsigned_arith_conv_runtime`
- `05__probe_nested_ternary_runtime`
- `05__probe_nested_ternary_outer_true_runtime`
- `05__probe_nested_ternary_false_chain_runtime`
- `05__probe_short_circuit_and_runtime`
- `05__probe_short_circuit_or_runtime`
- `05__probe_comma_eval_runtime`
- `05__probe_vla_sizeof_side_effect_runtime`
- `05__probe_ternary_side_effect_runtime`
- `05__probe_nested_ternary_deep_false_chain_runtime`
- `05__probe_compound_literal_array_runtime`

Latest additions (expressions diagnostics):
- `05__probe_alignof_void_reject`
- `05__probe_alignof_incomplete_reject`
- `05__probe_unary_bitnot_float_reject`
- `05__probe_unary_plus_struct_reject`
- `05__probe_unary_minus_pointer_reject`
- `05__probe_sizeof_void_reject`
- `05__probe_alignof_expr_reject`
