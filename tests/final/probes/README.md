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
  - `runtime/13__probe_phi_continue_runtime.c`
  - `runtime/13__probe_short_circuit_chain_runtime.c`
  - `runtime/13__probe_struct_copy_runtime.c`
  - `runtime/13__probe_ptr_stride_runtime.c`
  - `runtime/13__probe_global_init_runtime.c`
  - `runtime/13__probe_fnptr_ternary_decay_runtime.c`
  - `runtime/14__probe_unsigned_wrap.c`
  - `runtime/14__probe_float_nan.c`
  - `runtime/14__probe_float_cast_roundtrip.c`
  - `runtime/14__probe_many_args_mixed_width.c`
  - `runtime/14__probe_variadic_promotions_matrix.c`
  - `runtime/14__probe_struct_with_array_pass_return.c`
  - `runtime/14__probe_union_payload_roundtrip.c`
  - `runtime/14__probe_fnptr_dispatch_table_mixed.c`
  - `runtime/14__probe_switch_loop_control_mix.c`
  - `runtime/14__probe_vla_stride_indexing.c`
  - `runtime/14__probe_alignment_long_double_struct.c`
  - `runtime/14__probe_struct_array_byte_stride.c`
  - `runtime/14__probe_union_embedded_alignment.c`
  - `runtime/14__probe_vla_row_pointer_decay.c`
  - `runtime/14__probe_nested_switch_fallthrough_loop.c`
  - `runtime/14__probe_short_circuit_side_effect_counter.c`
  - `runtime/14__probe_vla_ptrdiff_row_size_dynamic.c`
  - `runtime/14__probe_vla_param_matrix_reduce.c`
  - `runtime/14__probe_fnptr_struct_by_value_dispatch.c`
  - `runtime/15__probe_switch_loop_lite.c`
  - `runtime/15__probe_switch_loop_mod5.c`
  - `runtime/15__probe_path_decl_nested_runtime.c`
  - `runtime/15__probe_deep_switch_loop_state_machine.c`
  - `runtime/15__probe_switch_sparse_case_jump_table.c`
  - `runtime/15__probe_declarator_depth_runtime_chain.c`
  - `runtime/15__probe_multitu_many_globals_crossref_main.c`
  - `runtime/15__probe_multitu_fnptr_dispatch_grid_main.c`
  - `runtime/15__probe_deep_recursion_stack_pressure.c`
  - `runtime/15__probe_large_vla_stride_pressure.c`
  - `runtime/15__probe_many_args_regstack_pressure.c`
  - `runtime/15__probe_seeded_expr_fuzz_smoke.c`
  - `runtime/15__probe_seeded_stmt_fuzz_smoke.c`
  - `runtime/15__probe_corpus_micro_compile_smoke.c`
  - `runtime/15__probe_control_rebind_dispatch_lattice.c`
  - `runtime/15__probe_large_struct_array_checksum_grid.c`
  - `runtime/15__probe_multitu_const_table_crc_main.c`
- Diagnostics:
  - `diagnostics/01__probe_line_directive_virtual_line_spelling_reject.c`
  - `diagnostics/01__probe_line_directive_virtual_macro_filename_spelling_reject.c`
  - `diagnostics/01__probe_line_mapping_real_file_baseline.c`
  - `diagnostics/01__probe_line_directive_virtual_line_nonvoid_return_location_reject.c`
  - `diagnostics/01__probe_line_directive_virtual_line_nonvoid_return_current_zerozero.c`
  - `diagnostics/01__probe_line_directive_virtual_line_undeclared_identifier_location_reject.c`
  - `diagnostics/01__probe_nonvoid_return_plain_current_zerozero.c`
  - `diagnostics/01__probe_line_directive_macro_nonvoid_return_location_reject.c`
  - `diagnostics/01__probe_line_directive_macro_nonvoid_return_current_zerozero.c`
  - `diagnostics/02__probe_lexer_line_directive_invalid_dollar_location_reject.c`
  - `diagnostics/02__probe_lexer_line_directive_invalid_dollar_current_physical_line.c`
  - `diagnostics/02__probe_lexer_line_directive_unterminated_string_location_reject.c`
  - `diagnostics/02__probe_lexer_line_directive_unterminated_string_current_physical_line.c`
  - `diagnostics/02__probe_lexer_line_directive_char_invalid_hex_escape_location_reject.c`
  - `diagnostics/02__probe_lexer_line_directive_char_invalid_hex_escape_current_physical_line.c`
  - `diagnostics/02__probe_lexer_line_directive_string_invalid_escape_location_reject.c`
  - `diagnostics/02__probe_lexer_line_directive_string_invalid_escape_current_physical_line.c`
  - `diagnostics/02__probe_lexer_line_directive_invalid_at_location_reject.c`
  - `diagnostics/02__probe_lexer_line_directive_invalid_at_current_physical_line.c`
  - `diagnostics/02__probe_lexer_line_directive_invalid_backtick_location_reject.c`
  - `diagnostics/02__probe_lexer_line_directive_invalid_backtick_current_physical_line.c`
  - `diagnostics/02__probe_lexer_line_directive_unterminated_char_location_reject.c`
  - `diagnostics/02__probe_lexer_line_directive_unterminated_char_current_physical_line.c`
  - `diagnostics/01__probe_diagjson_line_directive_macro_line_map_strict.c`
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
  - `diagnostics/13__probe_fnptr_too_many_args_reject.c`
  - `diagnostics/13__probe_fnptr_too_few_args_reject.c`
  - `diagnostics/13__probe_mod_float_reject.c`
  - `diagnostics/13__probe_fnptr_assign_incompatible_reject.c`
  - `diagnostics/13__probe_fnptr_nested_qualifier_loss_reject.c`
  - `diagnostics/13__probe_voidptr_to_fnptr_assign_reject.c`
  - `diagnostics/13__probe_fnptr_to_voidptr_assign_reject.c`
  - `diagnostics/13__probe_fnptr_nested_volatile_qualifier_loss_reject.c`
  - `diagnostics/13__probe_fnptr_deep_const_qualifier_loss_reject.c`
  - Runner-only diag-json probes using the same fixtures:
    `01__probe_diagjson_line_directive_macro_line_map_strict`,
    `01__probe_diagjson_line_directive_nonvoid_return_location_reject`,
    `01__probe_diagjson_line_directive_undeclared_identifier_location_strict`,
    `01__probe_diagjson_nonvoid_return_plain_current_zerozero`,
    `01__probe_diagjson_line_directive_macro_nonvoid_return_location_reject`,
    `01__probe_diagjson_line_directive_macro_nonvoid_return_current_zerozero`,
    `02__probe_diagjson_lexer_line_directive_invalid_dollar_location_reject`,
    `02__probe_diagjson_lexer_line_directive_invalid_dollar_current_physical_line`,
    `02__probe_diagjson_lexer_line_directive_unterminated_string_location_reject`,
    `02__probe_diagjson_lexer_line_directive_unterminated_string_current_physical_line`,
    `02__probe_diagjson_lexer_line_directive_char_invalid_hex_escape_location_reject`,
    `02__probe_diagjson_lexer_line_directive_char_invalid_hex_escape_current_physical_line`,
    `02__probe_diagjson_lexer_line_directive_string_invalid_escape_location_reject`,
    `02__probe_diagjson_lexer_line_directive_string_invalid_escape_current_physical_line`,
    `02__probe_diagjson_lexer_line_directive_invalid_at_location_reject`,
    `02__probe_diagjson_lexer_line_directive_invalid_at_current_physical_line`,
    `02__probe_diagjson_lexer_line_directive_invalid_backtick_location_reject`,
    `02__probe_diagjson_lexer_line_directive_invalid_backtick_current_physical_line`,
    `02__probe_diagjson_lexer_line_directive_unterminated_char_location_reject`,
    `02__probe_diagjson_lexer_line_directive_unterminated_char_current_physical_line`,
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
