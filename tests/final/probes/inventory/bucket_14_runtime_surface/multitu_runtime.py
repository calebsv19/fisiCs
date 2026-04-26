from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='14__probe_multitu_static_init_visibility_bridge',
        source=PROBE_DIR / 'runtime/14__probe_multitu_static_init_visibility_bridge_main.c',
        note='multi-TU static local + file-static visibility bridge should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/14__probe_multitu_static_init_visibility_bridge_main.c', PROBE_DIR / 'runtime/14__probe_multitu_static_init_visibility_bridge_lib.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_param_nested_handoff_matrix',
        source=PROBE_DIR / 'runtime/14__probe_vla_param_nested_handoff_matrix.c',
        note='VLA nested parameter handoff matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_stride_rebind_cross_fn_chain',
        source=PROBE_DIR / 'runtime/14__probe_vla_stride_rebind_cross_fn_chain.c',
        note='VLA stride rebind across helper-function chain should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_ptrdiff_cross_scope_matrix',
        source=PROBE_DIR / 'runtime/14__probe_vla_ptrdiff_cross_scope_matrix.c',
        note='VLA ptrdiff cross-scope matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_intptr_roundtrip_offset_matrix',
        source=PROBE_DIR / 'runtime/14__probe_intptr_roundtrip_offset_matrix.c',
        note='intptr_t pointer roundtrip offset matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_uintptr_mask_rebase_matrix',
        source=PROBE_DIR / 'runtime/14__probe_uintptr_mask_rebase_matrix.c',
        note='uintptr_t mask/rebase reconstruction matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_ptrdiff_boundary_crosscheck_matrix',
        source=PROBE_DIR / 'runtime/14__probe_ptrdiff_boundary_crosscheck_matrix.c',
        note='ptrdiff_t boundary crosscheck matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_variadic_llong_double_bridge',
        source=PROBE_DIR / 'runtime/14__probe_variadic_llong_double_bridge.c',
        note='variadic long-long/double width bridge should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_control_do_while_switch_phi_chain',
        source=PROBE_DIR / 'runtime/14__probe_control_do_while_switch_phi_chain.c',
        note='do-while + switch + continue/goto phi paths should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_struct_array_double_by_value_roundtrip',
        source=PROBE_DIR / 'runtime/14__probe_struct_array_double_by_value_roundtrip.c',
        note='struct with array+double by-value roundtrip should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_dim_recompute_stride_matrix',
        source=PROBE_DIR / 'runtime/14__probe_vla_dim_recompute_stride_matrix.c',
        note='VLA dimension recompute and stride matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_pointer_byte_rebase_roundtrip_matrix',
        source=PROBE_DIR / 'runtime/14__probe_pointer_byte_rebase_roundtrip_matrix.c',
        note='pointer byte-rebase roundtrip and element ptrdiff scaling should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_float_nan_inf_order_chain',
        source=PROBE_DIR / 'runtime/14__probe_float_nan_inf_order_chain.c',
        note='NaN/infinity ordering and comparison chain should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_null_ptrdiff_bridge',
        source=PROBE_DIR / 'runtime/14__probe_header_null_ptrdiff_bridge.c',
        note='header NULL + ptrdiff bridge semantics should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_struct_union_array_overlay_roundtrip',
        source=PROBE_DIR / 'runtime/14__probe_struct_union_array_overlay_roundtrip.c',
        note='struct+union+array by-value roundtrip overlay should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_variadic_long_width_crosscheck',
        source=PROBE_DIR / 'runtime/14__probe_variadic_long_width_crosscheck.c',
        note='variadic long/unsigned-long/long-long width crosscheck should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_struct_ternary_by_value_select_chain',
        source=PROBE_DIR / 'runtime/14__probe_struct_ternary_by_value_select_chain.c',
        note='struct ternary by-value selection chain should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_offsetof_nested_matrix',
        source=PROBE_DIR / 'runtime/14__probe_header_offsetof_nested_matrix.c',
        note='nested offsetof header surface should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_switch_sparse_signed_case_ladder',
        source=PROBE_DIR / 'runtime/14__probe_switch_sparse_signed_case_ladder.c',
        note='sparse signed switch case ladder should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_bitwise_compound_recurrence_matrix',
        source=PROBE_DIR / 'runtime/14__probe_bitwise_compound_recurrence_matrix.c',
        note='bitwise compound recurrence matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_bool_scalar_conversion_matrix',
        source=PROBE_DIR / 'runtime/14__probe_bool_scalar_conversion_matrix.c',
        note='scalar-to-bool conversion matrix across int/float/pointer lanes should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_bitfield_signed_promotion_matrix',
        source=PROBE_DIR / 'runtime/14__probe_bitfield_signed_promotion_matrix.c',
        note='signed bitfield promotion/update matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_bitfield_compound_assign_bridge',
        source=PROBE_DIR / 'runtime/14__probe_bitfield_compound_assign_bridge.c',
        note='unsigned bitfield compound update bridge should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_bitfield_bool_bridge_matrix',
        source=PROBE_DIR / 'runtime/14__probe_bitfield_bool_bridge_matrix.c',
        note='bitfield and bool bridge paths should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_multitu_fnptr_table_state_bridge',
        source=PROBE_DIR / 'runtime/14__probe_multitu_fnptr_table_state_bridge_main.c',
        note='multi-TU function-pointer table state bridge should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/14__probe_multitu_fnptr_table_state_bridge_main.c', PROBE_DIR / 'runtime/14__probe_multitu_fnptr_table_state_bridge_lib.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_multitu_variadic_fold_bridge',
        source=PROBE_DIR / 'runtime/14__probe_multitu_variadic_fold_bridge_main.c',
        note='multi-TU mixed-width fold bridge should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/14__probe_multitu_variadic_fold_bridge_main.c', PROBE_DIR / 'runtime/14__probe_multitu_variadic_fold_bridge_lib.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_multitu_vla_window_reduce_bridge',
        source=PROBE_DIR / 'runtime/14__probe_multitu_vla_window_reduce_bridge_main.c',
        note='multi-TU VLA window reduction bridge should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/14__probe_multitu_vla_window_reduce_bridge_main.c', PROBE_DIR / 'runtime/14__probe_multitu_vla_window_reduce_bridge_lib.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_multitu_struct_union_route_bridge',
        source=PROBE_DIR / 'runtime/14__probe_multitu_struct_union_route_bridge_main.c',
        note='multi-TU struct/union route bridge should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/14__probe_multitu_struct_union_route_bridge_main.c', PROBE_DIR / 'runtime/14__probe_multitu_struct_union_route_bridge_lib.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_control_switch_do_for_state_machine',
        source=PROBE_DIR / 'runtime/14__probe_control_switch_do_for_state_machine.c',
        note='control-flow state machine across for/do/switch should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_layout_offset_stride_bridge',
        source=PROBE_DIR / 'runtime/14__probe_layout_offset_stride_bridge.c',
        note='layout offsetof/stride bridge should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_pointer_stride_rebase_control_mix',
        source=PROBE_DIR / 'runtime/14__probe_pointer_stride_rebase_control_mix.c',
        note='pointer-stride rebasing with mixed control flow should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_bool_bitmask_control_chain',
        source=PROBE_DIR / 'runtime/14__probe_bool_bitmask_control_chain.c',
        note='bool-bitmask control chain should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_smoke_expr_eval_driver',
        source=PROBE_DIR / 'runtime/14__probe_smoke_expr_eval_driver.c',
        note='deterministic expression-evaluator mini-driver should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_smoke_dispatch_table_driver',
        source=PROBE_DIR / 'runtime/14__probe_smoke_dispatch_table_driver.c',
        note='deterministic function-pointer dispatch mini-driver should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_smoke_multitu_config_driver',
        source=PROBE_DIR / 'runtime/14__probe_smoke_multitu_config_driver_main.c',
        note='deterministic multi-TU config mini-driver should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/14__probe_smoke_multitu_config_driver_main.c', PROBE_DIR / 'runtime/14__probe_smoke_multitu_config_driver_lib.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_repeatable_output_hash_matrix',
        source=PROBE_DIR / 'runtime/14__probe_repeatable_output_hash_matrix.c',
        note='repeated same-input executions should produce stable runtime output hash matrix',
    ),
    RuntimeProbe(
        probe_id='14__probe_repeatable_diagjson_hash_matrix',
        source=PROBE_DIR / 'runtime/14__probe_repeatable_diagjson_hash_matrix.c',
        note='diagnostic-record modeling hash matrix should be stable across repeated builds',
    ),
    RuntimeProbe(
        probe_id='14__probe_repeatable_multitu_link_hash_matrix',
        source=PROBE_DIR / 'runtime/14__probe_repeatable_multitu_link_hash_matrix_main.c',
        note='multi-TU link-path hash matrix should remain stable across repeated calls',
        inputs=[PROBE_DIR / 'runtime/14__probe_repeatable_multitu_link_hash_matrix_main.c', PROBE_DIR / 'runtime/14__probe_repeatable_multitu_link_hash_matrix_lib.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_multitu_link_order_static_init_crc_matrix',
        source=PROBE_DIR / 'runtime/14__probe_multitu_link_order_static_init_crc_matrix_main.c',
        note='multi-TU static-init and call-order CRC matrix should remain deterministic and match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/14__probe_multitu_link_order_static_init_crc_matrix_main.c', PROBE_DIR / 'runtime/14__probe_multitu_link_order_static_init_crc_matrix_lib_a.c', PROBE_DIR / 'runtime/14__probe_multitu_link_order_static_init_crc_matrix_lib_b.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_repeatable_multitu_symbol_permutation_hash',
        source=PROBE_DIR / 'runtime/14__probe_repeatable_multitu_symbol_permutation_hash_main.c',
        note='multi-TU symbol permutation dispatch hash should remain deterministic and match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/14__probe_repeatable_multitu_symbol_permutation_hash_main.c', PROBE_DIR / 'runtime/14__probe_repeatable_multitu_symbol_permutation_hash_lib.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_repeatable_runtime_stdout_hash_matrix',
        source=PROBE_DIR / 'runtime/14__probe_repeatable_runtime_stdout_hash_matrix.c',
        note='text-output hash matrix should remain deterministic across repeated runs and match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_abi_long_double_variadic_regstack_hardening',
        source=PROBE_DIR / 'runtime/14__probe_abi_long_double_variadic_regstack_hardening.c',
        note='long-double variadic ABI mix should be repeatable and match clang at reg/stack boundaries',
    ),
    RuntimeProbe(
        probe_id='14__probe_multitu_static_init_order_depth_bridge',
        source=PROBE_DIR / 'runtime/14__probe_multitu_static_init_order_depth_bridge_main.c',
        note='multi-TU static init/order depth bridge should remain deterministic and match clang',
        inputs=[PROBE_DIR / 'runtime/14__probe_multitu_static_init_order_depth_bridge_main.c', PROBE_DIR / 'runtime/14__probe_multitu_static_init_order_depth_bridge_lib.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_queue_push_fail_preserves_committed_slot',
        source=PROBE_DIR / 'runtime/14__probe_queue_push_fail_preserves_committed_slot.c',
        note='queue push-fail path should preserve committed slot state and match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_multitu_token_generation_stale_drop',
        source=PROBE_DIR / 'runtime/14__probe_multitu_token_generation_stale_drop_main.c',
        note='multi-TU token generation stale-drop path should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/14__probe_multitu_token_generation_stale_drop_main.c', PROBE_DIR / 'runtime/14__probe_multitu_token_generation_stale_drop_lib.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_multitu_owner_transfer_release_once',
        source=PROBE_DIR / 'runtime/14__probe_multitu_owner_transfer_release_once_main.c',
        note='multi-TU owner-transfer release-once path should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/14__probe_multitu_owner_transfer_release_once_main.c', PROBE_DIR / 'runtime/14__probe_multitu_owner_transfer_release_once_lib.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_queue_pop_fail_preserves_window_state',
        source=PROBE_DIR / 'runtime/14__probe_queue_pop_fail_preserves_window_state.c',
        note='queue pop-fail path should preserve read window state and match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_multitu_token_reserve_commit_abort',
        source=PROBE_DIR / 'runtime/14__probe_multitu_token_reserve_commit_abort_main.c',
        note='multi-TU token reserve/commit/abort transitions should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/14__probe_multitu_token_reserve_commit_abort_main.c', PROBE_DIR / 'runtime/14__probe_multitu_token_reserve_commit_abort_lib.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_multitu_layer_budget_trim_generation',
        source=PROBE_DIR / 'runtime/14__probe_multitu_layer_budget_trim_generation_main.c',
        note='multi-TU layer-budget trim and generation transitions should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/14__probe_multitu_layer_budget_trim_generation_main.c', PROBE_DIR / 'runtime/14__probe_multitu_layer_budget_trim_generation_lib.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_realproj_map_forge_layer_toggle_residency_trim_runtime',
        source=PROBE_DIR / 'runtime/14__probe_realproj_map_forge_layer_toggle_residency_trim_runtime.c',
        note='reduced map_forge layer-toggle and residency trim behavior should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_realproj_map_forge_tile_queue_generation_guard_runtime',
        source=PROBE_DIR / 'runtime/14__probe_realproj_map_forge_tile_queue_generation_guard_runtime.c',
        note='reduced map_forge tile-queue generation guard behavior should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_realproj_map_forge_owner_transfer_single_release_runtime',
        source=PROBE_DIR / 'runtime/14__probe_realproj_map_forge_owner_transfer_single_release_runtime.c',
        note='reduced map_forge owner-transfer single-release behavior should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_cleanup_double_call_guard_matrix',
        source=PROBE_DIR / 'runtime/14__probe_cleanup_double_call_guard_matrix.c',
        note='cleanup double-call guard matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_queue_trim_idempotent_window_matrix',
        source=PROBE_DIR / 'runtime/14__probe_queue_trim_idempotent_window_matrix.c',
        note='queue trim idempotent-window matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_generation_gate_stale_entry_drop_matrix',
        source=PROBE_DIR / 'runtime/14__probe_generation_gate_stale_entry_drop_matrix.c',
        note='generation-gate stale-entry drop matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_struct_return_long_double_bridge',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_struct_return_long_double_bridge.c',
        note='function-pointer selected long-double struct return bridge should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_variadic_fnptr_struct_payload_bridge',
        source=PROBE_DIR / 'runtime/14__probe_variadic_fnptr_struct_payload_bridge.c',
        note='variadic function-pointer struct payload fold bridge should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_multitu_fnptr_variadic_state_pipeline',
        source=PROBE_DIR / 'runtime/14__probe_multitu_fnptr_variadic_state_pipeline_main.c',
        note='multi-TU variadic function-pointer state pipeline should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/14__probe_multitu_fnptr_variadic_state_pipeline_main.c', PROBE_DIR / 'runtime/14__probe_multitu_fnptr_variadic_state_pipeline_lib.c'],
    ),
    RuntimeProbe(
        probe_id='14__probe_restrict_char_alias_rebase_matrix',
        source=PROBE_DIR / 'runtime/14__probe_restrict_char_alias_rebase_matrix.c',
        note='restrict copy with char-alias rebasing matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_ptrdiff_roundtrip_through_uintptr_matrix',
        source=PROBE_DIR / 'runtime/14__probe_ptrdiff_roundtrip_through_uintptr_matrix.c',
        note='ptrdiff roundtrip through uintptr bridge matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_multitu_ptr_alias_window_stability',
        source=PROBE_DIR / 'runtime/14__probe_multitu_ptr_alias_window_stability_main.c',
        note='multi-TU pointer alias window stability path should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/14__probe_multitu_ptr_alias_window_stability_main.c', PROBE_DIR / 'runtime/14__probe_multitu_ptr_alias_window_stability_lib.c'],
    ),
]

DIAG_PROBES = []

DIAG_JSON_PROBES = []
