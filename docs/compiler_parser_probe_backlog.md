# Compiler Parser Probe Backlog

Last updated: 2026-03-11

Run command:

```bash
python3 tests/final/probes/run_probes.py
```

Current probe summary:
- blocked: `0`
- resolved: `78`
- skipped: `0`

## Remaining Blockers

- None currently. Probe backlog is clear at this snapshot.
- The previously blocked 05-expression probes were fixed and promoted into active suite coverage (`tests/final/meta/05-expressions-wave4.json`).
- `09__probe_do_while_runtime_codegen_crash` now resolves and is promoted into
  active suite as `09__runtime__do_while_side_effects`
  (`tests/final/meta/09-statements-controlflow-wave9.json`).

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
