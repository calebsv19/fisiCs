from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='14__probe_unsigned_wrap',
        source=PROBE_DIR / 'runtime/14__probe_unsigned_wrap.c',
        note='unsigned wrap behavior should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_float_nan',
        source=PROBE_DIR / 'runtime/14__probe_float_nan.c',
        note='NaN self-inequality should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_float_cast_roundtrip',
        source=PROBE_DIR / 'runtime/14__probe_float_cast_roundtrip.c',
        note='float-to-int casts should truncate toward zero and match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_many_args_mixed_width',
        source=PROBE_DIR / 'runtime/14__probe_many_args_mixed_width.c',
        note='mixed-width many-arg call ABI should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_variadic_promotions_matrix',
        source=PROBE_DIR / 'runtime/14__probe_variadic_promotions_matrix.c',
        note='default argument promotions across variadic boundary should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_struct_with_array_pass_return',
        source=PROBE_DIR / 'runtime/14__probe_struct_with_array_pass_return.c',
        note='struct containing array should survive by-value pass/return path',
    ),
    RuntimeProbe(
        probe_id='14__probe_union_payload_roundtrip',
        source=PROBE_DIR / 'runtime/14__probe_union_payload_roundtrip.c',
        note='union passed/returned by value should preserve active member value',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_dispatch_table_mixed',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_dispatch_table_mixed.c',
        note='function-pointer dispatch table calls should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_switch_loop_control_mix',
        source=PROBE_DIR / 'runtime/14__probe_switch_loop_control_mix.c',
        note='switch+loop with continue/break/goto control edges should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_stride_indexing',
        source=PROBE_DIR / 'runtime/14__probe_vla_stride_indexing.c',
        note='VLA multidimensional indexing and flattened pointer-difference path should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_alignment_long_double_struct',
        source=PROBE_DIR / 'runtime/14__probe_alignment_long_double_struct.c',
        note='long-double struct alignment/offset invariants should compile and match clang behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_struct_array_byte_stride',
        source=PROBE_DIR / 'runtime/14__probe_struct_array_byte_stride.c',
        note='struct-array byte-stride invariants should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_static_pointer_array_struct_write_chain',
        source=PROBE_DIR / 'runtime/14__probe_static_pointer_array_struct_write_chain.c',
        note='file-scope compound-literal array decay should materialize static storage and remain readable through later writes',
    ),
    RuntimeProbe(
        probe_id='14__probe_union_embedded_alignment',
        source=PROBE_DIR / 'runtime/14__probe_union_embedded_alignment.c',
        note='embedded union alignment/offset invariants should compile and match clang behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_row_pointer_decay',
        source=PROBE_DIR / 'runtime/14__probe_vla_row_pointer_decay.c',
        note='VLA row-pointer decay and row-stride pointer arithmetic should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_nested_switch_fallthrough_loop',
        source=PROBE_DIR / 'runtime/14__probe_nested_switch_fallthrough_loop.c',
        note='nested switch with loop fallthrough/continue edges should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_short_circuit_side_effect_counter',
        source=PROBE_DIR / 'runtime/14__probe_short_circuit_side_effect_counter.c',
        note='short-circuit side-effect counter behavior should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_ptrdiff_row_size_dynamic',
        source=PROBE_DIR / 'runtime/14__probe_vla_ptrdiff_row_size_dynamic.c',
        note='pointer differences over VLA row pointers should scale by runtime row size',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_param_matrix_reduce',
        source=PROBE_DIR / 'runtime/14__probe_vla_param_matrix_reduce.c',
        note='VLA parameter matrix reduction should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_struct_by_value_dispatch',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_struct_by_value_dispatch.c',
        note='function-pointer dispatch over struct-by-value args/returns should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_typedef_return_direct',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_typedef_return_direct.c',
        note='typedef function-pointer returns should preserve callable pointer values in direct call paths',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_typedef_return_ternary_callee',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_typedef_return_ternary_callee.c',
        note='typedef function-pointer returns should remain callable through ternary callee expressions',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_expression_callee_chain',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_expression_callee_chain.c',
        note='function-pointer expression callee chains should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_pointer_index_width_signedness',
        source=PROBE_DIR / 'runtime/14__probe_pointer_index_width_signedness.c',
        note='pointer indexing should preserve signedness and width semantics across int and size_t indices',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_param_mixed_signed_unsigned_indices',
        source=PROBE_DIR / 'runtime/14__probe_vla_param_mixed_signed_unsigned_indices.c',
        note='VLA parameter indexing with mixed signed/unsigned index paths should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_bitfield_unsigned_pack_roundtrip',
        source=PROBE_DIR / 'runtime/14__probe_bitfield_unsigned_pack_roundtrip.c',
        note='unsigned bitfield write/read roundtrip should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_nested_return_dispatch_matrix',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_nested_return_dispatch_matrix.c',
        note='nested function-pointer return dispatch matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_three_dim_stride_reduce',
        source=PROBE_DIR / 'runtime/14__probe_vla_three_dim_stride_reduce.c',
        note='3D VLA reduction and slab-stride pointer diff should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_variadic_promotion_edges',
        source=PROBE_DIR / 'runtime/14__probe_variadic_promotion_edges.c',
        note='variadic promotion edges for signed/unsigned char and float should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_chooser_roundtrip_call',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_chooser_roundtrip_call.c',
        note='chooser function-pointer roundtrip calls should preserve full call signature and match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_three_dim_index_stride_basic',
        source=PROBE_DIR / 'runtime/14__probe_vla_three_dim_index_stride_basic.c',
        note='basic 3D VLA indexing and slab/lane stride pointer differences should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_typedef_alias_chain_dispatch',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_typedef_alias_chain_dispatch.c',
        note='typedef-alias chooser call chains should preserve nested function-pointer returns',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_chooser_table_ternary_chain',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_chooser_table_ternary_chain.c',
        note='chooser-table ternary expression callee chains should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_four_dim_stride_matrix',
        source=PROBE_DIR / 'runtime/14__probe_vla_four_dim_stride_matrix.c',
        note='4D VLA indexing and slab/lane stride pointer differences should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_struct_temporary_chain',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_struct_temporary_chain.c',
        note='temporary struct returns carrying function pointers should remain callable through chained expressions',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_param_slice_stride_rebase',
        source=PROBE_DIR / 'runtime/14__probe_vla_param_slice_stride_rebase.c',
        note='VLA parameter slice rebasing and row/element stride math should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_volatile_short_circuit_sequence',
        source=PROBE_DIR / 'runtime/14__probe_volatile_short_circuit_sequence.c',
        note='volatile state updates across comma/short-circuit sequencing should match clang behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_ptrdiff_subslice_rebase_chain',
        source=PROBE_DIR / 'runtime/14__probe_vla_ptrdiff_subslice_rebase_chain.c',
        note='VLA subslice rebasing with multi-hop row/element pointer differences should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_struct_array_dispatch_pipeline',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_struct_array_dispatch_pipeline.c',
        note='struct-wrapped function-pointer dispatch selected through array/ternary pipeline should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_ptrdiff_char_bridge_roundtrip',
        source=PROBE_DIR / 'runtime/14__probe_ptrdiff_char_bridge_roundtrip.c',
        note='typed pointer difference and char-byte bridge roundtrip should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_volatile_comma_ternary_control_chain',
        source=PROBE_DIR / 'runtime/14__probe_volatile_comma_ternary_control_chain.c',
        note='volatile state updates across comma and ternary control chains should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_variadic_width_stress_matrix',
        source=PROBE_DIR / 'runtime/14__probe_variadic_width_stress_matrix.c',
        note='variadic promotion and width-mix matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_variadic_vacopy_forwarder_matrix',
        source=PROBE_DIR / 'runtime/14__probe_variadic_vacopy_forwarder_matrix.c',
        note='va_copy forwarding across mixed variadic lane tags should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_variadic_fnptr_dispatch_chain',
        source=PROBE_DIR / 'runtime/14__probe_variadic_fnptr_dispatch_chain.c',
        note='variadic function-pointer dispatch chains should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_variadic_nested_forwarder_table',
        source=PROBE_DIR / 'runtime/14__probe_variadic_nested_forwarder_table.c',
        note='nested variadic forwarders with va_copy table dispatch should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_struct_large_return_pipeline',
        source=PROBE_DIR / 'runtime/14__probe_struct_large_return_pipeline.c',
        note='large struct pass/return merge pipelines should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_struct_large_return_fnptr_pipeline',
        source=PROBE_DIR / 'runtime/14__probe_struct_large_return_fnptr_pipeline.c',
        note='large struct returns through function-pointer builders should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_param_negative_ptrdiff_matrix',
        source=PROBE_DIR / 'runtime/14__probe_vla_param_negative_ptrdiff_matrix.c',
        note='VLA parameter row rebasing with negative/positive ptrdiff should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_rebased_slice_signed_unsigned_mix',
        source=PROBE_DIR / 'runtime/14__probe_vla_rebased_slice_signed_unsigned_mix.c',
        note='VLA rebased slices with signed/unsigned index mixing should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_ptrdiff_struct_char_bridge_matrix',
        source=PROBE_DIR / 'runtime/14__probe_ptrdiff_struct_char_bridge_matrix.c',
        note='struct-pointer typed and byte-bridge ptrdiff matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_stateful_table_loop_matrix',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_stateful_table_loop_matrix.c',
        note='stateful function-pointer table loop dispatch should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_struct_union_by_value_roundtrip_chain',
        source=PROBE_DIR / 'runtime/14__probe_struct_union_by_value_roundtrip_chain.c',
        note='struct+union by-value roundtrip chains should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_compound_scene_descriptor_nested_access',
        source=PROBE_DIR / 'runtime/14__probe_compound_scene_descriptor_nested_access.c',
        note='compound scene descriptor nested struct-array and by-value shape-copy path should match clang',
    ),
    RuntimeProbe(
        probe_id='14__probe_struct_return_nested_union_box_shape',
        source=PROBE_DIR / 'runtime/14__probe_struct_return_nested_union_box_shape.c',
        note='large struct return with nested union box payload should preserve both double fields',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_return_struct_pipeline',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_return_struct_pipeline.c',
        note='function-pointer selected struct-return pipelines should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_param_cross_function_pipeline',
        source=PROBE_DIR / 'runtime/14__probe_vla_param_cross_function_pipeline.c',
        note='cross-function VLA parameter slice pipelines should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_ptrdiff_reinterpret_longlong_bridge',
        source=PROBE_DIR / 'runtime/14__probe_ptrdiff_reinterpret_longlong_bridge.c',
        note='long-long pointer/byte reinterpret delta bridge should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_recursive_fnptr_mix_runtime',
        source=PROBE_DIR / 'runtime/14__probe_recursive_fnptr_mix_runtime.c',
        note='recursive function-pointer stepping paths should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_four_dim_slice_rebase_runtime',
        source=PROBE_DIR / 'runtime/14__probe_vla_four_dim_slice_rebase_runtime.c',
        note='4D VLA slice rebasing and stride deltas should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_alias_chooser_struct_chain',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_alias_chooser_struct_chain.c',
        note='function-pointer alias chooser chains through struct lanes should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_variadic_small_types_forward_chain',
        source=PROBE_DIR / 'runtime/14__probe_variadic_small_types_forward_chain.c',
        note='small-integer variadic promotion forwarding with va_copy should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_bitfield_truncation_struct_flow',
        source=PROBE_DIR / 'runtime/14__probe_bitfield_truncation_struct_flow.c',
        note='bitfield truncation and by-value struct flow should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_four_dim_param_handoff_reduce',
        source=PROBE_DIR / 'runtime/14__probe_vla_four_dim_param_handoff_reduce.c',
        note='4D VLA parameter handoff and reduction paths should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_alias_indirect_dispatch',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_alias_indirect_dispatch.c',
        note='function-pointer alias indirect chooser dispatch should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_variadic_promote_char_short_float_mix',
        source=PROBE_DIR / 'runtime/14__probe_variadic_promote_char_short_float_mix.c',
        note='variadic char/short/float promotion mix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_bitfield_unsigned_mask_merge_chain',
        source=PROBE_DIR / 'runtime/14__probe_bitfield_unsigned_mask_merge_chain.c',
        note='unsigned bitfield mask/merge mutation chain should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_vla_four_dim_rebase_weighted_reduce',
        source=PROBE_DIR / 'runtime/14__probe_vla_four_dim_rebase_weighted_reduce.c',
        note='4D VLA weighted reduction with rebased stride deltas should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_alias_conditional_factory_simple',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_alias_conditional_factory_simple.c',
        note='function-pointer alias conditional factory chains should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_variadic_signed_unsigned_char_matrix',
        source=PROBE_DIR / 'runtime/14__probe_variadic_signed_unsigned_char_matrix.c',
        note='variadic signed/unsigned char and short promotion matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_bitfield_by_value_roundtrip_simple',
        source=PROBE_DIR / 'runtime/14__probe_bitfield_by_value_roundtrip_simple.c',
        note='bitfield by-value roundtrip copy should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_unsigned_div_mod_extremes_matrix',
        source=PROBE_DIR / 'runtime/14__probe_unsigned_div_mod_extremes_matrix.c',
        note='unsigned division/modulo near max-width boundaries should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_signed_unsigned_cmp_boundary_matrix',
        source=PROBE_DIR / 'runtime/14__probe_signed_unsigned_cmp_boundary_matrix.c',
        note='signed/unsigned comparison boundary matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_float_signed_zero_inf_matrix',
        source=PROBE_DIR / 'runtime/14__probe_float_signed_zero_inf_matrix.c',
        note='signed zero and infinity comparison matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_cast_chain_width_sign_matrix',
        source=PROBE_DIR / 'runtime/14__probe_cast_chain_width_sign_matrix.c',
        note='signed/unsigned width cast-chain matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_stddef_ptrdiff_size_t_bridge',
        source=PROBE_DIR / 'runtime/14__probe_header_stddef_ptrdiff_size_t_bridge.c',
        note='stddef bridge for ptrdiff_t/size_t/offsetof should compile and match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_stdint_intptr_uintptr_roundtrip',
        source=PROBE_DIR / 'runtime/14__probe_header_stdint_intptr_uintptr_roundtrip.c',
        note='stdint intptr/uintptr pointer roundtrip bridge should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_limits_llong_matrix',
        source=PROBE_DIR / 'runtime/14__probe_header_limits_llong_matrix.c',
        note='limits.h long-long boundary macro usage should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_stdbool_int_bridge',
        source=PROBE_DIR / 'runtime/14__probe_header_stdbool_int_bridge.c',
        note='stdbool bool/int bridge semantics should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_stdalign_bridge',
        source=PROBE_DIR / 'runtime/14__probe_header_stdalign_bridge.c',
        note='stdalign bridge semantics should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_stdint_limits_crosscheck',
        source=PROBE_DIR / 'runtime/14__probe_header_stdint_limits_crosscheck.c',
        note='stdint limits crosscheck should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_null_sizeof_ptrdiff_bridge',
        source=PROBE_DIR / 'runtime/14__probe_header_null_sizeof_ptrdiff_bridge.c',
        note='NULL/sizeof/ptrdiff header bridge should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_string_tokenize_tail_matrix',
        source=PROBE_DIR / 'runtime/14__probe_header_string_tokenize_tail_matrix.c',
        note='string.h tokenization and tail-search behavior should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_wchar_tokenize_tail_matrix',
        source=PROBE_DIR / 'runtime/14__probe_header_wchar_tokenize_tail_matrix.c',
        note='wchar.h tokenization and tail-search behavior should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_string_span_copy_window_matrix',
        source=PROBE_DIR / 'runtime/14__probe_header_string_span_copy_window_matrix.c',
        note='string.h copy/search/span window behavior should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_stdlib_div_abs_matrix',
        source=PROBE_DIR / 'runtime/14__probe_header_stdlib_div_abs_matrix.c',
        note='stdlib abs/div family behavior should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_stdio_snprintf_n_scan_matrix',
        source=PROBE_DIR / 'runtime/14__probe_header_stdio_snprintf_n_scan_matrix.c',
        note='stdio snprintf/sscanf/%n behavior should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_struct_bitfield_mixed_pass_return',
        source=PROBE_DIR / 'runtime/14__probe_struct_bitfield_mixed_pass_return.c',
        note='mixed-width bitfield struct pass/return paths should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_struct_double_int_padding_roundtrip',
        source=PROBE_DIR / 'runtime/14__probe_struct_double_int_padding_roundtrip.c',
        note='double/int padded struct by-value roundtrip should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_fnptr_variadic_dispatch_bridge',
        source=PROBE_DIR / 'runtime/14__probe_fnptr_variadic_dispatch_bridge.c',
        note='function-pointer variadic dispatch bridge should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_many_args_struct_scalar_mix',
        source=PROBE_DIR / 'runtime/14__probe_many_args_struct_scalar_mix.c',
        note='many-argument struct/scalar ABI mix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_float_negzero_propagation_chain',
        source=PROBE_DIR / 'runtime/14__probe_float_negzero_propagation_chain.c',
        note='negative-zero propagation across float arithmetic chain should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_ptrdiff_one_past_end_matrix',
        source=PROBE_DIR / 'runtime/14__probe_ptrdiff_one_past_end_matrix.c',
        note='one-past-end pointer comparisons and ptrdiff scaling should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_many_args_float_int_struct_mix',
        source=PROBE_DIR / 'runtime/14__probe_many_args_float_int_struct_mix.c',
        note='many-arg ABI lane with struct/int/double mix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_abi_reg_stack_frontier_matrix',
        source=PROBE_DIR / 'runtime/14__probe_abi_reg_stack_frontier_matrix.c',
        note='ABI reg/stack frontier matrix across mixed scalar+struct arguments should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_abi_mixed_struct_float_boundary',
        source=PROBE_DIR / 'runtime/14__probe_abi_mixed_struct_float_boundary.c',
        note='mixed struct/float boundary pass-return ABI behavior should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_variadic_abi_reg_stack_frontier',
        source=PROBE_DIR / 'runtime/14__probe_variadic_abi_reg_stack_frontier.c',
        note='variadic ABI reg/stack frontier behavior should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_static_local_init_once_chain',
        source=PROBE_DIR / 'runtime/14__probe_static_local_init_once_chain.c',
        note='static local state init/persistence chain should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_static_local_init_recursion_gate',
        source=PROBE_DIR / 'runtime/14__probe_static_local_init_recursion_gate.c',
        note='static local recursion gate and state persistence should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_static_local_checkpoint_replay_ladder',
        source=PROBE_DIR / 'runtime/14__probe_static_local_checkpoint_replay_ladder.c',
        note='static local checkpoint/replay ladder should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_static_local_alias_window_feedback',
        source=PROBE_DIR / 'runtime/14__probe_static_local_alias_window_feedback.c',
        note='static local alias-window feedback should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_static_local_owner_epoch_rollback',
        source=PROBE_DIR / 'runtime/14__probe_static_local_owner_epoch_rollback.c',
        note='static local owner/epoch rollback state should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_static_local_alias_guard_reclaim',
        source=PROBE_DIR / 'runtime/14__probe_static_local_alias_guard_reclaim.c',
        note='static local alias/guard reclaim state should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_pointer_window_borrow_replay_matrix',
        source=PROBE_DIR / 'runtime/14__probe_pointer_window_borrow_replay_matrix.c',
        note='pointer-window borrow/replay matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_pointer_window_stale_handoff_guard',
        source=PROBE_DIR / 'runtime/14__probe_pointer_window_stale_handoff_guard.c',
        note='pointer-window stale/handoff guard path should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_pointer_lifetime_epoch_invalidation_matrix',
        source=PROBE_DIR / 'runtime/14__probe_pointer_lifetime_epoch_invalidation_matrix.c',
        note='pointer-lifetime epoch invalidation matrix should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_pointer_lifetime_borrow_trim_feedback',
        source=PROBE_DIR / 'runtime/14__probe_pointer_lifetime_borrow_trim_feedback.c',
        note='pointer-lifetime borrow/trim feedback should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_pointer_lifetime_reseed_handoff_lattice',
        source=PROBE_DIR / 'runtime/14__probe_pointer_lifetime_reseed_handoff_lattice.c',
        note='pointer-lifetime reseed/handoff lattice should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_setjmp_direct_longjmp_runtime',
        source=PROBE_DIR / 'runtime/14__probe_header_setjmp_direct_longjmp_runtime.c',
        note='setjmp.h direct longjmp control transfer should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_header_setjmp_helper_longjmp_runtime',
        source=PROBE_DIR / 'runtime/14__probe_header_setjmp_helper_longjmp_runtime.c',
        note='setjmp.h helper-mediated longjmp control transfer should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave332_nested_payload_return_boundary',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave332_nested_payload_return_boundary.c',
        note='wave332 nested aggregate payload return-slot rewrites should preserve union byte lanes across boundary copies',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave332_callback_payload_preserve_boundary',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave332_callback_payload_preserve_boundary.c',
        note='wave332 callback/table-dispatch aggregate payload rewrites should preserve union byte lanes',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave332_compound_literal_payload_boundary',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave332_compound_literal_payload_boundary.c',
        note='wave332 compound-literal aggregate payload assignments should preserve nested union bytes',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave333_nested_aggregate_copy_payload',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave333_nested_aggregate_copy_payload.c',
        note='wave333 nested aggregate copy chains should preserve union array payload bytes and checksum parity',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave333_callback_selected_payload_chain',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave333_callback_selected_payload_chain.c',
        note='wave333 callback-selected aggregate payload chains should preserve selected union byte lanes',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave333_union_array_overlay_checksum',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave333_union_array_overlay_checksum.c',
        note='wave333 union/array overlay rewrites should preserve byte lanes and runtime checksum parity',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave333_runtime_checksum_parity',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave333_runtime_checksum_parity.c',
        note='wave333 callback-mapped nested aggregate rewrites should match clang runtime checksum parity',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave334_nested_copy_return_slot_payload',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave334_nested_copy_return_slot_payload.c',
        note='wave334 nested aggregate copy and return-slot rewrites should preserve union array payload bytes',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave334_union_array_memcpy_uintptr_payload',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave334_union_array_memcpy_uintptr_payload.c',
        note='wave334 memcpy/uintptr aggregate handoffs should preserve union overlay payload bytes',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave334_callback_selected_header_payload',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave334_callback_selected_header_payload.c',
        note='wave334 callback-selected header-backed aggregate rewrites should preserve nested payload bytes',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave334_array_overlay_return_slot_checksum',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave334_array_overlay_return_slot_checksum.c',
        note='wave334 array overlay return-slot checksums should preserve nested aggregate payload bytes',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave335_nested_union_return_slot_payload',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave335_nested_union_return_slot_payload.c',
        note='wave335 nested union return-slot rewrites should preserve payload bytes',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave335_stdlib_qsort_nested_payload',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave335_stdlib_qsort_nested_payload.c',
        note='wave335 qsort callback aggregate rewrites should preserve nested union payload bytes',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave335_header_memcpy_bsearch_payload',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave335_header_memcpy_bsearch_payload.c',
        note='wave335 bsearch/memcpy aggregate handoffs should preserve selected payload bytes',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave335_physics_units_aggregate_matrix',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave335_physics_units_aggregate_matrix.c',
        note='wave335 physics-units aggregate matrix rewrites should preserve runtime values',
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave336_atexit_static_payload',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave336_atexit_static_payload.c',
        note='wave336 atexit handlers should preserve static aggregate payload state in LIFO order',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave336_realloc_callback_owner_payload',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave336_realloc_callback_owner_payload.c',
        note='wave336 realloc ownership transfer and callback mutation should preserve aggregate payload state',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave337_rand_aggregate_state_progression',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave337_rand_aggregate_state_progression.c',
        note='wave337 srand/rand state progression should preserve aggregate payload values across reseeding',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave337_rand_aggregate_callback_reseed',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave337_rand_aggregate_callback_reseed.c',
        note='wave337 callback-built aggregate frames should preserve rand state across reseeding',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave338_overaligned_tail_array_stride',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave338_overaligned_tail_array_stride.c',
        note='wave338 over-aligned aggregate tail padding should preserve 32-byte array stride and payload values',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave338_overaligned_multifield_tail_stride',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave338_overaligned_multifield_tail_stride.c',
        note='wave338 multi-field over-aligned aggregates should preserve semantic size, tail offset, and array stride',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave338_nested_overaligned_tail_stride',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave338_nested_overaligned_tail_stride.c',
        note='wave338 nested over-aligned aggregate tail padding should propagate through containing layout and initialization',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave339_alignas_internal_gap_automatic',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave339_alignas_internal_gap_automatic.c',
        note='wave339 automatic alignas aggregates should preserve internal member gaps across positional and designated initialization',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave339_alignas_internal_gap_static_array',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave339_alignas_internal_gap_static_array.c',
        note='wave339 static alignas aggregate arrays should preserve physical padding, constant payloads, and semantic stride',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave339_alignas_internal_gap_nested',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave339_alignas_internal_gap_nested.c',
        note='wave339 nested alignas aggregates should propagate internal layout and array stride through containing records',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave340_overaligned_vla_base_stride',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave340_overaligned_vla_base_stride.c',
        note='wave340 over-aligned VLA storage should preserve semantic base alignment and element stride',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave341_overaligned_union_storage',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave341_overaligned_union_storage.c',
        note='wave341 over-aligned union storage should preserve semantic alignment, containing offsets, array stride, and payload initialization',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave342_overaligned_union_float_storage',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave342_overaligned_union_float_storage.c',
        note='wave342 over-aligned union constant storage should preserve floating-point payload bits and array stride',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave343_overaligned_union_pointer_storage',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave343_overaligned_union_pointer_storage.c',
        note='wave343 over-aligned union constant storage should preserve pointer relocations and array stride',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave344_overaligned_union_nested_pointer_storage',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave344_overaligned_union_nested_pointer_storage.c',
        note='wave344 over-aligned union constant storage should preserve a nested pointer relocation when another member determines the physical carrier',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave345_overaligned_union_dual_pointer_carrier',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave345_overaligned_union_dual_pointer_carrier.c',
        note='wave345 over-aligned union constant storage should preserve relocations from two distinct pointer-bearing member layouts',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave346_overaligned_union_shifted_pointer_relocation',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave346_overaligned_union_shifted_pointer_relocation.c',
        note='wave346 over-aligned union constant storage should preserve a shifted pointer through an integer carrier field relocation',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave347_overaligned_union_multi_relocation_carrier',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave347_overaligned_union_multi_relocation_carrier.c',
        note='wave347 over-aligned union constant storage should preserve simultaneous data and function pointer relocations across mixed carrier fields',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave348_overaligned_union_relocation_addends',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave348_overaligned_union_relocation_addends.c',
        note='wave348 over-aligned union constant storage should preserve array-element and struct-field relocation addends through integer carrier fields',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave349_overaligned_union_pointer_arrays',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave349_overaligned_union_pointer_arrays.c',
        note='wave349 over-aligned union constant storage should preserve nested arrays of data and function pointers through recursive carrier reconstruction',
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave350_overaligned_union_external_relocations_all_fisics',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave350_overaligned_union_external_relocations_main.c',
        note='wave350 all-fisiCs multi-TU union storage should preserve external array-addend and function relocations',
        inputs=[
            PROBE_DIR / 'runtime/14__probe_runtime_wave350_overaligned_union_external_relocations_main.c',
            PROBE_DIR / 'runtime/14__probe_runtime_wave350_overaligned_union_external_relocations_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave350_overaligned_union_external_relocations_mixed_clang_callee',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave350_overaligned_union_external_relocations_main.c',
        note='wave350 fisiCs union storage should resolve external relocations supplied by a Clang callee object',
        inputs=[
            PROBE_DIR / 'runtime/14__probe_runtime_wave350_overaligned_union_external_relocations_main.c',
        ],
        mixed_clang_inputs=[
            PROBE_DIR / 'runtime/14__probe_runtime_wave350_overaligned_union_external_relocations_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave351_overaligned_union_external_relocations_all_fisics',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave351_overaligned_union_external_relocations_main.c',
        note='wave351 all-fisiCs multi-TU union object should preserve external relocations owned by the caller translation unit',
        inputs=[
            PROBE_DIR / 'runtime/14__probe_runtime_wave351_overaligned_union_external_relocations_main.c',
            PROBE_DIR / 'runtime/14__probe_runtime_wave351_overaligned_union_external_relocations_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave351_overaligned_union_external_relocations_reverse_clang_caller',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave351_overaligned_union_external_relocations_lib.c',
        note='wave351 fisiCs relocation-bearing union object should link and execute under a reverse Clang caller',
        inputs=[
            PROBE_DIR / 'runtime/14__probe_runtime_wave351_overaligned_union_external_relocations_lib.c',
        ],
        mixed_clang_inputs=[
            PROBE_DIR / 'runtime/14__probe_runtime_wave351_overaligned_union_external_relocations_main.c',
        ],
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave352_overaligned_union_weak_strong_relocation',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave352_overaligned_union_weak_strong_relocation_main.c',
        note='wave352 union relocations should bind to strong data and function definitions over weak fallbacks',
        inputs=[
            PROBE_DIR / 'runtime/14__probe_runtime_wave352_overaligned_union_weak_strong_relocation_main.c',
            PROBE_DIR / 'runtime/14__probe_runtime_wave352_overaligned_union_weak_strong_relocation_weak.c',
            PROBE_DIR / 'runtime/14__probe_runtime_wave352_overaligned_union_weak_strong_relocation_strong.c',
        ],
    ),
    RuntimeProbe(
        probe_id='14__probe_runtime_wave353_overaligned_union_static_extern_shadow',
        source=PROBE_DIR / 'runtime/14__probe_runtime_wave353_overaligned_union_static_extern_shadow_main.c',
        note='wave353 union relocations should preserve distinct TU-local static and external same-name symbol identities',
        inputs=[
            PROBE_DIR / 'runtime/14__probe_runtime_wave353_overaligned_union_static_extern_shadow_main.c',
            PROBE_DIR / 'runtime/14__probe_runtime_wave353_overaligned_union_static_extern_shadow_lib.c',
        ],
    ),
]

DIAG_PROBES = []

DIAG_JSON_PROBES = []
