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
]

DIAG_PROBES = []

DIAG_JSON_PROBES = []
