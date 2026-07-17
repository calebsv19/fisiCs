"""Explicit stable owners assigned by the 2026-07 promotion-closure pass.

This map is intentionally limited to frozen probe-only records whose live
oracle is already represented by an equivalent stable final test.  Keeping the
mapping separate avoids cloning stable cases merely to make inventory paths
match.
"""

PROMOTION_CLOSURE_OWNERS = {
    # Buckets 04-05: direct owners assigned during the first closure slices.
    "04__probe_wave38_nested_oldstyle_float_fnptr_conflict":
        "04__diag__wave47_nested_oldstyle_float_fnptr_conflict",
    "04__probe_wave38_nested_void_int_fnptr_conflict":
        "04__diag__wave47_nested_void_int_fnptr_conflict",
    "04__probe_wave38_nested_void_oldstyle_fnptr_compatible":
        "04__runtime__wave47_nested_void_oldstyle_fnptr_compatible",
    "04__probe_deep_declarator_typedef_factory_runtime":
        "04__declarator__factory_call_typedef_runtime",
    "04__probe_deep_declarator_typedef_factory_assignment_runtime":
        "04__declarator__factory_assignment_typedef_runtime",
    "05__probe_line_directive_unary_minus_ptr_reduced_location_pass":
        "05__line_directive_unary_minus_ptr_diag_line_map",
    "05__probe_diagjson_line_directive_alignof_expr_file_presence_reject":
        "05__diagjson__line_directive_alignof_expr_current_sparse_pass",
    "05__probe_diagjson_line_directive_alignof_expr_reduced_location_pass":
        "05__line_directive_alignof_expr_diagjson_line_map",
    "05__probe_diagjson_line_directive_include_alignof_expr_file_presence_reject":
        "05__diagjson__line_directive_include_alignof_expr_current_sparse_pass",
    "05__probe_diagjson_line_directive_include_macro_add_rich_presence_strict":
        "05__line_directive_include_macro_add_diagjson_rich_location",
    "05__probe_diagjson_line_directive_include_shift_width_file_presence_reject":
        "05__diagjson__line_directive_include_shift_width_current_sparse_pass",
    "05__probe_diagjson_line_directive_macro_add_rich_presence_strict":
        "05__line_directive_macro_add_diagjson_rich_location",
    "05__probe_diagjson_line_directive_shift_width_file_presence_reject":
        "05__diagjson__line_directive_shift_width_current_sparse_pass",

    # Bucket 06: assignment and lvalue diagnostic provenance.
    "06__probe_line_directive_assign_incompatible_spelling_reject":
        "06__probe_promotion__diag__line_directive_assign_incompatible_current_sparse_pass",
    "06__probe_line_directive_assign_qualifier_loss_spelling_reject":
        "06__probe_promotion__diag__line_directive_assign_qualifier_loss_current_sparse_pass",
    "06__probe_line_directive_include_assign_incompatible_spelling_reject":
        "06__probe_promotion__diag__line_directive_include_assign_incompatible_current_sparse_pass",
    "06__probe_line_directive_include_assign_qualifier_loss_spelling_reject":
        "06__probe_promotion__diag__line_directive_include_assign_qualifier_loss_current_sparse_pass",
    "06__probe_line_directive_nonmodifiable_lvalue_spelling_reject":
        "06__probe_promotion__diag__line_directive_nonmodifiable_lvalue_current_sparse_pass",
    "06__probe_line_directive_include_nonmodifiable_lvalue_spelling_reject":
        "06__probe_promotion__diag__line_directive_include_nonmodifiable_lvalue_current_sparse_pass",
    "06__probe_line_directive_bitfield_address_spelling_reject":
        "06__line_directive_bitfield_address_diag_current_sparse",
    "06__probe_line_directive_bitfield_address_current_sparse_pass":
        "06__line_directive_bitfield_address_diag_current_sparse",
    "06__probe_line_directive_include_bitfield_address_spelling_reject":
        "06__line_directive_include_bitfield_address_diag_current_sparse",
    "06__probe_line_directive_include_bitfield_address_current_sparse_pass":
        "06__line_directive_include_bitfield_address_diag_current_sparse",
    "06__probe_line_directive_temp_increment_spelling_reject":
        "06__line_directive_temp_increment_diag_current_sparse",
    "06__probe_line_directive_temp_increment_current_sparse_pass":
        "06__line_directive_temp_increment_diag_current_sparse",
    "06__probe_line_directive_include_temp_increment_spelling_reject":
        "06__line_directive_include_temp_increment_diag_current_sparse",
    "06__probe_line_directive_include_temp_increment_current_sparse_pass":
        "06__line_directive_include_temp_increment_diag_current_sparse",
    "06__probe_line_directive_compound_assign_pointer_plus_pointer_spelling_reject":
        "06__line_directive_compound_assign_pointer_plus_pointer_diag_current_sparse",
    "06__probe_line_directive_compound_assign_pointer_plus_pointer_current_sparse_pass":
        "06__line_directive_compound_assign_pointer_plus_pointer_diag_current_sparse",
    "06__probe_line_directive_include_compound_assign_pointer_plus_pointer_spelling_reject":
        "06__line_directive_include_compound_assign_pointer_plus_pointer_diag_current_sparse",
    "06__probe_line_directive_include_compound_assign_pointer_plus_pointer_current_sparse_pass":
        "06__line_directive_include_compound_assign_pointer_plus_pointer_diag_current_sparse",
    "06__probe_diagjson_line_directive_assign_incompatible_file_presence_reject":
        "06__probe_promotion__diagjson__line_directive_assign_incompatible_current_sparse_pass",
    "06__probe_diagjson_line_directive_assign_qualifier_loss_file_presence_reject":
        "06__probe_promotion__diagjson__line_directive_assign_qualifier_loss_current_sparse_pass",
    "06__probe_diagjson_line_directive_include_assign_incompatible_file_presence_reject":
        "06__probe_promotion__diagjson__line_directive_include_assign_incompatible_current_sparse_pass",
    "06__probe_diagjson_line_directive_include_assign_qualifier_loss_file_presence_reject":
        "06__probe_promotion__diagjson__line_directive_include_assign_qualifier_loss_current_sparse_pass",
    "06__probe_diagjson_line_directive_nonmodifiable_lvalue_file_presence_reject":
        "06__probe_promotion__diagjson__line_directive_nonmodifiable_lvalue_current_sparse_pass",
    "06__probe_diagjson_line_directive_include_nonmodifiable_lvalue_file_presence_reject":
        "06__probe_promotion__diagjson__line_directive_include_nonmodifiable_lvalue_current_sparse_pass",
    "06__probe_diagjson_line_directive_bitfield_address_file_presence_reject":
        "06__line_directive_bitfield_address_diagjson_current_sparse",
    "06__probe_diagjson_line_directive_bitfield_address_current_sparse_pass":
        "06__line_directive_bitfield_address_diagjson_current_sparse",
    "06__probe_diagjson_line_directive_include_bitfield_address_file_presence_reject":
        "06__line_directive_include_bitfield_address_diagjson_current_sparse",
    "06__probe_diagjson_line_directive_include_bitfield_address_current_sparse_pass":
        "06__line_directive_include_bitfield_address_diagjson_current_sparse",
    "06__probe_diagjson_line_directive_temp_increment_file_presence_reject":
        "06__line_directive_temp_increment_diagjson_current_sparse",
    "06__probe_diagjson_line_directive_temp_increment_current_sparse_pass":
        "06__line_directive_temp_increment_diagjson_current_sparse",
    "06__probe_diagjson_line_directive_include_temp_increment_file_presence_reject":
        "06__line_directive_include_temp_increment_diagjson_current_sparse",
    "06__probe_diagjson_line_directive_include_temp_increment_current_sparse_pass":
        "06__line_directive_include_temp_increment_diagjson_current_sparse",
    "06__probe_diagjson_line_directive_compound_assign_pointer_plus_pointer_file_presence_reject":
        "06__line_directive_compound_assign_pointer_plus_pointer_diagjson_current_sparse",
    "06__probe_diagjson_line_directive_compound_assign_pointer_plus_pointer_current_sparse_pass":
        "06__line_directive_compound_assign_pointer_plus_pointer_diagjson_current_sparse",
    "06__probe_diagjson_line_directive_include_compound_assign_pointer_plus_pointer_file_presence_reject":
        "06__line_directive_include_compound_assign_pointer_plus_pointer_diagjson_current_sparse",
    "06__probe_diagjson_line_directive_include_compound_assign_pointer_plus_pointer_current_sparse_pass":
        "06__line_directive_include_compound_assign_pointer_plus_pointer_diagjson_current_sparse",
}


# Bucket 08: paired reduced/strict diagnostics already share stable expectations
# that assert both the semantic payload and the remapped spelling/file fields.
_BUCKET_08_DIAG_BASES = (
    "line_directive_designator_array_index_negative",
    "line_directive_designator_array_index_nonconst",
    "line_directive_flex_not_last",
    "line_directive_include_designator_array_index_negative",
    "line_directive_include_designator_array_index_nonconst",
    "line_directive_include_flex_not_last",
    "line_directive_include_union_flex_member",
    "line_directive_union_flex_member",
)
for _base in _BUCKET_08_DIAG_BASES:
    _owner = f"08__{_base}_diag_current_sparse"
    PROMOTION_CLOSURE_OWNERS[f"08__probe_diag_{_base}_current_sparse_pass"] = _owner
    PROMOTION_CLOSURE_OWNERS[f"08__probe_diag_{_base}_spelling_reject"] = _owner

_BUCKET_08_DIAGJSON_BASES = (
    "line_directive_designator_array_index_negative",
    "line_directive_designator_array_index_nonconst",
    "line_directive_flex_not_last",
    "line_directive_include_designator_array_index_negative",
    "line_directive_include_designator_array_index_nonconst",
    "line_directive_include_flex_not_last",
    "line_directive_include_nested_designator_array_index_negative",
    "line_directive_include_nested_designator_array_index_nonconst",
    "line_directive_include_union_flex_member",
    "line_directive_nested_designator_array_index_negative",
    "line_directive_nested_designator_array_index_nonconst",
    "line_directive_union_flex_member",
)
for _base in _BUCKET_08_DIAGJSON_BASES:
    _owner = f"08__{_base}_diagjson_current_sparse"
    PROMOTION_CLOSURE_OWNERS[f"08__probe_diagjson_{_base}_current_sparse_pass"] = _owner
    PROMOTION_CLOSURE_OWNERS[f"08__probe_diagjson_{_base}_file_presence_reject"] = _owner

PROMOTION_CLOSURE_OWNERS.update({
    "08__probe_runtime_file_scope_union_string_bitfield_feedback_mesh":
        "08__runtime_file_scope_union_string_bitfield_feedback_mesh",
    "08__probe_runtime_wave76_union_row_designator_overrides_current":
        "08__runtime_wave76_union_row_designator_overrides",
})


# Bucket 09: switch-control text diagnostics and loop-control JSON records are
# paired views of stable remapped-location tests.
_BUCKET_09_DIAG_BASES = (
    "line_directive_include_switch_double_condition",
    "line_directive_include_switch_pointer_condition",
    "line_directive_include_switch_string_condition",
    "line_directive_switch_double_condition",
    "line_directive_switch_pointer_condition",
    "line_directive_switch_string_condition",
)
for _base in _BUCKET_09_DIAG_BASES:
    _owner = f"09__{_base}_diag_current_sparse"
    PROMOTION_CLOSURE_OWNERS[f"09__probe_diag_{_base}_current_sparse_pass"] = _owner
    PROMOTION_CLOSURE_OWNERS[f"09__probe_diag_{_base}_spelling_reject"] = _owner

_BUCKET_09_DIAGJSON_BASES = (
    "line_directive_break_outside_loop",
    "line_directive_continue_outside_loop",
    "line_directive_include_break_outside_loop",
    "line_directive_include_continue_outside_loop",
)
for _base in _BUCKET_09_DIAGJSON_BASES:
    _owner = f"09__{_base}_diagjson_current_sparse"
    PROMOTION_CLOSURE_OWNERS[f"09__probe_diagjson_{_base}_current_sparse_pass"] = _owner
    PROMOTION_CLOSURE_OWNERS[f"09__probe_diagjson_{_base}_file_presence_reject"] = _owner

PROMOTION_CLOSURE_OWNERS[
    "09__probe_runtime_switch_default_fallthrough_continue_break_mesh_xiv_current_sparse_pass"
] = "09__runtime__switch_default_fallthrough_continue_break_mesh_xiv"


# Bucket 10: the stable linkage manifests already own these strict remapped
# text/JSON oracles; several legacy ids retain historical "current" labels.
PROMOTION_CLOSURE_OWNERS.update({
    "10__probe_diag_line_directive_extern_array_def_mismatch_spelling_strict":
        "10__line_directive_extern_array_def_mismatch_diag_text_current_nodiag",
    "10__probe_diag_line_directive_extern_array_mismatch_first_decl_line_spelling_strict":
        "10__line_directive_extern_array_mismatch_first_decl_line_diag_text_current_nodiag",
    "10__probe_diag_line_directive_extern_array_mismatch_second_decl_line_spelling_strict":
        "10__line_directive_extern_array_mismatch_second_decl_line_diag_text_current_nodiag",
    "10__probe_diag_line_directive_extern_array_mismatch_spelling_strict":
        "10__line_directive_extern_array_mismatch_diag_text_current_nodiag",
    "10__probe_diag_line_directive_extern_array_vs_scalar_conflict_spelling_strict":
        "10__line_directive_extern_array_vs_scalar_conflict_diag_text_strict",
    "10__probe_diag_line_directive_include_extern_array_def_mismatch_spelling_strict":
        "10__line_directive_include_extern_array_def_mismatch_diag_text_current_nodiag",
    "10__probe_diag_line_directive_include_extern_array_mismatch_spelling_strict":
        "10__line_directive_include_extern_array_mismatch_diag_text_current_nodiag",
    "10__probe_diag_line_directive_multitu_extern_array_def_mismatch_spelling_strict":
        "10__line_directive_multitu_extern_array_def_mismatch_diag_text_current_nodiag",
    "10__probe_diag_line_directive_multitu_include_extern_array_def_mismatch_spelling_strict":
        "10__line_directive_multitu_include_extern_array_def_mismatch_diag_text_current_nodiag",
    "10__probe_diagjson_line_directive_block_extern_different_type_rich_strict":
        "10__line_directive_block_extern_different_type_diagjson_strict",
    "10__probe_diagjson_line_directive_extern_array_def_mismatch_file_presence_reject":
        "10__line_directive_extern_array_def_mismatch_diagjson_strict",
    "10__probe_diagjson_line_directive_extern_array_mismatch_file_presence_reject":
        "10__line_directive_extern_array_mismatch_diagjson_strict",
    "10__probe_diagjson_line_directive_extern_array_mismatch_first_decl_line_file_presence_reject":
        "10__line_directive_extern_array_mismatch_first_decl_line_diagjson_strict",
    "10__probe_diagjson_line_directive_extern_array_mismatch_second_decl_line_file_presence_reject":
        "10__line_directive_extern_array_mismatch_second_decl_line_diagjson_strict",
    "10__probe_diagjson_line_directive_extern_static_mismatch_rich_strict":
        "10__line_directive_extern_static_mismatch_diagjson_strict",
    "10__probe_diagjson_line_directive_extern_type_mismatch_rich_strict":
        "10__line_directive_extern_type_mismatch_diagjson_strict",
    "10__probe_diagjson_line_directive_function_redecl_conflict_rich_strict":
        "10__line_directive_function_redecl_conflict_diagjson_strict",
    "10__probe_diagjson_line_directive_include_block_extern_different_type_rich_strict":
        "10__line_directive_include_block_extern_different_type_diagjson_strict",
    "10__probe_diagjson_line_directive_include_extern_array_def_mismatch_file_presence_reject":
        "10__line_directive_include_extern_array_def_mismatch_diagjson_strict",
    "10__probe_diagjson_line_directive_include_extern_array_mismatch_file_presence_reject":
        "10__line_directive_include_extern_array_mismatch_diagjson_strict",
    "10__probe_diagjson_line_directive_include_extern_static_mismatch_rich_strict":
        "10__line_directive_include_extern_static_mismatch_diagjson_strict",
    "10__probe_diagjson_line_directive_include_extern_type_mismatch_rich_strict":
        "10__line_directive_include_extern_type_mismatch_diagjson_strict",
    "10__probe_diagjson_line_directive_include_function_redecl_conflict_rich_strict":
        "10__line_directive_include_function_redecl_conflict_diagjson_strict",
    "10__probe_diagjson_line_directive_include_tentative_static_conflict_rich_strict":
        "10__line_directive_include_tentative_static_conflict_diagjson_strict",
    "10__probe_diagjson_line_directive_tentative_static_conflict_rich_strict":
        "10__line_directive_tentative_static_conflict_diagjson_strict",
})


# Bucket 11: paired sparse/strict JSON records use stable expectations that
# already include remapped file presence. Four multi-TU text guards have new
# same-family owners in the wave93 closure manifest.
_BUCKET_11_JSON_BASES = (
    ("argument_type_mismatch", "argument_type_mismatch"),
    ("nonvoid_missing_return", "nonvoid_missing_return"),
    ("return_type_mismatch", "return_type_mismatch"),
    ("too_few_args", "prototype_too_few_args"),
    ("too_many_args", "prototype_too_many_args"),
    ("void_return_value", "void_return_value"),
)
for _probe_base, _owner_base in _BUCKET_11_JSON_BASES:
    for _include in ("", "include_"):
        _probe = f"line_directive_{_include}{_probe_base}"
        _owner = f"11__line_directive_{_include}{_owner_base}_diagjson_current_sparse"
        PROMOTION_CLOSURE_OWNERS[f"11__probe_diagjson_{_probe}_current_sparse_pass"] = _owner
        PROMOTION_CLOSURE_OWNERS[f"11__probe_diagjson_{_probe}_file_presence_reject"] = _owner

PROMOTION_CLOSURE_OWNERS.update({
    "11__probe_diagjson_line_directive_fnptr_assign_file_presence_reject":
        "11__line_directive_function_pointer_assign_diagjson_strict",
    "11__probe_diagjson_line_directive_include_fnptr_assign_file_presence_reject":
        "11__line_directive_include_function_pointer_assign_diagjson_strict",
    "11__probe_diagjson_line_directive_multitu_fnptr_assign_file_presence_reject":
        "11__line_directive_multitu_function_pointer_assign_diagjson_strict",
    "11__probe_diagjson_line_directive_multitu_include_fnptr_assign_file_presence_reject":
        "11__line_directive_multitu_include_function_pointer_assign_diagjson_strict",
    "11__probe_diagjson_line_directive_typedef_fnptr_argument_type_mismatch_current_empty_pass":
        "11__line_directive_typedef_fnptr_argument_type_mismatch_diagjson_strict",
    "11__probe_diagjson_line_directive_typedef_fnptr_argument_type_mismatch_file_presence_reject":
        "11__line_directive_typedef_fnptr_argument_type_mismatch_diagjson_strict",
    "11__probe_diagjson_line_directive_include_typedef_fnptr_argument_type_mismatch_current_empty_pass":
        "11__line_directive_include_typedef_fnptr_argument_type_mismatch_diagjson_strict",
    "11__probe_diagjson_line_directive_include_typedef_fnptr_argument_type_mismatch_file_presence_reject":
        "11__line_directive_include_typedef_fnptr_argument_type_mismatch_diagjson_strict",
    "11__probe_diagjson_line_directive_multitu_too_many_args_file_presence_reject":
        "11__line_directive_multitu_prototype_too_many_args_diagjson_strict",
    "11__probe_diagjson_line_directive_multitu_include_too_many_args_file_presence_reject":
        "11__line_directive_multitu_include_prototype_too_many_args_diagjson_strict",
    "11__probe_diagjson_line_directive_multitu_parserdiag_decl_missing_rparen_presence_reject":
        "11__line_directive_multitu_parserdiag_declarator_missing_rparen_diagjson_strict",
    "11__probe_diagjson_line_directive_multitu_include_parserdiag_decl_missing_rparen_presence_reject":
        "11__line_directive_multitu_include_parserdiag_declarator_missing_rparen_diagjson_strict",
    "11__probe_diagjson_line_directive_multitu_return_type_mismatch_file_presence_reject":
        "11__line_directive_multitu_return_type_mismatch_diagjson_strict",
    "11__probe_diagjson_line_directive_multitu_include_return_type_mismatch_file_presence_reject":
        "11__line_directive_multitu_include_return_type_mismatch_diagjson_strict",
    "11__probe_diagjson_line_directive_multitu_typedef_fnptr_argument_type_mismatch_current_threshold_pass":
        "11__line_directive_multitu_typedef_fnptr_argument_type_mismatch_diagjson_current_threshold",
    "11__probe_diagjson_line_directive_multitu_include_typedef_fnptr_argument_type_mismatch_current_threshold_pass":
        "11__line_directive_multitu_include_typedef_fnptr_argument_type_mismatch_diagjson_current_threshold",
    "11__probe_diagjson_line_directive_multitu_typedef_fnptr_argument_type_mismatch_file_presence_reject":
        "11__line_directive_multitu_typedef_fnptr_argument_type_mismatch_diagjson_strict",
    "11__probe_diagjson_line_directive_multitu_include_typedef_fnptr_argument_type_mismatch_file_presence_reject":
        "11__line_directive_multitu_include_typedef_fnptr_argument_type_mismatch_diagjson_strict",
    "11__probe_diag_line_directive_multitu_too_many_args_text_threshold_pass":
        "11__diag__line_directive_multitu_too_many_args_text_threshold",
    "11__probe_diag_line_directive_multitu_include_too_many_args_text_threshold_pass":
        "11__diag__line_directive_multitu_include_too_many_args_text_threshold",
    "11__probe_diag_line_directive_multitu_parserdiag_decl_missing_rparen_text_threshold_pass":
        "11__diag__line_directive_multitu_parserdiag_decl_missing_rparen_text_threshold",
    "11__probe_diag_line_directive_multitu_include_parserdiag_decl_missing_rparen_text_threshold_pass":
        "11__diag__line_directive_multitu_include_parserdiag_decl_missing_rparen_text_threshold",
})

del _base, _owner, _probe, _probe_base, _owner_base, _include


# These records are deliberately not stable-final tests. They remain in the
# probe runner because their value is stress/corpus robustness rather than a
# product behavior contract with a durable exact oracle.
PROMOTION_CLOSURE_RETAINED = {
    "14__probe_axis3_wave19_multitu_abi_hfa_struct_return_variadic_shadow_replay_fold_edge_reduced":
        "axis3 reduced stress companion; strict behavior already has stable ownership",
    "15__probe_diag_corpus_external_compile_reject":
        "external malformed corpus robustness lane",
    "15__probe_diag_corpus_external_include_guard_mismatch_reject":
        "external malformed corpus robustness lane",
    "15__probe_diag_corpus_external_macro_chain_reject":
        "external malformed corpus robustness lane",
    "15__probe_diag_corpus_external_macro_guard_reject":
        "external malformed corpus robustness lane",
    "15__probe_diag_corpus_pinned_macro_include_chain_reject":
        "pinned malformed corpus robustness lane",
    "15__probe_diag_corpus_pinned_typedef_decl_cycle_reject":
        "pinned malformed corpus robustness lane",
    "15__probe_diag_fuzz_seeded_malformed_volume_replay_no_crash":
        "seeded fuzz replay no-crash lane",
    "15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash":
        "seeded malformed preprocessor no-crash lane",
    "15__probe_diag_malformed_token_stream_seeded_a_no_crash":
        "seeded malformed token-stream no-crash lane",
    "15__probe_diag_malformed_token_stream_seeded_b_no_crash":
        "seeded malformed token-stream no-crash lane",
    "15__probe_diag_malformed_token_stream_seeded_c_no_crash":
        "seeded malformed token-stream no-crash lane",
    "15__probe_diag_pathological_initializer_rewrite_surface_reject":
        "pathological diagnostic stress lane",
    "15__probe_diag_pathological_initializer_shape_reject":
        "pathological diagnostic stress lane",
    "15__probe_diag_pathological_switch_case_surface_reject":
        "pathological diagnostic stress lane",
}
