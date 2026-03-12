# Final Probe Fixtures (Runtime/Diagnostic Repros)

These probe fixtures are **not** part of `make final`.

Purpose:
- Keep high-risk or historically fragile behaviors reproducible.
- Provide a small, fixed input set for fix-phase verification.
- Keep differential/runtime/diagnostic checks fast while iterating on parser/semantic/codegen fixes.

## Layout
- `runtime/`: runtime differential repros (`fisics` vs `clang`).
- `diagnostics/`: diagnostics/constraint repros.
- `run_probes.py`: runs all probes and prints current status.

## Current Focus Probes
- Runtime:
  - `runtime/04__probe_fnptr_array_call_runtime.c`
  - `runtime/04__probe_tag_block_shadow_ok.c`
  - `runtime/04__probe_deep_declarator_call_only.c`
  - `runtime/04__probe_deep_declarator_codegen_hang.c`
  - `runtime/04__probe_union_overlap_runtime.c`
  - `runtime/05__probe_typedef_shadow_parenthesized_expr.c`
  - `runtime/09__probe_do_while_runtime_codegen_crash.c`
  - `runtime/10__probe_static_function_then_extern_decl_ok.c`
  - `runtime/11__probe_function_param_function_type_adjust_ok.c`
  - `runtime/14__probe_unsigned_wrap.c`
  - `runtime/14__probe_float_nan.c`
  - `runtime/15__probe_switch_loop_lite.c`
  - `runtime/15__probe_switch_loop_mod5.c`
  - `runtime/15__probe_path_decl_nested_runtime.c`
- Diagnostics:
  - `diagnostics/04__probe_block_extern_initializer_reject.c`
  - `diagnostics/04__probe_param_extern_reject.c`
  - `diagnostics/04__probe_param_static_nonarray_reject.c`
  - `diagnostics/04__probe_param_void_named_reject.c`
  - `diagnostics/04__probe_enum_const_typedef_conflict_reject.c`
  - `diagnostics/04__probe_enum_const_var_conflict_reject.c`
  - `diagnostics/04__probe_tag_cross_kind_conflict_reject.c`
  - `diagnostics/04__probe_struct_member_missing_type_reject.c`
  - `diagnostics/04__probe_bitfield_missing_colon_reject.c`
  - `diagnostics/04__probe_enum_missing_rbrace_reject.c`
  - `diagnostics/04__probe_typedef_missing_identifier_reject.c`
  - `diagnostics/04__probe_declarator_unbalanced_parens_reject.c`
  - `diagnostics/04__probe_complex_int_reject.c`
  - `diagnostics/04__probe_complex_unsigned_reject.c`
  - `diagnostics/04__probe_complex_missing_base_reject.c`
  - `diagnostics/05__probe_conditional_void_condition_reject.c`
  - `diagnostics/05__probe_conditional_struct_result_reject.c`
  - `diagnostics/05__probe_logical_and_struct_reject.c`
  - `diagnostics/05__probe_unary_not_struct_reject.c`
  - `diagnostics/07__probe_assign_struct_to_int_reject.c`
  - `diagnostics/08__probe_designator_unknown_field_reject.c`
  - `diagnostics/09__probe_goto_into_vla_scope_reject.c`
  - `diagnostics/09__probe_switch_float_condition_reject.c`
  - `diagnostics/09__probe_switch_pointer_condition_reject.c`
  - `diagnostics/09__probe_switch_string_condition_reject.c`
  - `diagnostics/09__probe_switch_double_condition_reject.c`
  - `diagnostics/09__probe_if_void_condition_reject.c`
  - `diagnostics/09__probe_if_struct_condition_reject.c`
  - `diagnostics/09__probe_while_void_condition_reject.c`
  - `diagnostics/09__probe_do_struct_condition_reject.c`
  - `diagnostics/09__probe_for_void_condition_reject.c`
  - `diagnostics/09__probe_for_struct_condition_reject.c`
  - `diagnostics/09__probe_if_missing_then_stmt_reject.c`
  - `diagnostics/09__probe_else_missing_stmt_reject.c`
  - `diagnostics/09__probe_switch_missing_rparen_reject.c`
  - `diagnostics/09__probe_for_missing_first_semicolon_reject.c`
  - `diagnostics/09__probe_goto_undefined_label_nested_reject.c`
  - `diagnostics/10__probe_block_extern_different_type_reject.c`
  - `diagnostics/11__probe_duplicate_param_name_reject.c`
  - `diagnostics/11__probe_param_auto_reject.c`
  - `diagnostics/11__probe_param_void_plus_other_reject.c`
  - `diagnostics/11__probe_param_incomplete_type_reject.c`
  - `diagnostics/11__probe_return_void_expr_in_int_reject.c`
  - `diagnostics/12__probe_invalid_shift_width.c`
  - `diagnostics/12__probe_incompatible_ptr_assign.c`
  - `diagnostics/12__probe_illegal_struct_to_int_cast.c`
  - `diagnostics/12__probe_while_missing_lparen_reject.c`
  - `diagnostics/12__probe_do_while_missing_semicolon_reject.c`
  - `diagnostics/12__probe_for_header_missing_semicolon_reject.c`
  - Runner-only diag-json probes using the same fixtures:
    `12__probe_diagjson_while_missing_lparen`,
    `12__probe_diagjson_do_while_missing_semicolon`,
    `12__probe_diagjson_for_header_missing_semicolon`

## Run
```bash
python3 tests/final/probes/run_probes.py
```

Run only a subset by id/prefix:

```bash
PROBE_FILTER=09__probe_ python3 tests/final/probes/run_probes.py
```

`run_probes.py` prints whether each probe currently resolves or is blocked.
