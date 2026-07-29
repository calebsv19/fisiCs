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


# Bucket 15 wave 139: deterministic OS-P3 and post-EDU-19 runtime matrices now
# execute in the stable final suite. Object-policy inspection remains owned by
# the probe runner because the stable runner does not duplicate ELF contracts.
_BUCKET_15_WAVE139_RUNTIME_PROMOTIONS = (
    "15__probe_os_post_edu19_durable_owner_chain_matrix",
    "15__probe_os_post_edu19_edu21_control_v1_matrix",
    "15__probe_os_post_edu19_edu22_queue_v2_matrix",
    "15__probe_os_post_edu19_edu24_31_wire_history_matrix",
    "15__probe_os_post_edu19_edu24_artifact_exchange_matrix",
    "15__probe_os_post_edu19_edu25_loader_geometry_matrix",
    "15__probe_os_post_edu19_edu27_phase_execution_matrix",
    "15__probe_os_post_edu19_edu28_artifact_meta_matrix",
    "15__probe_os_post_edu19_edu29_30_async_stop_matrix",
    "15__probe_os_post_edu19_edu31_time_arithmetic_matrix",
    "15__probe_os_post_edu19_edu37_two_owner_store_matrix",
    "15__probe_os_post_edu19_edu38_control_v13_matrix",
    "15__probe_os_post_edu19_edu38_runner_context_matrix",
    "15__probe_os_post_edu19_edu39_phase_owner_matrix",
    "15__probe_os_post_edu19_edu40_mailbox_owner_matrix",
    "15__probe_os_post_edu19_edu41_two_active_runner_matrix",
    "15__probe_os_post_edu19_two_owner_fault_composition_matrix",
    "15__probe_osp3_admission_precedence_matrix",
    "15__probe_osp3_aggregate_checkpoint_matrix",
    "15__probe_osp3_callback_dispatch_matrix",
    "15__probe_osp3_expand_admission_one",
    "15__probe_osp3_expand_admission_prime",
    "15__probe_osp3_expand_admission_stress_sanitized",
    "15__probe_osp3_expand_admission_zero",
    "15__probe_osp3_expand_aggregate_one",
    "15__probe_osp3_expand_aggregate_prime",
    "15__probe_osp3_expand_aggregate_stress_sanitized",
    "15__probe_osp3_expand_aggregate_zero",
    "15__probe_osp3_expand_callback_one",
    "15__probe_osp3_expand_callback_prime",
    "15__probe_osp3_expand_callback_stress_sanitized",
    "15__probe_osp3_expand_callback_zero",
    "15__probe_osp3_expand_extent_one",
    "15__probe_osp3_expand_extent_prime",
    "15__probe_osp3_expand_extent_stress_sanitized",
    "15__probe_osp3_expand_extent_zero",
    "15__probe_osp3_expand_queue_one",
    "15__probe_osp3_expand_queue_prime",
    "15__probe_osp3_expand_queue_stress_sanitized",
    "15__probe_osp3_expand_queue_zero",
    "15__probe_osp3_expand_scalar_double_one",
    "15__probe_osp3_expand_scalar_double_prime",
    "15__probe_osp3_expand_scalar_double_stress_sanitized",
    "15__probe_osp3_expand_scalar_double_zero",
    "15__probe_osp3_expand_scheduler_one",
    "15__probe_osp3_expand_scheduler_prime",
    "15__probe_osp3_expand_scheduler_stress_sanitized",
    "15__probe_osp3_expand_scheduler_zero",
    "15__probe_osp3_expand_sync_rank_one",
    "15__probe_osp3_expand_sync_rank_prime",
    "15__probe_osp3_expand_sync_rank_stress_sanitized",
    "15__probe_osp3_expand_sync_rank_zero",
    "15__probe_osp3_expand_token_one",
    "15__probe_osp3_expand_token_prime",
    "15__probe_osp3_expand_token_stress_sanitized",
    "15__probe_osp3_expand_token_zero",
    "15__probe_osp3_expand_variadic_sret_one",
    "15__probe_osp3_expand_variadic_sret_prime",
    "15__probe_osp3_expand_variadic_sret_stress_sanitized",
    "15__probe_osp3_expand_variadic_sret_zero",
    "15__probe_osp3_extent_overflow_matrix",
    "15__probe_osp3_generation_token_matrix",
    "15__probe_osp3_long_double_abi_model",
    "15__probe_osp3_queue_transition_matrix",
    "15__probe_osp3_raw_elf_mutation_alt",
    "15__probe_osp3_raw_elf_mutation_stress_sanitized",
    "15__probe_osp3_raw_job_authority_replay",
    "15__probe_osp3_raw_job_late_reject_reset",
    "15__probe_osp3_raw_job_mutation_alt",
    "15__probe_osp3_raw_job_mutation_base",
    "15__probe_osp3_raw_job_mutation_stress_sanitized",
    "15__probe_osp3_raw_job_rejection_precedence",
    "15__probe_osp3_raw_job_step_ranges_overlap",
    "15__probe_osp3_raw_job_truncation_geometry",
    "15__probe_osp3_raw_job_valid_packages",
    "15__probe_osp3_raw_storage_extent_overlap",
    "15__probe_osp3_raw_storage_late_reject_reset",
    "15__probe_osp3_raw_storage_mutation_alt",
    "15__probe_osp3_raw_storage_mutation_base",
    "15__probe_osp3_raw_storage_mutation_stress_sanitized",
    "15__probe_osp3_raw_storage_rejection_precedence",
    "15__probe_osp3_raw_storage_sequence_replay",
    "15__probe_osp3_raw_storage_truncation_geometry",
    "15__probe_osp3_raw_storage_valid_images",
    "15__probe_osp3_scalar_double_dyadic_matrix",
    "15__probe_osp3_scheduler_selection_matrix",
    "15__probe_osp3_sync_rank_matrix",
    "15__probe_osp3_variadic_struct_return_matrix",
)
for _probe_id in _BUCKET_15_WAVE139_RUNTIME_PROMOTIONS:
    PROMOTION_CLOSURE_OWNERS[_probe_id] = (
        "15__runtime_promotion__" + _probe_id.removeprefix("15__probe_")
    )


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
