#!/usr/bin/env python3
import json
import os
import shutil
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence


PROBE_DIR = Path(__file__).resolve().parent
REPO_ROOT = PROBE_DIR.parent.parent.parent
FISICS = REPO_ROOT / "fisics"
COMPILE_TIMEOUT_SEC = 20
RUN_TIMEOUT_SEC = 8


@dataclass
class RuntimeProbe:
    probe_id: str
    source: Path
    note: str
    inputs: Sequence[Path] | None = None
    extra_differential_compiler: str | None = None


@dataclass
class DiagnosticProbe:
    probe_id: str
    source: Path
    note: str
    required_substrings: Sequence[str] | None = None
    inputs: Sequence[Path] | None = None


@dataclass
class DiagnosticJsonProbe:
    probe_id: str
    source: Path
    note: str
    require_any_diagnostic: bool = True
    expected_codes: Sequence[int] | None = None
    expected_line: int | None = None
    inputs: Sequence[Path] | None = None


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="04__probe_fnptr_array_call_runtime",
        source=PROBE_DIR / "runtime/04__probe_fnptr_array_call_runtime.c",
        note="function-pointer array call path should match clang runtime result",
    ),
    RuntimeProbe(
        probe_id="04__probe_tag_block_shadow_ok",
        source=PROBE_DIR / "runtime/04__probe_tag_block_shadow_ok.c",
        note="inner-scope struct tag shadowing should compile/run cleanly",
    ),
    RuntimeProbe(
        probe_id="04__probe_deep_declarator_call_only",
        source=PROBE_DIR / "runtime/04__probe_deep_declarator_call_only.c",
        note="deep declarator call-only path should compile/run cleanly",
    ),
    RuntimeProbe(
        probe_id="04__probe_deep_declarator_codegen_hang",
        source=PROBE_DIR / "runtime/04__probe_deep_declarator_codegen_hang.c",
        note="deep declarator runtime path should compile and run (no codegen hang)",
    ),
    RuntimeProbe(
        probe_id="04__probe_union_overlap_runtime",
        source=PROBE_DIR / "runtime/04__probe_union_overlap_runtime.c",
        note="union members should overlap storage base address and satisfy size floor checks",
    ),
    RuntimeProbe(
        probe_id="07__probe_agg_member_access_runtime",
        source=PROBE_DIR / "runtime/07__probe_agg_member_access_runtime.c",
        note="nested struct member access via '.' and '->' should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="07__probe_agg_offsets_runtime",
        source=PROBE_DIR / "runtime/07__probe_agg_offsets_runtime.c",
        note="offsetof ordering and stride invariants should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="10__probe_static_function_then_extern_decl_ok",
        source=PROBE_DIR / "runtime/10__probe_static_function_then_extern_decl_ok.c",
        note="file-scope static function followed by extern declaration should remain valid",
    ),
    RuntimeProbe(
        probe_id="11__probe_function_param_function_type_adjust_ok",
        source=PROBE_DIR / "runtime/11__probe_function_param_function_type_adjust_ok.c",
        note="function parameter declared as function type should adjust to pointer and accept matching function argument",
    ),
    RuntimeProbe(
        probe_id="13__probe_phi_continue_runtime",
        source=PROBE_DIR / "runtime/13__probe_phi_continue_runtime.c",
        note="loop-carried accumulator with continue/break edges should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="13__probe_short_circuit_chain_runtime",
        source=PROBE_DIR / "runtime/13__probe_short_circuit_chain_runtime.c",
        note="nested short-circuit chain side effects should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="13__probe_struct_copy_runtime",
        source=PROBE_DIR / "runtime/13__probe_struct_copy_runtime.c",
        note="struct return and by-value copy/update path should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="13__probe_ptr_stride_runtime",
        source=PROBE_DIR / "runtime/13__probe_ptr_stride_runtime.c",
        note="pointer-difference scaling and indexed pointer loads should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="13__probe_global_init_runtime",
        source=PROBE_DIR / "runtime/13__probe_global_init_runtime.c",
        note="global partial initializer and zero-fill behavior should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="13__probe_fnptr_ternary_decay_runtime",
        source=PROBE_DIR / "runtime/13__probe_fnptr_ternary_decay_runtime.c",
        note="function designators in ternary should decay to compatible function pointers",
    ),
    RuntimeProbe(
        probe_id="14__probe_unsigned_wrap",
        source=PROBE_DIR / "runtime/14__probe_unsigned_wrap.c",
        note="unsigned wrap behavior should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_float_nan",
        source=PROBE_DIR / "runtime/14__probe_float_nan.c",
        note="NaN self-inequality should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_float_cast_roundtrip",
        source=PROBE_DIR / "runtime/14__probe_float_cast_roundtrip.c",
        note="float-to-int casts should truncate toward zero and match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_many_args_mixed_width",
        source=PROBE_DIR / "runtime/14__probe_many_args_mixed_width.c",
        note="mixed-width many-arg call ABI should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_variadic_promotions_matrix",
        source=PROBE_DIR / "runtime/14__probe_variadic_promotions_matrix.c",
        note="default argument promotions across variadic boundary should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_struct_with_array_pass_return",
        source=PROBE_DIR / "runtime/14__probe_struct_with_array_pass_return.c",
        note="struct containing array should survive by-value pass/return path",
    ),
    RuntimeProbe(
        probe_id="14__probe_union_payload_roundtrip",
        source=PROBE_DIR / "runtime/14__probe_union_payload_roundtrip.c",
        note="union passed/returned by value should preserve active member value",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_dispatch_table_mixed",
        source=PROBE_DIR / "runtime/14__probe_fnptr_dispatch_table_mixed.c",
        note="function-pointer dispatch table calls should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_switch_loop_control_mix",
        source=PROBE_DIR / "runtime/14__probe_switch_loop_control_mix.c",
        note="switch+loop with continue/break/goto control edges should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_stride_indexing",
        source=PROBE_DIR / "runtime/14__probe_vla_stride_indexing.c",
        note="VLA multidimensional indexing and flattened pointer-difference path should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_alignment_long_double_struct",
        source=PROBE_DIR / "runtime/14__probe_alignment_long_double_struct.c",
        note="long-double struct alignment/offset invariants should compile and match clang behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_struct_array_byte_stride",
        source=PROBE_DIR / "runtime/14__probe_struct_array_byte_stride.c",
        note="struct-array byte-stride invariants should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_union_embedded_alignment",
        source=PROBE_DIR / "runtime/14__probe_union_embedded_alignment.c",
        note="embedded union alignment/offset invariants should compile and match clang behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_row_pointer_decay",
        source=PROBE_DIR / "runtime/14__probe_vla_row_pointer_decay.c",
        note="VLA row-pointer decay and row-stride pointer arithmetic should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_nested_switch_fallthrough_loop",
        source=PROBE_DIR / "runtime/14__probe_nested_switch_fallthrough_loop.c",
        note="nested switch with loop fallthrough/continue edges should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_short_circuit_side_effect_counter",
        source=PROBE_DIR / "runtime/14__probe_short_circuit_side_effect_counter.c",
        note="short-circuit side-effect counter behavior should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_ptrdiff_row_size_dynamic",
        source=PROBE_DIR / "runtime/14__probe_vla_ptrdiff_row_size_dynamic.c",
        note="pointer differences over VLA row pointers should scale by runtime row size",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_param_matrix_reduce",
        source=PROBE_DIR / "runtime/14__probe_vla_param_matrix_reduce.c",
        note="VLA parameter matrix reduction should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_struct_by_value_dispatch",
        source=PROBE_DIR / "runtime/14__probe_fnptr_struct_by_value_dispatch.c",
        note="function-pointer dispatch over struct-by-value args/returns should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_typedef_return_direct",
        source=PROBE_DIR / "runtime/14__probe_fnptr_typedef_return_direct.c",
        note="typedef function-pointer returns should preserve callable pointer values in direct call paths",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_typedef_return_ternary_callee",
        source=PROBE_DIR / "runtime/14__probe_fnptr_typedef_return_ternary_callee.c",
        note="typedef function-pointer returns should remain callable through ternary callee expressions",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_expression_callee_chain",
        source=PROBE_DIR / "runtime/14__probe_fnptr_expression_callee_chain.c",
        note="function-pointer expression callee chains should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_pointer_index_width_signedness",
        source=PROBE_DIR / "runtime/14__probe_pointer_index_width_signedness.c",
        note="pointer indexing should preserve signedness and width semantics across int and size_t indices",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_param_mixed_signed_unsigned_indices",
        source=PROBE_DIR / "runtime/14__probe_vla_param_mixed_signed_unsigned_indices.c",
        note="VLA parameter indexing with mixed signed/unsigned index paths should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_bitfield_unsigned_pack_roundtrip",
        source=PROBE_DIR / "runtime/14__probe_bitfield_unsigned_pack_roundtrip.c",
        note="unsigned bitfield write/read roundtrip should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_nested_return_dispatch_matrix",
        source=PROBE_DIR / "runtime/14__probe_fnptr_nested_return_dispatch_matrix.c",
        note="nested function-pointer return dispatch matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_three_dim_stride_reduce",
        source=PROBE_DIR / "runtime/14__probe_vla_three_dim_stride_reduce.c",
        note="3D VLA reduction and slab-stride pointer diff should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_variadic_promotion_edges",
        source=PROBE_DIR / "runtime/14__probe_variadic_promotion_edges.c",
        note="variadic promotion edges for signed/unsigned char and float should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_chooser_roundtrip_call",
        source=PROBE_DIR / "runtime/14__probe_fnptr_chooser_roundtrip_call.c",
        note="chooser function-pointer roundtrip calls should preserve full call signature and match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_three_dim_index_stride_basic",
        source=PROBE_DIR / "runtime/14__probe_vla_three_dim_index_stride_basic.c",
        note="basic 3D VLA indexing and slab/lane stride pointer differences should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_typedef_alias_chain_dispatch",
        source=PROBE_DIR / "runtime/14__probe_fnptr_typedef_alias_chain_dispatch.c",
        note="typedef-alias chooser call chains should preserve nested function-pointer returns",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_chooser_table_ternary_chain",
        source=PROBE_DIR / "runtime/14__probe_fnptr_chooser_table_ternary_chain.c",
        note="chooser-table ternary expression callee chains should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_four_dim_stride_matrix",
        source=PROBE_DIR / "runtime/14__probe_vla_four_dim_stride_matrix.c",
        note="4D VLA indexing and slab/lane stride pointer differences should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_struct_temporary_chain",
        source=PROBE_DIR / "runtime/14__probe_fnptr_struct_temporary_chain.c",
        note="temporary struct returns carrying function pointers should remain callable through chained expressions",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_param_slice_stride_rebase",
        source=PROBE_DIR / "runtime/14__probe_vla_param_slice_stride_rebase.c",
        note="VLA parameter slice rebasing and row/element stride math should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_volatile_short_circuit_sequence",
        source=PROBE_DIR / "runtime/14__probe_volatile_short_circuit_sequence.c",
        note="volatile state updates across comma/short-circuit sequencing should match clang behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_ptrdiff_subslice_rebase_chain",
        source=PROBE_DIR / "runtime/14__probe_vla_ptrdiff_subslice_rebase_chain.c",
        note="VLA subslice rebasing with multi-hop row/element pointer differences should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_struct_array_dispatch_pipeline",
        source=PROBE_DIR / "runtime/14__probe_fnptr_struct_array_dispatch_pipeline.c",
        note="struct-wrapped function-pointer dispatch selected through array/ternary pipeline should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_ptrdiff_char_bridge_roundtrip",
        source=PROBE_DIR / "runtime/14__probe_ptrdiff_char_bridge_roundtrip.c",
        note="typed pointer difference and char-byte bridge roundtrip should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_volatile_comma_ternary_control_chain",
        source=PROBE_DIR / "runtime/14__probe_volatile_comma_ternary_control_chain.c",
        note="volatile state updates across comma and ternary control chains should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_variadic_width_stress_matrix",
        source=PROBE_DIR / "runtime/14__probe_variadic_width_stress_matrix.c",
        note="variadic promotion and width-mix matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_variadic_vacopy_forwarder_matrix",
        source=PROBE_DIR / "runtime/14__probe_variadic_vacopy_forwarder_matrix.c",
        note="va_copy forwarding across mixed variadic lane tags should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_variadic_fnptr_dispatch_chain",
        source=PROBE_DIR / "runtime/14__probe_variadic_fnptr_dispatch_chain.c",
        note="variadic function-pointer dispatch chains should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_variadic_nested_forwarder_table",
        source=PROBE_DIR / "runtime/14__probe_variadic_nested_forwarder_table.c",
        note="nested variadic forwarders with va_copy table dispatch should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_struct_large_return_pipeline",
        source=PROBE_DIR / "runtime/14__probe_struct_large_return_pipeline.c",
        note="large struct pass/return merge pipelines should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_struct_large_return_fnptr_pipeline",
        source=PROBE_DIR / "runtime/14__probe_struct_large_return_fnptr_pipeline.c",
        note="large struct returns through function-pointer builders should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_param_negative_ptrdiff_matrix",
        source=PROBE_DIR / "runtime/14__probe_vla_param_negative_ptrdiff_matrix.c",
        note="VLA parameter row rebasing with negative/positive ptrdiff should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_rebased_slice_signed_unsigned_mix",
        source=PROBE_DIR / "runtime/14__probe_vla_rebased_slice_signed_unsigned_mix.c",
        note="VLA rebased slices with signed/unsigned index mixing should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_ptrdiff_struct_char_bridge_matrix",
        source=PROBE_DIR / "runtime/14__probe_ptrdiff_struct_char_bridge_matrix.c",
        note="struct-pointer typed and byte-bridge ptrdiff matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_stateful_table_loop_matrix",
        source=PROBE_DIR / "runtime/14__probe_fnptr_stateful_table_loop_matrix.c",
        note="stateful function-pointer table loop dispatch should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_struct_union_by_value_roundtrip_chain",
        source=PROBE_DIR / "runtime/14__probe_struct_union_by_value_roundtrip_chain.c",
        note="struct+union by-value roundtrip chains should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_return_struct_pipeline",
        source=PROBE_DIR / "runtime/14__probe_fnptr_return_struct_pipeline.c",
        note="function-pointer selected struct-return pipelines should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_param_cross_function_pipeline",
        source=PROBE_DIR / "runtime/14__probe_vla_param_cross_function_pipeline.c",
        note="cross-function VLA parameter slice pipelines should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_ptrdiff_reinterpret_longlong_bridge",
        source=PROBE_DIR / "runtime/14__probe_ptrdiff_reinterpret_longlong_bridge.c",
        note="long-long pointer/byte reinterpret delta bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_recursive_fnptr_mix_runtime",
        source=PROBE_DIR / "runtime/14__probe_recursive_fnptr_mix_runtime.c",
        note="recursive function-pointer stepping paths should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_four_dim_slice_rebase_runtime",
        source=PROBE_DIR / "runtime/14__probe_vla_four_dim_slice_rebase_runtime.c",
        note="4D VLA slice rebasing and stride deltas should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_alias_chooser_struct_chain",
        source=PROBE_DIR / "runtime/14__probe_fnptr_alias_chooser_struct_chain.c",
        note="function-pointer alias chooser chains through struct lanes should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_variadic_small_types_forward_chain",
        source=PROBE_DIR / "runtime/14__probe_variadic_small_types_forward_chain.c",
        note="small-integer variadic promotion forwarding with va_copy should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_bitfield_truncation_struct_flow",
        source=PROBE_DIR / "runtime/14__probe_bitfield_truncation_struct_flow.c",
        note="bitfield truncation and by-value struct flow should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_four_dim_param_handoff_reduce",
        source=PROBE_DIR / "runtime/14__probe_vla_four_dim_param_handoff_reduce.c",
        note="4D VLA parameter handoff and reduction paths should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_alias_indirect_dispatch",
        source=PROBE_DIR / "runtime/14__probe_fnptr_alias_indirect_dispatch.c",
        note="function-pointer alias indirect chooser dispatch should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_variadic_promote_char_short_float_mix",
        source=PROBE_DIR / "runtime/14__probe_variadic_promote_char_short_float_mix.c",
        note="variadic char/short/float promotion mix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_bitfield_unsigned_mask_merge_chain",
        source=PROBE_DIR / "runtime/14__probe_bitfield_unsigned_mask_merge_chain.c",
        note="unsigned bitfield mask/merge mutation chain should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_four_dim_rebase_weighted_reduce",
        source=PROBE_DIR / "runtime/14__probe_vla_four_dim_rebase_weighted_reduce.c",
        note="4D VLA weighted reduction with rebased stride deltas should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_alias_conditional_factory_simple",
        source=PROBE_DIR / "runtime/14__probe_fnptr_alias_conditional_factory_simple.c",
        note="function-pointer alias conditional factory chains should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_variadic_signed_unsigned_char_matrix",
        source=PROBE_DIR / "runtime/14__probe_variadic_signed_unsigned_char_matrix.c",
        note="variadic signed/unsigned char and short promotion matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_bitfield_by_value_roundtrip_simple",
        source=PROBE_DIR / "runtime/14__probe_bitfield_by_value_roundtrip_simple.c",
        note="bitfield by-value roundtrip copy should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_unsigned_div_mod_extremes_matrix",
        source=PROBE_DIR / "runtime/14__probe_unsigned_div_mod_extremes_matrix.c",
        note="unsigned division/modulo near max-width boundaries should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_signed_unsigned_cmp_boundary_matrix",
        source=PROBE_DIR / "runtime/14__probe_signed_unsigned_cmp_boundary_matrix.c",
        note="signed/unsigned comparison boundary matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_float_signed_zero_inf_matrix",
        source=PROBE_DIR / "runtime/14__probe_float_signed_zero_inf_matrix.c",
        note="signed zero and infinity comparison matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_cast_chain_width_sign_matrix",
        source=PROBE_DIR / "runtime/14__probe_cast_chain_width_sign_matrix.c",
        note="signed/unsigned width cast-chain matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_header_stddef_ptrdiff_size_t_bridge",
        source=PROBE_DIR / "runtime/14__probe_header_stddef_ptrdiff_size_t_bridge.c",
        note="stddef bridge for ptrdiff_t/size_t/offsetof should compile and match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_header_stdint_intptr_uintptr_roundtrip",
        source=PROBE_DIR / "runtime/14__probe_header_stdint_intptr_uintptr_roundtrip.c",
        note="stdint intptr/uintptr pointer roundtrip bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_header_limits_llong_matrix",
        source=PROBE_DIR / "runtime/14__probe_header_limits_llong_matrix.c",
        note="limits.h long-long boundary macro usage should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_header_stdbool_int_bridge",
        source=PROBE_DIR / "runtime/14__probe_header_stdbool_int_bridge.c",
        note="stdbool bool/int bridge semantics should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_header_stdalign_bridge",
        source=PROBE_DIR / "runtime/14__probe_header_stdalign_bridge.c",
        note="stdalign bridge semantics should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_header_stdint_limits_crosscheck",
        source=PROBE_DIR / "runtime/14__probe_header_stdint_limits_crosscheck.c",
        note="stdint limits crosscheck should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_header_null_sizeof_ptrdiff_bridge",
        source=PROBE_DIR / "runtime/14__probe_header_null_sizeof_ptrdiff_bridge.c",
        note="NULL/sizeof/ptrdiff header bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_struct_bitfield_mixed_pass_return",
        source=PROBE_DIR / "runtime/14__probe_struct_bitfield_mixed_pass_return.c",
        note="mixed-width bitfield struct pass/return paths should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_struct_double_int_padding_roundtrip",
        source=PROBE_DIR / "runtime/14__probe_struct_double_int_padding_roundtrip.c",
        note="double/int padded struct by-value roundtrip should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_variadic_dispatch_bridge",
        source=PROBE_DIR / "runtime/14__probe_fnptr_variadic_dispatch_bridge.c",
        note="function-pointer variadic dispatch bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_many_args_struct_scalar_mix",
        source=PROBE_DIR / "runtime/14__probe_many_args_struct_scalar_mix.c",
        note="many-argument struct/scalar ABI mix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_float_negzero_propagation_chain",
        source=PROBE_DIR / "runtime/14__probe_float_negzero_propagation_chain.c",
        note="negative-zero propagation across float arithmetic chain should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_ptrdiff_one_past_end_matrix",
        source=PROBE_DIR / "runtime/14__probe_ptrdiff_one_past_end_matrix.c",
        note="one-past-end pointer comparisons and ptrdiff scaling should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_many_args_float_int_struct_mix",
        source=PROBE_DIR / "runtime/14__probe_many_args_float_int_struct_mix.c",
        note="many-arg ABI lane with struct/int/double mix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_abi_reg_stack_frontier_matrix",
        source=PROBE_DIR / "runtime/14__probe_abi_reg_stack_frontier_matrix.c",
        note="ABI reg/stack frontier matrix across mixed scalar+struct arguments should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_abi_mixed_struct_float_boundary",
        source=PROBE_DIR / "runtime/14__probe_abi_mixed_struct_float_boundary.c",
        note="mixed struct/float boundary pass-return ABI behavior should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_variadic_abi_reg_stack_frontier",
        source=PROBE_DIR / "runtime/14__probe_variadic_abi_reg_stack_frontier.c",
        note="variadic ABI reg/stack frontier behavior should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_static_local_init_once_chain",
        source=PROBE_DIR / "runtime/14__probe_static_local_init_once_chain.c",
        note="static local state init/persistence chain should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_static_local_init_recursion_gate",
        source=PROBE_DIR / "runtime/14__probe_static_local_init_recursion_gate.c",
        note="static local recursion gate and state persistence should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_static_init_visibility_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_static_init_visibility_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_static_init_visibility_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_static_init_visibility_bridge_lib.c",
        ),
        note="multi-TU static local + file-static visibility bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_param_nested_handoff_matrix",
        source=PROBE_DIR / "runtime/14__probe_vla_param_nested_handoff_matrix.c",
        note="VLA nested parameter handoff matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_stride_rebind_cross_fn_chain",
        source=PROBE_DIR / "runtime/14__probe_vla_stride_rebind_cross_fn_chain.c",
        note="VLA stride rebind across helper-function chain should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_ptrdiff_cross_scope_matrix",
        source=PROBE_DIR / "runtime/14__probe_vla_ptrdiff_cross_scope_matrix.c",
        note="VLA ptrdiff cross-scope matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_intptr_roundtrip_offset_matrix",
        source=PROBE_DIR / "runtime/14__probe_intptr_roundtrip_offset_matrix.c",
        note="intptr_t pointer roundtrip offset matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_uintptr_mask_rebase_matrix",
        source=PROBE_DIR / "runtime/14__probe_uintptr_mask_rebase_matrix.c",
        note="uintptr_t mask/rebase reconstruction matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_ptrdiff_boundary_crosscheck_matrix",
        source=PROBE_DIR / "runtime/14__probe_ptrdiff_boundary_crosscheck_matrix.c",
        note="ptrdiff_t boundary crosscheck matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_variadic_llong_double_bridge",
        source=PROBE_DIR / "runtime/14__probe_variadic_llong_double_bridge.c",
        note="variadic long-long/double width bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_control_do_while_switch_phi_chain",
        source=PROBE_DIR / "runtime/14__probe_control_do_while_switch_phi_chain.c",
        note="do-while + switch + continue/goto phi paths should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_struct_array_double_by_value_roundtrip",
        source=PROBE_DIR / "runtime/14__probe_struct_array_double_by_value_roundtrip.c",
        note="struct with array+double by-value roundtrip should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_dim_recompute_stride_matrix",
        source=PROBE_DIR / "runtime/14__probe_vla_dim_recompute_stride_matrix.c",
        note="VLA dimension recompute and stride matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_pointer_byte_rebase_roundtrip_matrix",
        source=PROBE_DIR / "runtime/14__probe_pointer_byte_rebase_roundtrip_matrix.c",
        note="pointer byte-rebase roundtrip and element ptrdiff scaling should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_float_nan_inf_order_chain",
        source=PROBE_DIR / "runtime/14__probe_float_nan_inf_order_chain.c",
        note="NaN/infinity ordering and comparison chain should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_header_null_ptrdiff_bridge",
        source=PROBE_DIR / "runtime/14__probe_header_null_ptrdiff_bridge.c",
        note="header NULL + ptrdiff bridge semantics should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_struct_union_array_overlay_roundtrip",
        source=PROBE_DIR / "runtime/14__probe_struct_union_array_overlay_roundtrip.c",
        note="struct+union+array by-value roundtrip overlay should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_variadic_long_width_crosscheck",
        source=PROBE_DIR / "runtime/14__probe_variadic_long_width_crosscheck.c",
        note="variadic long/unsigned-long/long-long width crosscheck should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_struct_ternary_by_value_select_chain",
        source=PROBE_DIR / "runtime/14__probe_struct_ternary_by_value_select_chain.c",
        note="struct ternary by-value selection chain should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_header_offsetof_nested_matrix",
        source=PROBE_DIR / "runtime/14__probe_header_offsetof_nested_matrix.c",
        note="nested offsetof header surface should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_switch_sparse_signed_case_ladder",
        source=PROBE_DIR / "runtime/14__probe_switch_sparse_signed_case_ladder.c",
        note="sparse signed switch case ladder should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_bitwise_compound_recurrence_matrix",
        source=PROBE_DIR / "runtime/14__probe_bitwise_compound_recurrence_matrix.c",
        note="bitwise compound recurrence matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_bool_scalar_conversion_matrix",
        source=PROBE_DIR / "runtime/14__probe_bool_scalar_conversion_matrix.c",
        note="scalar-to-bool conversion matrix across int/float/pointer lanes should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_bitfield_signed_promotion_matrix",
        source=PROBE_DIR / "runtime/14__probe_bitfield_signed_promotion_matrix.c",
        note="signed bitfield promotion/update matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_bitfield_compound_assign_bridge",
        source=PROBE_DIR / "runtime/14__probe_bitfield_compound_assign_bridge.c",
        note="unsigned bitfield compound update bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_bitfield_bool_bridge_matrix",
        source=PROBE_DIR / "runtime/14__probe_bitfield_bool_bridge_matrix.c",
        note="bitfield and bool bridge paths should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_fnptr_table_state_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_fnptr_table_state_bridge_main.c",
        note="multi-TU function-pointer table state bridge should match clang runtime behavior",
        inputs=[
            PROBE_DIR / "runtime/14__probe_multitu_fnptr_table_state_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_fnptr_table_state_bridge_lib.c",
        ],
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_variadic_fold_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_variadic_fold_bridge_main.c",
        note="multi-TU mixed-width fold bridge should match clang runtime behavior",
        inputs=[
            PROBE_DIR / "runtime/14__probe_multitu_variadic_fold_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_variadic_fold_bridge_lib.c",
        ],
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_vla_window_reduce_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_vla_window_reduce_bridge_main.c",
        note="multi-TU VLA window reduction bridge should match clang runtime behavior",
        inputs=[
            PROBE_DIR / "runtime/14__probe_multitu_vla_window_reduce_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_vla_window_reduce_bridge_lib.c",
        ],
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_struct_union_route_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_struct_union_route_bridge_main.c",
        note="multi-TU struct/union route bridge should match clang runtime behavior",
        inputs=[
            PROBE_DIR / "runtime/14__probe_multitu_struct_union_route_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_struct_union_route_bridge_lib.c",
        ],
    ),
    RuntimeProbe(
        probe_id="14__probe_control_switch_do_for_state_machine",
        source=PROBE_DIR / "runtime/14__probe_control_switch_do_for_state_machine.c",
        note="control-flow state machine across for/do/switch should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_layout_offset_stride_bridge",
        source=PROBE_DIR / "runtime/14__probe_layout_offset_stride_bridge.c",
        note="layout offsetof/stride bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_pointer_stride_rebase_control_mix",
        source=PROBE_DIR / "runtime/14__probe_pointer_stride_rebase_control_mix.c",
        note="pointer-stride rebasing with mixed control flow should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_bool_bitmask_control_chain",
        source=PROBE_DIR / "runtime/14__probe_bool_bitmask_control_chain.c",
        note="bool-bitmask control chain should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_smoke_expr_eval_driver",
        source=PROBE_DIR / "runtime/14__probe_smoke_expr_eval_driver.c",
        note="deterministic expression-evaluator mini-driver should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_smoke_dispatch_table_driver",
        source=PROBE_DIR / "runtime/14__probe_smoke_dispatch_table_driver.c",
        note="deterministic function-pointer dispatch mini-driver should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_smoke_multitu_config_driver",
        source=PROBE_DIR / "runtime/14__probe_smoke_multitu_config_driver_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_smoke_multitu_config_driver_main.c",
            PROBE_DIR / "runtime/14__probe_smoke_multitu_config_driver_lib.c",
        ),
        note="deterministic multi-TU config mini-driver should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_repeatable_output_hash_matrix",
        source=PROBE_DIR / "runtime/14__probe_repeatable_output_hash_matrix.c",
        note="repeated same-input executions should produce stable runtime output hash matrix",
    ),
    RuntimeProbe(
        probe_id="14__probe_repeatable_diagjson_hash_matrix",
        source=PROBE_DIR / "runtime/14__probe_repeatable_diagjson_hash_matrix.c",
        note="diagnostic-record modeling hash matrix should be stable across repeated builds",
    ),
    RuntimeProbe(
        probe_id="14__probe_repeatable_multitu_link_hash_matrix",
        source=PROBE_DIR / "runtime/14__probe_repeatable_multitu_link_hash_matrix_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_repeatable_multitu_link_hash_matrix_main.c",
            PROBE_DIR / "runtime/14__probe_repeatable_multitu_link_hash_matrix_lib.c",
        ),
        note="multi-TU link-path hash matrix should remain stable across repeated calls",
    ),
    RuntimeProbe(
        probe_id="14__probe_abi_long_double_variadic_regstack_hardening",
        source=PROBE_DIR / "runtime/14__probe_abi_long_double_variadic_regstack_hardening.c",
        note="long-double variadic ABI mix should be repeatable and match clang at reg/stack boundaries",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_static_init_order_depth_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_static_init_order_depth_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_static_init_order_depth_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_static_init_order_depth_bridge_lib.c",
        ),
        note="multi-TU static init/order depth bridge should remain deterministic and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_switch_loop_lite",
        source=PROBE_DIR / "runtime/15__probe_switch_loop_lite.c",
        note="loop+switch lowering should match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_switch_loop_mod5",
        source=PROBE_DIR / "runtime/15__probe_switch_loop_mod5.c",
        note="loop+mod+switch lowering should match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_path_decl_nested_runtime",
        source=PROBE_DIR / "runtime/15__probe_path_decl_nested_runtime.c",
        note="function-pointer runtime call path should not crash",
    ),
    RuntimeProbe(
        probe_id="15__probe_deep_switch_loop_state_machine",
        source=PROBE_DIR / "runtime/15__probe_deep_switch_loop_state_machine.c",
        note="deep loop+switch state machine should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_switch_sparse_case_jump_table",
        source=PROBE_DIR / "runtime/15__probe_switch_sparse_case_jump_table.c",
        note="sparse-case switch control lowering should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_declarator_depth_runtime_chain",
        source=PROBE_DIR / "runtime/15__probe_declarator_depth_runtime_chain.c",
        note="declarator-depth function-pointer runtime chain should match clang behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_many_globals_crossref",
        source=PROBE_DIR / "runtime/15__probe_multitu_many_globals_crossref_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_many_globals_crossref_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_many_globals_crossref_lib.c",
        ),
        note="multi-TU many-globals cross-reference path should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_fnptr_dispatch_grid",
        source=PROBE_DIR / "runtime/15__probe_multitu_fnptr_dispatch_grid_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_fnptr_dispatch_grid_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_fnptr_dispatch_grid_lib.c",
        ),
        note="multi-TU function-pointer dispatch grid should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_deep_recursion_stack_pressure",
        source=PROBE_DIR / "runtime/15__probe_deep_recursion_stack_pressure.c",
        note="deep recursion with per-frame stack payload should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_deep_recursion_stack_pressure_ii",
        source=PROBE_DIR / "runtime/15__probe_deep_recursion_stack_pressure_ii.c",
        note="deeper recursion with larger per-frame payload should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_deep_recursion_stack_pressure_iii",
        source=PROBE_DIR / "runtime/15__probe_deep_recursion_stack_pressure_iii.c",
        note="third-tier recursion pressure lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_large_vla_stride_pressure",
        source=PROBE_DIR / "runtime/15__probe_large_vla_stride_pressure.c",
        note="large local VLA row-stride and pointer-delta stress path should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_many_args_regstack_pressure",
        source=PROBE_DIR / "runtime/15__probe_many_args_regstack_pressure.c",
        note="high-arity mixed scalar+struct call pressure should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_seeded_expr_fuzz_smoke",
        source=PROBE_DIR / "runtime/15__probe_seeded_expr_fuzz_smoke.c",
        note="deterministic seeded expression stress lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_seeded_stmt_fuzz_smoke",
        source=PROBE_DIR / "runtime/15__probe_seeded_stmt_fuzz_smoke.c",
        note="deterministic seeded statement/control stress lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_micro_compile_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_micro_compile_smoke.c",
        note="deterministic corpus-style micro workload should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_control_rebind_dispatch_lattice",
        source=PROBE_DIR / "runtime/15__probe_control_rebind_dispatch_lattice.c",
        note="deep control-flow with function-pointer rebinding lattice should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_large_struct_array_checksum_grid",
        source=PROBE_DIR / "runtime/15__probe_large_struct_array_checksum_grid.c",
        note="large local struct-array checksum grid should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_many_decls_density_pressure_ii",
        source=PROBE_DIR / "runtime/15__probe_many_decls_density_pressure_ii.c",
        note="dense declaration-matrix pressure lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_many_decls_density_pressure_iii",
        source=PROBE_DIR / "runtime/15__probe_many_decls_density_pressure_iii.c",
        note="third-tier dense declaration-matrix pressure lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_large_struct_array_pressure_ii",
        source=PROBE_DIR / "runtime/15__probe_large_struct_array_pressure_ii.c",
        note="larger local struct-array pressure lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_large_struct_array_pressure_iii",
        source=PROBE_DIR / "runtime/15__probe_large_struct_array_pressure_iii.c",
        note="third-tier larger local struct-array pressure lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_const_table_crc",
        source=PROBE_DIR / "runtime/15__probe_multitu_const_table_crc_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_const_table_crc_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_const_table_crc_lib.c",
        ),
        note="multi-TU const-table CRC fold path should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_fnptr_state_matrix",
        source=PROBE_DIR / "runtime/15__probe_multitu_fnptr_state_matrix_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_fnptr_state_matrix_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_fnptr_state_matrix_lib.c",
        ),
        note="multi-TU stateful function-pointer matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_struct_vla_stride_bridge",
        source=PROBE_DIR / "runtime/15__probe_multitu_struct_vla_stride_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_struct_vla_stride_bridge_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_struct_vla_stride_bridge_lib.c",
        ),
        note="multi-TU struct+VLA stride bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_ring_digest_pipeline",
        source=PROBE_DIR / "runtime/15__probe_multitu_ring_digest_pipeline_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_ring_digest_pipeline_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_ring_digest_pipeline_lib.c",
            PROBE_DIR / "runtime/15__probe_multitu_ring_digest_pipeline_aux.c",
        ),
        note="multi-TU ring-digest pipeline with static state should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_large_table_crc_pressure",
        source=PROBE_DIR / "runtime/15__probe_multitu_large_table_crc_pressure_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_large_table_crc_pressure_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_large_table_crc_pressure_lib.c",
        ),
        note="multi-TU large-table CRC pressure lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_recursive_dispatch_pressure",
        source=PROBE_DIR / "runtime/15__probe_multitu_recursive_dispatch_pressure_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_recursive_dispatch_pressure_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_recursive_dispatch_pressure_lib.c",
        ),
        note="multi-TU recursive dispatch pressure lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_layout_stride_pressure",
        source=PROBE_DIR / "runtime/15__probe_multitu_layout_stride_pressure_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_layout_stride_pressure_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_layout_stride_pressure_lib.c",
        ),
        note="multi-TU layout/stride pressure lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_external_compile_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_external_compile_smoke.c",
        note="external-corpus styled deterministic parser/normalizer should compile/run and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_external_parser_fragment_a_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_external_parser_fragment_a_smoke.c",
        note="external-corpus parser fragment A should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_external_parser_fragment_b_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_external_parser_fragment_b_smoke.c",
        note="external-corpus parser fragment B should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_external_parser_fragment_c_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_external_parser_fragment_c_smoke.c",
        note="external-corpus parser fragment C should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_external_parser_fragment_d_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_external_parser_fragment_d_smoke.c",
        note="external-corpus parser fragment D should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_external_decl_chain_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_external_decl_chain_smoke.c",
        note="external-corpus declaration-chain fragment should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_external_typedef_graph_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_external_typedef_graph_smoke.c",
        note="external-corpus typedef-graph fragment should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_pinned_fragment_e_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_pinned_fragment_e_smoke.c",
        note="pinned corpus fragment E should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_pinned_fragment_f_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_pinned_fragment_f_smoke.c",
        note="pinned corpus fragment F should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_pinned_fragment_g_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_pinned_fragment_g_smoke.c",
        note="pinned corpus fragment G should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_ast_pathological_type_graph",
        source=PROBE_DIR / "runtime/15__probe_ast_pathological_type_graph.c",
        note="pathological type-graph compile/runtime lane should remain deterministic and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_ast_pathological_decl_graph_surface",
        source=PROBE_DIR / "runtime/15__probe_ast_pathological_decl_graph_surface.c",
        note="pathological declaration-graph surface lane should remain deterministic and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_smoke",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_smoke.c",
        note="deterministic runtime smoke lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_control_checksum",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_control_checksum.c",
        note="control checksum lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_multitu_state_bridge",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_state_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_state_bridge_main.c",
            PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_state_bridge_lib.c",
        ),
        note="multi-TU state bridge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_abi_args_matrix",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_abi_args_matrix.c",
        note="ABI args matrix lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_loop_state_crc_matrix",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_loop_state_crc_matrix.c",
        note="loop-state CRC matrix lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_struct_array_stride_crc_matrix",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_struct_array_stride_crc_matrix.c",
        note="struct-array stride CRC lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_pointer_mix_checksum_matrix",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_pointer_mix_checksum_matrix.c",
        note="pointer-mix checksum matrix lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_multitu_fnptr_table_bridge",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_fnptr_table_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_fnptr_table_bridge_main.c",
            PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_fnptr_table_bridge_lib.c",
        ),
        note="multi-TU fnptr table bridge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_multitu_const_seed_pipeline",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_const_seed_pipeline_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_const_seed_pipeline_main.c",
            PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_const_seed_pipeline_lib.c",
        ),
        note="multi-TU const-seed pipeline lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_multitu_layout_digest_bridge",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_layout_digest_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_layout_digest_bridge_main.c",
            PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_layout_digest_bridge_lib.c",
        ),
        note="multi-TU layout digest bridge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_policy_shift_char_matrix",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_policy_shift_char_matrix.c",
        note="policy shift/char matrix lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_policy_struct_abi_matrix",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_policy_struct_abi_matrix.c",
        note="policy struct ABI matrix lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_header_control_dispatch_matrix",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_header_control_dispatch_matrix.c",
        note="header control/dispatch matrix lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_header_layout_fold_matrix",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_header_layout_fold_matrix.c",
        note="header layout/fold matrix lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_control_flow_lattice_matrix",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_control_flow_lattice_matrix.c",
        note="control-flow lattice matrix lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_abi_variadic_regstack_matrix",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_abi_variadic_regstack_matrix.c",
        note="ABI variadic reg/stack matrix lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_vla_stride_rebase_matrix",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_vla_stride_rebase_matrix.c",
        note="VLA stride/rebase matrix lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_clang_gcc_tri_diff_struct_union_layout_bridge",
        source=PROBE_DIR / "runtime/15__probe_runtime_clang_gcc_tri_diff_struct_union_layout_bridge.c",
        note="struct/union layout bridge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_policy_signed_shift_impl_defined_tagged",
        source=PROBE_DIR / "runtime/15__probe_policy_signed_shift_impl_defined_tagged.c",
        note="implementation-defined signed right-shift lane should remain deterministic on this host",
    ),
    RuntimeProbe(
        probe_id="15__probe_policy_alias_ub_tagged",
        source=PROBE_DIR / "runtime/15__probe_policy_alias_ub_tagged.c",
        note="restrict-alias UB lane should remain tracked and deterministic in probe mode",
    ),
    RuntimeProbe(
        probe_id="15__probe_policy_impldef_signed_shift_matrix_tagged",
        source=PROBE_DIR / "runtime/15__probe_policy_impldef_signed_shift_matrix_tagged.c",
        note="implementation-defined signed-right-shift matrix should remain deterministic on this host",
    ),
    RuntimeProbe(
        probe_id="15__probe_policy_impldef_signed_char_matrix_tagged",
        source=PROBE_DIR / "runtime/15__probe_policy_impldef_signed_char_matrix_tagged.c",
        note="implementation-defined plain-char signedness matrix should remain deterministic on this host",
    ),
    RuntimeProbe(
        probe_id="15__probe_policy_ub_unsequenced_write_tagged",
        source=PROBE_DIR / "runtime/15__probe_policy_ub_unsequenced_write_tagged.c",
        note="unsequenced-write UB lane should remain tracked and deterministic in probe mode",
    ),
    RuntimeProbe(
        probe_id="15__probe_policy_ub_alias_overlap_tagged",
        source=PROBE_DIR / "runtime/15__probe_policy_ub_alias_overlap_tagged.c",
        note="overlap-alias UB lane should remain tracked and deterministic in probe mode",
    ),
    RuntimeProbe(
        probe_id="15__probe_policy_impldef_signed_shift_width_matrix_tagged",
        source=PROBE_DIR / "runtime/15__probe_policy_impldef_signed_shift_width_matrix_tagged.c",
        note="implementation-defined signed-right-shift width matrix should remain deterministic on this host",
    ),
    RuntimeProbe(
        probe_id="15__probe_policy_impldef_char_promotion_matrix_tagged",
        source=PROBE_DIR / "runtime/15__probe_policy_impldef_char_promotion_matrix_tagged.c",
        note="implementation-defined plain-char promotion matrix should remain deterministic on this host",
    ),
    RuntimeProbe(
        probe_id="15__probe_policy_ub_signed_overflow_chain_tagged",
        source=PROBE_DIR / "runtime/15__probe_policy_ub_signed_overflow_chain_tagged.c",
        note="signed-overflow UB chain should remain tracked and deterministic in probe mode",
    ),
    RuntimeProbe(
        probe_id="15__probe_policy_ub_eval_order_call_sidefx_tagged",
        source=PROBE_DIR / "runtime/15__probe_policy_ub_eval_order_call_sidefx_tagged.c",
        note="call-side-effect evaluation-order UB lane should remain tracked and deterministic in probe mode",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_vla_fnptr_feedback_matrix",
        source=PROBE_DIR / "runtime/15__probe_runtime_vla_fnptr_feedback_matrix.c",
        note="single-TU VLA+function-pointer feedback matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_runtime_struct_union_reducer_chain",
        source=PROBE_DIR / "runtime/15__probe_runtime_struct_union_reducer_chain.c",
        note="struct/union reducer chain should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_state_reseed_pipeline",
        source=PROBE_DIR / "runtime/15__probe_multitu_state_reseed_pipeline_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_state_reseed_pipeline_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_state_reseed_pipeline_lib.c",
            PROBE_DIR / "runtime/15__probe_multitu_state_reseed_pipeline_aux.c",
        ),
        note="multi-TU state reseed pipeline with static ring should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_ceiling_recursion_depth_sweep",
        source=PROBE_DIR / "runtime/15__probe_ceiling_recursion_depth_sweep.c",
        note="stress-ceiling recursion-depth sweep lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_ceiling_decl_density_sweep",
        source=PROBE_DIR / "runtime/15__probe_ceiling_decl_density_sweep.c",
        note="stress-ceiling declaration-density sweep lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_ceiling_aggregate_size_sweep",
        source=PROBE_DIR / "runtime/15__probe_ceiling_aggregate_size_sweep.c",
        note="stress-ceiling aggregate-size sweep lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_ceiling_multitu_layout_pressure_sweep",
        source=PROBE_DIR / "runtime/15__probe_ceiling_multitu_layout_pressure_sweep_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_ceiling_multitu_layout_pressure_sweep_main.c",
            PROBE_DIR / "runtime/15__probe_ceiling_multitu_layout_pressure_sweep_lib.c",
        ),
        note="stress-ceiling multi-TU layout-pressure sweep lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_fuzz_seeded_expr_volume_replay",
        source=PROBE_DIR / "runtime/15__probe_fuzz_seeded_expr_volume_replay.c",
        note="seeded fuzz-volume expression replay lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_fuzz_seeded_stmt_volume_replay",
        source=PROBE_DIR / "runtime/15__probe_fuzz_seeded_stmt_volume_replay.c",
        note="seeded fuzz-volume statement/control replay lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="05__probe_typedef_shadow_parenthesized_expr",
        source=PROBE_DIR / "runtime/05__probe_typedef_shadow_parenthesized_expr.c",
        note="parenthesized identifier should win over cast when typedef is shadowed",
    ),
    RuntimeProbe(
        probe_id="05__probe_precedence_runtime",
        source=PROBE_DIR / "runtime/05__probe_precedence_runtime.c",
        note="operator precedence runtime result should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_unsigned_arith_conv_runtime",
        source=PROBE_DIR / "runtime/05__probe_unsigned_arith_conv_runtime.c",
        note="usual arithmetic conversions for signed/unsigned mix should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_nested_ternary_runtime",
        source=PROBE_DIR / "runtime/05__probe_nested_ternary_runtime.c",
        note="nested ternary associativity/runtime behavior should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_nested_ternary_outer_true_runtime",
        source=PROBE_DIR / "runtime/05__probe_nested_ternary_outer_true_runtime.c",
        note="nested ternary outer-true path should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_nested_ternary_false_chain_runtime",
        source=PROBE_DIR / "runtime/05__probe_nested_ternary_false_chain_runtime.c",
        note="nested ternary false-chain path should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_short_circuit_and_runtime",
        source=PROBE_DIR / "runtime/05__probe_short_circuit_and_runtime.c",
        note="logical AND short-circuit side effects should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_short_circuit_or_runtime",
        source=PROBE_DIR / "runtime/05__probe_short_circuit_or_runtime.c",
        note="logical OR short-circuit side effects should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_comma_eval_runtime",
        source=PROBE_DIR / "runtime/05__probe_comma_eval_runtime.c",
        note="comma operator evaluation/value behavior should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_vla_sizeof_side_effect_runtime",
        source=PROBE_DIR / "runtime/05__probe_vla_sizeof_side_effect_runtime.c",
        note="sizeof(VLA) should evaluate bound side effects and match clang runtime",
    ),
    RuntimeProbe(
        probe_id="05__probe_ternary_side_effect_runtime",
        source=PROBE_DIR / "runtime/05__probe_ternary_side_effect_runtime.c",
        note="conditional operator branch side effects should match clang runtime",
    ),
    RuntimeProbe(
        probe_id="05__probe_nested_ternary_deep_false_chain_runtime",
        source=PROBE_DIR / "runtime/05__probe_nested_ternary_deep_false_chain_runtime.c",
        note="deep nested ternary false-chain path should match clang",
    ),
    RuntimeProbe(
        probe_id="05__probe_compound_literal_array_runtime",
        source=PROBE_DIR / "runtime/05__probe_compound_literal_array_runtime.c",
        note="compound-literal array value/lifetime-in-block behavior should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_control_goto_cleanup_counter_matrix",
        source=PROBE_DIR / "runtime/14__probe_control_goto_cleanup_counter_matrix.c",
        note="goto cleanup edges across nested loops should preserve deterministic control/data flow",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_sizeof_rebind_stride_matrix",
        source=PROBE_DIR / "runtime/14__probe_vla_sizeof_rebind_stride_matrix.c",
        note="VLA rebind + sizeof(row) stride behavior should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_array_ternary_reseed_matrix",
        source=PROBE_DIR / "runtime/14__probe_fnptr_array_ternary_reseed_matrix.c",
        note="function-pointer array + ternary reseed chain should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_ptrdiff_signed_index_rebase_matrix",
        source=PROBE_DIR / "runtime/14__probe_ptrdiff_signed_index_rebase_matrix.c",
        note="signed/unsigned index rebasing and ptrdiff scaling should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_static_local_counter_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_static_local_counter_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_static_local_counter_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_static_local_counter_bridge_lib.c",
        ),
        note="multi-TU static local state should persist across calls and match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_static_storage_slice_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_static_storage_slice_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_static_storage_slice_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_static_storage_slice_bridge_lib.c",
        ),
        note="multi-TU static storage slice updates should remain deterministic and match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_row_pointer_handoff_rebase_matrix",
        source=PROBE_DIR / "runtime/14__probe_vla_row_pointer_handoff_rebase_matrix.c",
        note="VLA row-pointer handoff/rebase and stride math should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_struct_state_reseed_loop_matrix",
        source=PROBE_DIR / "runtime/14__probe_fnptr_struct_state_reseed_loop_matrix.c",
        note="function-pointer struct state reseed loops should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_fnptr_rotation_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_fnptr_rotation_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_fnptr_rotation_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_fnptr_rotation_bridge_lib.c",
        ),
        note="multi-TU function-pointer rotation bridge should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_pair_fold_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_pair_fold_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_pair_fold_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_pair_fold_bridge_lib.c",
        ),
        note="multi-TU struct pair fold/update bridge should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_vla_column_pointer_stride_mix",
        source=PROBE_DIR / "runtime/14__probe_vla_column_pointer_stride_mix.c",
        note="VLA column-pointer stride arithmetic should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_switch_goto_reentry_matrix",
        source=PROBE_DIR / "runtime/14__probe_switch_goto_reentry_matrix.c",
        note="switch+goto reentry control-flow matrix should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_restrict_nonoverlap_accumulate_matrix",
        source=PROBE_DIR / "runtime/14__probe_restrict_nonoverlap_accumulate_matrix.c",
        note="restrict-qualified non-overlap accumulate matrix should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_restrict_vla_window_stride_matrix",
        source=PROBE_DIR / "runtime/14__probe_restrict_vla_window_stride_matrix.c",
        note="restrict-qualified VLA window/stride matrix should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_restrict_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_restrict_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_restrict_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_restrict_bridge_lib.c",
        ),
        note="multi-TU restrict bridge pipeline should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_restrict_alias_overlap_ub_path",
        source=PROBE_DIR / "runtime/14__probe_restrict_alias_overlap_ub_path.c",
        note="restrict overlap UB lane should be tracked deterministically",
    ),
    RuntimeProbe(
        probe_id="14__probe_volatile_restrict_store_order_matrix",
        source=PROBE_DIR / "runtime/14__probe_volatile_restrict_store_order_matrix.c",
        note="volatile+restrict store-order matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_restrict_nonoverlap_rebind_chain",
        source=PROBE_DIR / "runtime/14__probe_restrict_nonoverlap_rebind_chain.c",
        note="restrict non-overlap rebind chain should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_volatile_fnptr_control_chain",
        source=PROBE_DIR / "runtime/14__probe_volatile_fnptr_control_chain.c",
        note="volatile-gated function-pointer control chain should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_long_double_call_chain_matrix",
        source=PROBE_DIR / "runtime/14__probe_long_double_call_chain_matrix.c",
        note="long double call-chain arithmetic should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_long_double_variadic_bridge",
        source=PROBE_DIR / "runtime/14__probe_long_double_variadic_bridge.c",
        note="long double variadic bridge should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_long_double_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_long_double_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_long_double_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_long_double_bridge_lib.c",
        ),
        note="multi-TU long double bridge should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_long_double_struct_pass_return",
        source=PROBE_DIR / "runtime/14__probe_long_double_struct_pass_return.c",
        note="long double struct pass/return ABI path should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_long_double_variadic_struct_bridge",
        source=PROBE_DIR / "runtime/14__probe_long_double_variadic_struct_bridge.c",
        note="long double variadic struct bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_long_double_bridge_matrix",
        source=PROBE_DIR / "runtime/14__probe_complex_long_double_bridge_matrix.c",
        note="complex<long double> bridge matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_long_double_variadic_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_long_double_variadic_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_long_double_variadic_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_long_double_variadic_bridge_lib.c",
        ),
        note="multi-TU long double variadic bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_addsub_eq_matrix",
        source=PROBE_DIR / "runtime/14__probe_complex_addsub_eq_matrix.c",
        note="complex add/sub and equality matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_unary_scalar_promotion_chain",
        source=PROBE_DIR / "runtime/14__probe_complex_unary_scalar_promotion_chain.c",
        note="complex unary and scalar-promotion chain should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_param_return_bridge",
        source=PROBE_DIR / "runtime/14__probe_complex_param_return_bridge.c",
        note="complex function parameter/return bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_complex_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_complex_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_complex_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_complex_bridge_lib.c",
        ),
        note="multi-TU complex call bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_struct_pass_return_bridge",
        source=PROBE_DIR / "runtime/14__probe_complex_struct_pass_return_bridge.c",
        note="complex-field struct pass/return bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_struct_array_pass_return_chain",
        source=PROBE_DIR / "runtime/14__probe_complex_struct_array_pass_return_chain.c",
        note="complex-array struct pass/return chain should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_nested_struct_field_bridge",
        source=PROBE_DIR / "runtime/14__probe_complex_nested_struct_field_bridge.c",
        note="nested struct field path with complex values should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_struct_ternary_select_bridge",
        source=PROBE_DIR / "runtime/14__probe_complex_struct_ternary_select_bridge.c",
        note="ternary selection over structs carrying complex values should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_complex_struct_pass_return",
        source=PROBE_DIR / "runtime/14__probe_multitu_complex_struct_pass_return_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_complex_struct_pass_return_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_complex_struct_pass_return_lib.c",
        ),
        note="multi-TU complex-field struct pass/return should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_complex_nested_route",
        source=PROBE_DIR / "runtime/14__probe_multitu_complex_nested_route_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_complex_nested_route_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_complex_nested_route_lib.c",
        ),
        note="multi-TU nested struct route with complex field should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_struct_pointer_roundtrip_bridge",
        source=PROBE_DIR / "runtime/14__probe_complex_struct_pointer_roundtrip_bridge.c",
        note="pointer roundtrip reads over struct complex fields should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_ptr_field_writeback_direct",
        source=PROBE_DIR / "runtime/14__probe_complex_ptr_field_writeback_direct.c",
        note="direct pointer-member writeback over complex fields should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_ptr_field_compound_writeback",
        source=PROBE_DIR / "runtime/14__probe_complex_ptr_field_compound_writeback.c",
        note="compound pointer-member writeback over complex fields should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_ptr_field_mul_writeback",
        source=PROBE_DIR / "runtime/14__probe_complex_ptr_field_mul_writeback.c",
        note="complex pointer-member '*=' writeback should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_ptr_field_div_writeback",
        source=PROBE_DIR / "runtime/14__probe_complex_ptr_field_div_writeback.c",
        note="complex pointer-member '/=' writeback should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_mul_div_matrix",
        source=PROBE_DIR / "runtime/14__probe_complex_mul_div_matrix.c",
        note="complex multiply/divide matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_scalar_mul_div_chain",
        source=PROBE_DIR / "runtime/14__probe_complex_scalar_mul_div_chain.c",
        note="complex scalar multiply/divide promotion chain should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_struct_copy_assign_chain",
        source=PROBE_DIR / "runtime/14__probe_complex_struct_copy_assign_chain.c",
        note="copy/assign chains over struct complex fields should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_layout_size_align_matrix",
        source=PROBE_DIR / "runtime/14__probe_complex_layout_size_align_matrix.c",
        note="complex size/alignment and field-offset matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_layout_nested_offset_matrix",
        source=PROBE_DIR / "runtime/14__probe_complex_layout_nested_offset_matrix.c",
        note="nested struct offsets around complex fields should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_layout_array_stride_matrix",
        source=PROBE_DIR / "runtime/14__probe_complex_layout_array_stride_matrix.c",
        note="array stride and aggregate value flow for structs with complex fields should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_complex_layout_union_overlay_roundtrip",
        source=PROBE_DIR / "runtime/14__probe_complex_layout_union_overlay_roundtrip.c",
        note="union overlay roundtrip through struct/flat complex views should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="09__probe_do_while_runtime_codegen_crash",
        source=PROBE_DIR / "runtime/09__probe_do_while_runtime_codegen_crash.c",
        note="minimal do-while runtime executable path should compile/run and match clang",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="04__probe_block_extern_initializer_reject",
        source=PROBE_DIR / "diagnostics/04__probe_block_extern_initializer_reject.c",
        note="block-scope extern declaration with initializer should be rejected",
    ),
    DiagnosticProbe(
        probe_id="04__probe_param_extern_reject",
        source=PROBE_DIR / "diagnostics/04__probe_param_extern_reject.c",
        note="parameter declaration should reject extern storage class",
    ),
    DiagnosticProbe(
        probe_id="04__probe_param_static_nonarray_reject",
        source=PROBE_DIR / "diagnostics/04__probe_param_static_nonarray_reject.c",
        note="parameter declaration should reject static when not used in array parameter form",
    ),
    DiagnosticProbe(
        probe_id="04__probe_param_void_named_reject",
        source=PROBE_DIR / "diagnostics/04__probe_param_void_named_reject.c",
        note="named parameter with type void should be rejected",
    ),
    DiagnosticProbe(
        probe_id="04__probe_enum_const_typedef_conflict_reject",
        source=PROBE_DIR / "diagnostics/04__probe_enum_const_typedef_conflict_reject.c",
        note="enumerator should not reuse an existing typedef name in same scope",
    ),
    DiagnosticProbe(
        probe_id="04__probe_enum_const_var_conflict_reject",
        source=PROBE_DIR / "diagnostics/04__probe_enum_const_var_conflict_reject.c",
        note="enumerator should not reuse an existing variable name in same scope",
    ),
    DiagnosticProbe(
        probe_id="04__probe_tag_cross_kind_conflict_reject",
        source=PROBE_DIR / "diagnostics/04__probe_tag_cross_kind_conflict_reject.c",
        note="struct/enum tags sharing one name in same scope should be rejected",
    ),
    DiagnosticProbe(
        probe_id="04__probe_struct_member_missing_type_reject",
        source=PROBE_DIR / "diagnostics/04__probe_struct_member_missing_type_reject.c",
        note="struct member declaration missing a type specifier should be rejected",
    ),
    DiagnosticProbe(
        probe_id="04__probe_bitfield_missing_colon_reject",
        source=PROBE_DIR / "diagnostics/04__probe_bitfield_missing_colon_reject.c",
        note="bitfield declaration missing ':' before width should be rejected",
    ),
    DiagnosticProbe(
        probe_id="04__probe_enum_missing_rbrace_reject",
        source=PROBE_DIR / "diagnostics/04__probe_enum_missing_rbrace_reject.c",
        note="enum body missing closing '}' should be rejected",
    ),
    DiagnosticProbe(
        probe_id="04__probe_typedef_missing_identifier_reject",
        source=PROBE_DIR / "diagnostics/04__probe_typedef_missing_identifier_reject.c",
        note="typedef declaration missing declarator identifier should be rejected",
    ),
    DiagnosticProbe(
        probe_id="04__probe_declarator_unbalanced_parens_reject",
        source=PROBE_DIR / "diagnostics/04__probe_declarator_unbalanced_parens_reject.c",
        note="declarator with unbalanced parentheses should be rejected",
    ),
    DiagnosticProbe(
        probe_id="04__probe_complex_int_reject",
        source=PROBE_DIR / "diagnostics/04__probe_complex_int_reject.c",
        note="_Complex int should be rejected (complex base type must be floating)",
    ),
    DiagnosticProbe(
        probe_id="04__probe_complex_unsigned_reject",
        source=PROBE_DIR / "diagnostics/04__probe_complex_unsigned_reject.c",
        note="unsigned _Complex double should be rejected",
    ),
    DiagnosticProbe(
        probe_id="04__probe_complex_missing_base_reject",
        source=PROBE_DIR / "diagnostics/04__probe_complex_missing_base_reject.c",
        note="_Complex without floating base type should be rejected",
    ),
    DiagnosticProbe(
        probe_id="05__probe_conditional_void_condition_reject",
        source=PROBE_DIR / "diagnostics/05__probe_conditional_void_condition_reject.c",
        note="conditional operator first operand must be scalar (void expression should reject)",
    ),
    DiagnosticProbe(
        probe_id="05__probe_conditional_struct_result_reject",
        source=PROBE_DIR / "diagnostics/05__probe_conditional_struct_result_reject.c",
        note="conditional expression with struct result should be rejected in int return context",
    ),
    DiagnosticProbe(
        probe_id="05__probe_logical_and_struct_reject",
        source=PROBE_DIR / "diagnostics/05__probe_logical_and_struct_reject.c",
        note="logical && operands must be scalar (struct operand should reject)",
    ),
    DiagnosticProbe(
        probe_id="05__probe_unary_not_struct_reject",
        source=PROBE_DIR / "diagnostics/05__probe_unary_not_struct_reject.c",
        note="unary ! operand must be scalar (struct operand should reject)",
    ),
    DiagnosticProbe(
        probe_id="05__probe_address_of_rvalue_reject",
        source=PROBE_DIR / "diagnostics/05__probe_address_of_rvalue_reject.c",
        note="address-of operator should reject non-lvalue operand",
    ),
    DiagnosticProbe(
        probe_id="05__probe_deref_non_pointer_reject",
        source=PROBE_DIR / "diagnostics/05__probe_deref_non_pointer_reject.c",
        note="dereference operator should reject non-pointer operand",
    ),
    DiagnosticProbe(
        probe_id="05__probe_sizeof_incomplete_type_reject",
        source=PROBE_DIR / "diagnostics/05__probe_sizeof_incomplete_type_reject.c",
        note="sizeof should reject incomplete object types",
    ),
    DiagnosticProbe(
        probe_id="05__probe_sizeof_function_reject",
        source=PROBE_DIR / "diagnostics/05__probe_sizeof_function_reject.c",
        note="sizeof should reject function types",
    ),
    DiagnosticProbe(
        probe_id="05__probe_logical_void_operand_reject",
        source=PROBE_DIR / "diagnostics/05__probe_logical_void_operand_reject.c",
        note="logical operators should reject void operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_relational_void_reject",
        source=PROBE_DIR / "diagnostics/05__probe_relational_void_reject.c",
        note="relational operators should reject void operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_ternary_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/05__probe_ternary_struct_condition_reject.c",
        note="conditional operator first operand must be scalar (struct should reject)",
    ),
    DiagnosticProbe(
        probe_id="05__probe_cast_int_to_struct_reject",
        source=PROBE_DIR / "diagnostics/05__probe_cast_int_to_struct_reject.c",
        note="casts between scalar and non-scalar types should reject",
    ),
    DiagnosticProbe(
        probe_id="05__probe_alignof_void_reject",
        source=PROBE_DIR / "diagnostics/05__probe_alignof_void_reject.c",
        note="_Alignof should reject void type operand",
    ),
    DiagnosticProbe(
        probe_id="05__probe_alignof_incomplete_reject",
        source=PROBE_DIR / "diagnostics/05__probe_alignof_incomplete_reject.c",
        note="_Alignof should reject incomplete type operand",
    ),
    DiagnosticProbe(
        probe_id="05__probe_shift_width_large_reject",
        source=PROBE_DIR / "diagnostics/05__probe_shift_width_large_reject.c",
        note="left/right shift should reject widths >= bit width of promoted lhs",
    ),
    DiagnosticProbe(
        probe_id="05__probe_bitwise_float_reject",
        source=PROBE_DIR / "diagnostics/05__probe_bitwise_float_reject.c",
        note="bitwise operators should reject floating operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_relational_struct_reject",
        source=PROBE_DIR / "diagnostics/05__probe_relational_struct_reject.c",
        note="relational operators should reject struct operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_equality_struct_reject",
        source=PROBE_DIR / "diagnostics/05__probe_equality_struct_reject.c",
        note="equality operators should reject struct operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_add_void_reject",
        source=PROBE_DIR / "diagnostics/05__probe_add_void_reject.c",
        note="arithmetic operators should reject void operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_unary_bitnot_float_reject",
        source=PROBE_DIR / "diagnostics/05__probe_unary_bitnot_float_reject.c",
        note="unary bitwise-not should reject floating operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_unary_plus_struct_reject",
        source=PROBE_DIR / "diagnostics/05__probe_unary_plus_struct_reject.c",
        note="unary plus should reject struct operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_unary_minus_pointer_reject",
        source=PROBE_DIR / "diagnostics/05__probe_unary_minus_pointer_reject.c",
        note="unary minus should reject pointer operands",
    ),
    DiagnosticProbe(
        probe_id="05__probe_sizeof_void_reject",
        source=PROBE_DIR / "diagnostics/05__probe_sizeof_void_reject.c",
        note="sizeof should reject void type operand",
    ),
    DiagnosticProbe(
        probe_id="05__probe_alignof_expr_reject",
        source=PROBE_DIR / "diagnostics/05__probe_alignof_expr_reject.c",
        note="_Alignof should reject expression operand in strict C mode",
    ),
    DiagnosticProbe(
        probe_id="07__probe_assign_struct_to_int_reject",
        source=PROBE_DIR / "diagnostics/07__probe_assign_struct_to_int_reject.c",
        note="assignment from struct value to int object should be rejected",
    ),
    DiagnosticProbe(
        probe_id="07__probe_agg_invalid_member_reject",
        source=PROBE_DIR / "diagnostics/07__probe_agg_invalid_member_reject.c",
        note="member access on a non-existent struct field should be rejected",
    ),
    DiagnosticProbe(
        probe_id="07__probe_constexpr_array_size_reject",
        source=PROBE_DIR / "diagnostics/07__probe_constexpr_array_size_reject.c",
        note="file-scope array size must be an integer constant expression",
        required_substrings=("constant",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_constexpr_case_label_reject",
        source=PROBE_DIR / "diagnostics/07__probe_constexpr_case_label_reject.c",
        note="case labels must be integer constant expressions",
        required_substrings=("Case label is not an integer constant expression",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_constexpr_static_init_reject",
        source=PROBE_DIR / "diagnostics/07__probe_constexpr_static_init_reject.c",
        note="static storage initializers must be constant expressions",
        required_substrings=("not a constant expression",),
    ),
    DiagnosticProbe(
        probe_id="08__probe_designator_unknown_field_reject",
        source=PROBE_DIR / "diagnostics/08__probe_designator_unknown_field_reject.c",
        note="designated initializer should reject unknown field names",
    ),
    DiagnosticProbe(
        probe_id="09__probe_goto_into_vla_scope_reject",
        source=PROBE_DIR / "diagnostics/09__probe_goto_into_vla_scope_reject.c",
        note="goto into VLA scope should be rejected",
    ),
    DiagnosticProbe(
        probe_id="09__probe_switch_float_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_switch_float_condition_reject.c",
        note="switch controlling expression must be integer type (float should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_switch_pointer_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_switch_pointer_condition_reject.c",
        note="switch controlling expression must be integer type (pointer should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_switch_string_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_switch_string_condition_reject.c",
        note="switch controlling expression must be integer type (string should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_switch_double_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_switch_double_condition_reject.c",
        note="switch controlling expression must be integer type (double should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_if_void_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_if_void_condition_reject.c",
        note="if controlling expression must be scalar (void expression should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_if_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_if_struct_condition_reject.c",
        note="if controlling expression must be scalar (struct expression should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_while_void_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_while_void_condition_reject.c",
        note="while controlling expression must be scalar (void expression should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_do_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_do_struct_condition_reject.c",
        note="do-while controlling expression must be scalar (struct expression should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_for_void_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_for_void_condition_reject.c",
        note="for controlling expression must be scalar (void expression should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_for_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/09__probe_for_struct_condition_reject.c",
        note="for controlling expression must be scalar (struct expression should reject)",
    ),
    DiagnosticProbe(
        probe_id="09__probe_if_missing_then_stmt_reject",
        source=PROBE_DIR / "diagnostics/09__probe_if_missing_then_stmt_reject.c",
        note="if statement missing required controlled statement should be rejected",
    ),
    DiagnosticProbe(
        probe_id="09__probe_else_missing_stmt_reject",
        source=PROBE_DIR / "diagnostics/09__probe_else_missing_stmt_reject.c",
        note="else arm missing required statement should be rejected",
    ),
    DiagnosticProbe(
        probe_id="09__probe_switch_missing_rparen_reject",
        source=PROBE_DIR / "diagnostics/09__probe_switch_missing_rparen_reject.c",
        note="switch controlling expression missing ')' should be rejected",
    ),
    DiagnosticProbe(
        probe_id="09__probe_for_missing_first_semicolon_reject",
        source=PROBE_DIR / "diagnostics/09__probe_for_missing_first_semicolon_reject.c",
        note="for header missing first ';' separator should be rejected",
    ),
    DiagnosticProbe(
        probe_id="09__probe_goto_undefined_label_nested_reject",
        source=PROBE_DIR / "diagnostics/09__probe_goto_undefined_label_nested_reject.c",
        note="goto to undefined label in nested control flow should be rejected",
    ),
    DiagnosticProbe(
        probe_id="10__probe_block_extern_different_type_reject",
        source=PROBE_DIR / "diagnostics/10__probe_block_extern_different_type_reject.c",
        note="block-scope extern should reject redeclaration with conflicting type",
    ),
    DiagnosticProbe(
        probe_id="11__probe_duplicate_param_name_reject",
        source=PROBE_DIR / "diagnostics/11__probe_duplicate_param_name_reject.c",
        note="duplicate parameter names in one function declarator should be rejected",
    ),
    DiagnosticProbe(
        probe_id="11__probe_param_auto_reject",
        source=PROBE_DIR / "diagnostics/11__probe_param_auto_reject.c",
        note="function parameter should reject auto storage class",
    ),
    DiagnosticProbe(
        probe_id="11__probe_param_void_plus_other_reject",
        source=PROBE_DIR / "diagnostics/11__probe_param_void_plus_other_reject.c",
        note="parameter list must reject 'void' combined with additional parameters",
    ),
    DiagnosticProbe(
        probe_id="11__probe_param_incomplete_type_reject",
        source=PROBE_DIR / "diagnostics/11__probe_param_incomplete_type_reject.c",
        note="function parameter declared with incomplete struct type should be rejected",
    ),
    DiagnosticProbe(
        probe_id="11__probe_return_void_expr_in_int_reject",
        source=PROBE_DIR / "diagnostics/11__probe_return_void_expr_in_int_reject.c",
        note="int-returning function should reject returning void expression",
    ),
    DiagnosticProbe(
        probe_id="12__probe_invalid_shift_width",
        source=PROBE_DIR / "diagnostics/12__probe_invalid_shift_width.c",
        note="should emit diagnostic for negative shift width",
        required_substrings=("Invalid shift width",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_incompatible_ptr_assign",
        source=PROBE_DIR / "diagnostics/12__probe_incompatible_ptr_assign.c",
        note="should emit assignment/pointer incompatibility diagnostic",
        required_substrings=("Incompatible assignment operands",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_illegal_struct_to_int_cast",
        source=PROBE_DIR / "diagnostics/12__probe_illegal_struct_to_int_cast.c",
        note="should emit invalid cast diagnostic",
        required_substrings=("Invalid cast between non-scalar types",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_while_missing_lparen_reject",
        source=PROBE_DIR / "diagnostics/12__probe_while_missing_lparen_reject.c",
        note="while statement missing '(' should emit a diagnostic (fail-closed parser recovery)",
    ),
    DiagnosticProbe(
        probe_id="12__probe_do_while_missing_semicolon_reject",
        source=PROBE_DIR / "diagnostics/12__probe_do_while_missing_semicolon_reject.c",
        note="do-while statement missing trailing ';' should emit a diagnostic",
    ),
    DiagnosticProbe(
        probe_id="12__probe_for_header_missing_semicolon_reject",
        source=PROBE_DIR / "diagnostics/12__probe_for_header_missing_semicolon_reject.c",
        note="for header missing first ';' should emit a diagnostic",
        required_substrings=("Undeclared identifier",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_fnptr_too_many_args_reject",
        source=PROBE_DIR / "diagnostics/13__probe_fnptr_too_many_args_reject.c",
        note="function-pointer call should reject too many arguments for fixed-arity target",
        required_substrings=("Too many arguments",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_fnptr_too_few_args_reject",
        source=PROBE_DIR / "diagnostics/13__probe_fnptr_too_few_args_reject.c",
        note="function-pointer call should reject too few arguments for fixed-arity target",
        required_substrings=("Too few arguments",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_mod_float_reject",
        source=PROBE_DIR / "diagnostics/13__probe_mod_float_reject.c",
        note="modulo with floating operand should be rejected",
        required_substrings=("Operator '%'",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_fnptr_assign_incompatible_reject",
        source=PROBE_DIR / "diagnostics/13__probe_fnptr_assign_incompatible_reject.c",
        note="function-pointer assignment with incompatible signature should be rejected",
        required_substrings=("Incompatible assignment operands",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_fnptr_nested_qualifier_loss_reject",
        source=PROBE_DIR / "diagnostics/13__probe_fnptr_nested_qualifier_loss_reject.c",
        note="nested function-pointer assignment should reject qualifier loss at pointee depth",
        required_substrings=("discards qualifiers",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_voidptr_to_fnptr_assign_reject",
        source=PROBE_DIR / "diagnostics/13__probe_voidptr_to_fnptr_assign_reject.c",
        note="assignment from void* to function pointer should be rejected",
        required_substrings=("Incompatible assignment operands",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_fnptr_to_voidptr_assign_reject",
        source=PROBE_DIR / "diagnostics/13__probe_fnptr_to_voidptr_assign_reject.c",
        note="assignment from function pointer to void* should be rejected",
        required_substrings=("Incompatible assignment operands",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_fnptr_nested_volatile_qualifier_loss_reject",
        source=PROBE_DIR / "diagnostics/13__probe_fnptr_nested_volatile_qualifier_loss_reject.c",
        note="nested function-pointer assignment should reject volatile qualifier loss at pointee depth",
        required_substrings=("discards qualifiers",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_fnptr_deep_const_qualifier_loss_reject",
        source=PROBE_DIR / "diagnostics/13__probe_fnptr_deep_const_qualifier_loss_reject.c",
        note="multi-level function-pointer assignment should reject deep const qualifier loss",
        required_substrings=("discards qualifiers",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_fnptr_table_incompatible_assign_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_fnptr_table_incompatible_assign_reject.c",
        note="runtime fnptr table lane should reject too-few-arguments call through function pointer",
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_fnptr_table_too_many_args_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_fnptr_table_too_many_args_reject.c",
        note="runtime fnptr table lane should reject too-many-arguments call through function pointer",
        required_substrings=("Too many arguments",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_fnptr_table_incompatible_signature_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_fnptr_table_incompatible_signature_reject.c",
        note="runtime fnptr table lane should reject incompatible function-pointer initializer signatures",
        required_substrings=("Incompatible initializer type",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_fnptr_struct_arg_incompatible_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_fnptr_struct_arg_incompatible_reject.c",
        note="runtime fnptr lane should reject struct argument passed where function pointer is required",
        required_substrings=("has incompatible type",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_fnptr_param_noncallable_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_fnptr_param_noncallable_reject.c",
        note="runtime fnptr lane should reject non-callable argument passed as function-pointer parameter",
        required_substrings=("has incompatible type",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_offsetof_bitfield_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_offsetof_bitfield_reject.c",
        note="layout lane should reject offsetof on bitfield member",
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_vla_non_integer_bound_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_vla_non_integer_bound_reject.c",
        note="layout lane should reject file-scope VLA with non-constant bound",
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_ternary_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_ternary_struct_condition_reject.c",
        note="control lane should reject ternary struct condition operand",
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_switch_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_switch_struct_condition_reject.c",
        note="control lane should reject switch with non-integer struct condition",
        required_substrings=("switch controlling expression must be integer",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_if_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_if_struct_condition_reject.c",
        note="control lane should reject if with non-scalar struct condition",
        required_substrings=("if controlling expression must be scalar",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_while_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_while_struct_condition_reject.c",
        note="control lane should reject while with non-scalar struct condition",
        required_substrings=("while controlling expression must be scalar",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_return_struct_to_int_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_return_struct_to_int_reject.c",
        note="semantic lane should reject returning struct from int function",
        required_substrings=("Incompatible return type",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_continue_outside_loop_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_continue_outside_loop_reject.c",
        note="control lane should reject continue outside iterative statements",
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_break_outside_loop_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_break_outside_loop_reject.c",
        note="control lane should reject break outside loop/switch statements",
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_case_outside_switch_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_case_outside_switch_reject.c",
        note="control lane should reject case labels outside switch statements",
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_default_outside_switch_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_default_outside_switch_reject.c",
        note="control lane should reject default labels outside switch statements",
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_switch_duplicate_default_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_switch_duplicate_default_reject.c",
        note="control lane should reject duplicate default labels in switch statements",
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_switch_duplicate_case_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_switch_duplicate_case_reject.c",
        note="control lane should reject duplicate case labels in switch statements",
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_switch_nonconst_case_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_switch_nonconst_case_reject.c",
        note="control lane should reject non-constant case labels",
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_continue_in_switch_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_continue_in_switch_reject.c",
        note="control lane should reject continue inside switch without an enclosing loop",
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_complex_lt_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_complex_lt_reject.c",
        note="complex relational '<' should be rejected with a semantic diagnostic",
        required_substrings=("real (non-complex) comparable operands",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_complex_le_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_complex_le_reject.c",
        note="complex relational '<=' should be rejected with a semantic diagnostic",
        required_substrings=("real (non-complex) comparable operands",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_complex_gt_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_complex_gt_reject.c",
        note="complex relational '>' should be rejected with a semantic diagnostic",
        required_substrings=("real (non-complex) comparable operands",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_complex_ge_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_complex_ge_reject.c",
        note="complex relational '>=' should be rejected with a semantic diagnostic",
        required_substrings=("real (non-complex) comparable operands",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_multitu_duplicate_external_definition_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_external_definition_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_external_definition_reject_main.c",
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_external_definition_reject_lib.c",
        ),
        note="multi-TU lane should reject duplicate external definitions at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_multitu_extern_type_mismatch_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_multitu_extern_type_mismatch_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_extern_type_mismatch_reject_main.c",
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_extern_type_mismatch_reject_lib.c",
        ),
        note="multi-TU lane should reject conflicting extern types that collide at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_multitu_duplicate_tentative_type_conflict_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_tentative_type_conflict_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_tentative_type_conflict_reject_main.c",
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_tentative_type_conflict_reject_lib.c",
        ),
        note="multi-TU lane should reject conflicting external definitions with mismatched types",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_multitu_duplicate_function_definition_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_function_definition_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_function_definition_reject_main.c",
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_function_definition_reject_lib.c",
        ),
        note="multi-TU lane should reject duplicate function definitions at link stage (text diagnostics only)",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_unterminated_string_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_unterminated_string_no_crash.c",
        note="malformed unterminated-string input should fail closed without crashing",
        required_substrings=("Unterminated string literal",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_bad_escape_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_bad_escape_no_crash.c",
        note="malformed bad-escape input should fail closed without crashing",
        required_substrings=("Invalid escape in string literal",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_pp_directive_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_directive_no_crash.c",
        note="malformed preprocessor directive should fail closed without crashing",
        required_substrings=("invalid parameter list in #define",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_pp_unterminated_if_chain_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_unterminated_if_chain_no_crash.c",
        note="malformed unterminated #if/#elif chain should fail closed without crashing",
        required_substrings=("unclosed #if/#ifdef/#ifndef",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_pp_macro_paste_fragments_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_macro_paste_fragments_no_crash.c",
        note="malformed macro-paste fragments should fail closed without crashing",
        required_substrings=("GNU ', ##__VA_ARGS__' extension is not supported",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash.c",
        note="malformed nested #if/#elif chain should fail closed without crashing",
        required_substrings=("unclosed #if/#ifdef/#ifndef",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_pp_include_cycle_guarded_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_include_cycle_guarded_no_crash.c",
        note="recursive self-include should fail closed with deterministic preprocessing diagnostic",
        required_substrings=("detected recursive include of",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_multifile_header_tail_garbage_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_multifile_header_tail_garbage_no_crash.c",
        note="malformed multi-file include with header-tail garbage should fail closed without crashing",
        required_substrings=("Invalid character in source",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_multifile_macro_arity_mismatch_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_multifile_macro_arity_mismatch_no_crash.c",
        note="malformed multi-file macro arity mismatch should fail closed without crashing",
        required_substrings=("requires 2 argument(s), got 1",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_header_guard_tail_mismatch_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_header_guard_tail_mismatch_no_crash.c",
        note="malformed header include-guard mismatch recursion should fail closed without crashing",
        required_substrings=("detected recursive include of",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_header_macro_cycle_chain_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_header_macro_cycle_chain_no_crash.c",
        note="malformed header cycle + macro-arity chain should fail closed without crashing",
        required_substrings=("requires 2 argument(s), got 1",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_header_guard_tail_mismatch_chain_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_header_guard_tail_mismatch_chain_no_crash.c",
        note="malformed header guard-tail mismatch chain recursion should fail closed without crashing",
        required_substrings=("detected recursive include of",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_include_graph_guard_collision_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_include_graph_guard_collision_no_crash.c",
        note="malformed include-graph guard collision should fail closed with deterministic diagnostics",
        required_substrings=("Undeclared identifier",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_multitu_duplicate_symbol_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_multitu_duplicate_symbol_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/15__probe_diag_multitu_duplicate_symbol_reject_main.c",
            PROBE_DIR / "diagnostics/15__probe_diag_multitu_duplicate_symbol_reject_lib.c",
        ),
        note="multi-TU duplicate external symbol definitions should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_corpus_external_compile_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_corpus_external_compile_reject.c",
        note="external-corpus fragment with malformed typedef tail should fail closed with parser diagnostic",
        required_substrings=("Expected identifier in declarator",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_corpus_external_macro_guard_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_corpus_external_macro_guard_reject.c",
        note="external-corpus malformed macro-guard fragment should fail closed in preprocessing",
        required_substrings=("invalid parameter list in #define",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_corpus_external_macro_chain_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_corpus_external_macro_chain_reject.c",
        note="external-corpus malformed macro-chain fragment should fail closed in preprocessing",
        required_substrings=("invalid parameter list in #define",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_corpus_external_include_guard_mismatch_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_corpus_external_include_guard_mismatch_reject.c",
        note="external-corpus include-guard mismatch should fail closed via recursive-include detection",
        required_substrings=("detected recursive include of",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_corpus_pinned_macro_include_chain_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_corpus_pinned_macro_include_chain_reject.c",
        note="pinned corpus macro include-chain fragment should fail closed in preprocessing",
        required_substrings=("invalid parameter list in #define",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_corpus_pinned_typedef_decl_cycle_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_corpus_pinned_typedef_decl_cycle_reject.c",
        note="pinned corpus typedef-decl cycle fragment should fail closed with parser diagnostic",
        required_substrings=("Expected identifier in declarator",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_pathological_initializer_shape_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_pathological_initializer_shape_reject.c",
        note="pathological designated-initializer shape should fail closed with deterministic diagnostics",
        required_substrings=("Expected expression after '=' in struct initializer",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_pathological_initializer_rewrite_surface_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_pathological_initializer_rewrite_surface_reject.c",
        note="pathological initializer rewrite surface lane should fail closed with deterministic diagnostics",
        required_substrings=("Expected expression after '=' in struct initializer",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_pathological_switch_case_surface_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_pathological_switch_case_surface_reject.c",
        note="pathological switch case surface lane should reject non-constant case labels",
        required_substrings=("Case label is not an integer constant expression",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_token_stream_seeded_a_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_token_stream_seeded_a_no_crash.c",
        note="seeded malformed token-stream A should fail closed without crashing",
        required_substrings=("Unexpected token at start of expression",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_token_stream_seeded_b_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_token_stream_seeded_b_no_crash.c",
        note="seeded malformed token-stream B should fail closed without crashing",
        required_substrings=("Unexpected token at start of expression",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_token_stream_seeded_c_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_token_stream_seeded_c_no_crash.c",
        note="seeded malformed token-stream C should fail closed without crashing",
        required_substrings=("Unexpected token at start of expression",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_fuzz_seeded_malformed_volume_replay_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_fuzz_seeded_malformed_volume_replay_no_crash.c",
        note="seeded malformed fuzz-volume replay lane should fail closed without crashing",
        required_substrings=("Unexpected token at start of expression",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_multitu_duplicate_function_definition_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_multitu_duplicate_function_definition_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/15__probe_diag_multitu_duplicate_function_definition_reject_main.c",
            PROBE_DIR / "diagnostics/15__probe_diag_multitu_duplicate_function_definition_reject_lib.c",
        ),
        note="multi-TU duplicate function definitions should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_multitu_symbol_type_conflict_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_multitu_symbol_type_conflict_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/15__probe_diag_multitu_symbol_type_conflict_reject_main.c",
            PROBE_DIR / "diagnostics/15__probe_diag_multitu_symbol_type_conflict_reject_lib.c",
        ),
        note="multi-TU duplicate global symbols with type conflict should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_invalid_dollar_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_invalid_dollar_no_crash.c",
        note="malformed invalid-dollar token stream should fail closed without crashing",
        required_substrings=("Invalid character in source",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_char_invalid_hex_escape_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_char_invalid_hex_escape_no_crash.c",
        note="malformed invalid hex escape in character literal should fail closed without crashing",
        required_substrings=("Invalid \\x escape in character literal",),
    ),
]


DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_while_missing_lparen",
        source=PROBE_DIR / "diagnostics/12__probe_while_missing_lparen_reject.c",
        note="diagnostics JSON export should include parser diagnostics for missing '(' after while",
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_do_while_missing_semicolon",
        source=PROBE_DIR / "diagnostics/12__probe_do_while_missing_semicolon_reject.c",
        note="diagnostics JSON export should include parser diagnostics for missing ';' after do-while",
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_for_header_missing_semicolon",
        source=PROBE_DIR / "diagnostics/12__probe_for_header_missing_semicolon_reject.c",
        note="diagnostics JSON export should include diagnostics for malformed for header",
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_complex_lt_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_complex_lt_reject.c",
        note="diagnostics JSON should include semantic relational-complex rejection for '<'",
        expected_codes=(2000,),
        expected_line=4,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_complex_le_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_complex_le_reject.c",
        note="diagnostics JSON should include semantic relational-complex rejection for '<='",
        expected_codes=(2000,),
        expected_line=4,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_complex_gt_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_complex_gt_reject.c",
        note="diagnostics JSON should include semantic relational-complex rejection for '>'",
        expected_codes=(2000,),
        expected_line=4,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_complex_ge_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_complex_ge_reject.c",
        note="diagnostics JSON should include semantic relational-complex rejection for '>='",
        expected_codes=(2000,),
        expected_line=4,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_continue_outside_loop_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_continue_outside_loop_reject.c",
        note="diagnostics JSON should include control-flow rejection for continue outside loop",
        expected_codes=(2000,),
        expected_line=3,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_break_outside_loop_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_break_outside_loop_reject.c",
        note="diagnostics JSON should include control-flow rejection for break outside loop/switch",
        expected_codes=(2000,),
        expected_line=3,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_case_outside_switch_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_case_outside_switch_reject.c",
        note="diagnostics JSON should include control-flow rejection for case outside switch",
        expected_codes=(1000,),
        expected_line=2,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_default_outside_switch_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_default_outside_switch_reject.c",
        note="diagnostics JSON should include control-flow rejection for default outside switch",
        expected_codes=(1000,),
        expected_line=2,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_switch_duplicate_default_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_switch_duplicate_default_reject.c",
        note="diagnostics JSON should include duplicate default-label rejection",
        expected_codes=(2000,),
        expected_line=9,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_switch_duplicate_case_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_switch_duplicate_case_reject.c",
        note="diagnostics JSON should include duplicate case-label rejection",
        expected_codes=(2000,),
        expected_line=8,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_switch_nonconst_case_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_switch_nonconst_case_reject.c",
        note="diagnostics JSON should include non-constant case-label rejection",
        expected_codes=(2000,),
        expected_line=6,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_continue_in_switch_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_continue_in_switch_reject.c",
        note="diagnostics JSON should include continue-in-switch rejection diagnostic",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_fnptr_table_too_many_args_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_fnptr_table_too_many_args_reject.c",
        note="diagnostics JSON should include fnptr too-many-args rejection",
        expected_codes=(2000,),
        expected_line=7,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_fnptr_table_incompatible_signature_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_fnptr_table_incompatible_signature_reject.c",
        note="diagnostics JSON should include fnptr incompatible-initializer-signature rejection",
        expected_codes=(2000,),
        expected_line=4,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_fnptr_struct_arg_incompatible_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_fnptr_struct_arg_incompatible_reject.c",
        note="diagnostics JSON should include fnptr incompatible-struct-argument rejection",
        expected_codes=(2000,),
        expected_line=12,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_fnptr_param_noncallable_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_fnptr_param_noncallable_reject.c",
        note="diagnostics JSON should include fnptr non-callable argument rejection",
        expected_codes=(2000,),
        expected_line=7,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_file_scope_vla_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_vla_non_integer_bound_reject.c",
        note="diagnostics JSON should include file-scope VLA static-duration rejection",
        expected_codes=(2000,),
        expected_line=2,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_offsetof_bitfield_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_offsetof_bitfield_reject.c",
        note="diagnostics JSON should include offsetof bitfield rejection",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_ternary_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_ternary_struct_condition_reject.c",
        note="diagnostics JSON should include ternary struct-condition rejection",
        expected_codes=(2000,),
        expected_line=7,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_fnptr_table_too_few_args_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_fnptr_table_incompatible_assign_reject.c",
        note="diagnostics JSON should include fnptr too-few-args rejection",
        expected_codes=(2000,),
        expected_line=7,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_switch_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_switch_struct_condition_reject.c",
        note="diagnostics JSON should include switch non-integer condition rejection",
        expected_codes=(2000,),
        expected_line=7,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_if_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_if_struct_condition_reject.c",
        note="diagnostics JSON should include if non-scalar condition rejection",
        expected_codes=(2000,),
        expected_line=7,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_while_struct_condition_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_while_struct_condition_reject.c",
        note="diagnostics JSON should include while non-scalar condition rejection",
        expected_codes=(2000,),
        expected_line=7,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_return_struct_to_int_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_return_struct_to_int_reject.c",
        note="diagnostics JSON should include return-type mismatch rejection",
        expected_codes=(2000,),
        expected_line=7,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_multitu_duplicate_external_definition_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_external_definition_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_external_definition_reject_main.c",
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_external_definition_reject_lib.c",
        ),
        note="diagnostics JSON should be exported for multi-TU duplicate external definition link failures",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_multitu_extern_type_mismatch_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_multitu_extern_type_mismatch_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_extern_type_mismatch_reject_main.c",
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_extern_type_mismatch_reject_lib.c",
        ),
        note="diagnostics JSON should be exported for multi-TU extern-type mismatch link failures",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_multitu_duplicate_tentative_type_conflict_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_tentative_type_conflict_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_tentative_type_conflict_reject_main.c",
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_tentative_type_conflict_reject_lib.c",
        ),
        note="diagnostics JSON should be exported for multi-TU tentative-definition type conflict link failures",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_multitu_duplicate_function_definition_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_function_definition_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_function_definition_reject_main.c",
            PROBE_DIR / "diagnostics/14__probe_diag_multitu_duplicate_function_definition_reject_lib.c",
        ),
        note="diagnostics JSON should be exported for multi-TU duplicate function definition link failures",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_unbalanced_block_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diagjson_malformed_unbalanced_block_no_crash.c",
        note="diagnostics JSON should be emitted for malformed unbalanced-block torture lane",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_unclosed_comment_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diagjson_malformed_unclosed_comment_no_crash.c",
        note="diagnostics JSON should be emitted for malformed unclosed-comment torture lane",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_multitu_duplicate_symbol_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_multitu_duplicate_symbol_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/15__probe_diag_multitu_duplicate_symbol_reject_main.c",
            PROBE_DIR / "diagnostics/15__probe_diag_multitu_duplicate_symbol_reject_lib.c",
        ),
        note="diagnostics JSON should be exported for multi-TU duplicate-symbol link failures",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_pathological_initializer_shape_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_pathological_initializer_shape_reject.c",
        note="diagnostics JSON should be exported for pathological designated-initializer rejection",
        expected_codes=(2000,),
        expected_line=15,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_corpus_pinned_macro_include_chain_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_corpus_pinned_macro_include_chain_reject.c",
        note="diagnostics JSON should be exported for pinned-corpus macro include-chain rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_corpus_pinned_typedef_decl_cycle_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_corpus_pinned_typedef_decl_cycle_reject.c",
        note="diagnostics JSON should be exported for pinned-corpus typedef-decl-cycle rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_pathological_initializer_rewrite_surface_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_pathological_initializer_rewrite_surface_reject.c",
        note="diagnostics JSON should be exported for pathological initializer-rewrite surface rejection",
        expected_codes=(2000,),
        expected_line=22,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_pathological_switch_case_surface_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_pathological_switch_case_surface_reject.c",
        note="diagnostics JSON should be exported for pathological non-constant switch-case surface rejection",
        expected_codes=(2000,),
        expected_line=5,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_token_stream_seeded_a_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_token_stream_seeded_a_no_crash.c",
        note="diagnostics JSON should be exported for seeded malformed token-stream A",
        expected_codes=(1000,),
        expected_line=3,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_token_stream_seeded_b_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_token_stream_seeded_b_no_crash.c",
        note="diagnostics JSON should be exported for seeded malformed token-stream B",
        expected_codes=(1000,),
        expected_line=3,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_token_stream_seeded_c_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_token_stream_seeded_c_no_crash.c",
        note="diagnostics JSON should be exported for seeded malformed token-stream C",
        expected_codes=(1000,),
        expected_line=3,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_fuzz_seeded_malformed_volume_replay_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_fuzz_seeded_malformed_volume_replay_no_crash.c",
        note="diagnostics JSON should be exported for seeded malformed fuzz-volume replay lane",
        expected_codes=(1000,),
        expected_line=5,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_multitu_duplicate_function_definition_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_multitu_duplicate_function_definition_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/15__probe_diag_multitu_duplicate_function_definition_reject_main.c",
            PROBE_DIR / "diagnostics/15__probe_diag_multitu_duplicate_function_definition_reject_lib.c",
        ),
        note="diagnostics JSON should be exported for multi-TU duplicate function definition link failures",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_multitu_symbol_type_conflict_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_multitu_symbol_type_conflict_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/15__probe_diag_multitu_symbol_type_conflict_reject_main.c",
            PROBE_DIR / "diagnostics/15__probe_diag_multitu_symbol_type_conflict_reject_lib.c",
        ),
        note="diagnostics JSON should be exported for multi-TU duplicate symbol type-conflict link failures",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_invalid_dollar_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_invalid_dollar_no_crash.c",
        note="diagnostics JSON should be exported for malformed invalid-dollar lane",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_char_invalid_hex_escape_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_char_invalid_hex_escape_no_crash.c",
        note="diagnostics JSON should be exported for malformed invalid-hex-escape char lane",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_pp_unterminated_if_chain_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_unterminated_if_chain_no_crash.c",
        note="diagnostics JSON should be exported for malformed unterminated #if/#elif chain lane",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_pp_macro_paste_fragments_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_macro_paste_fragments_no_crash.c",
        note="diagnostics JSON should be exported for malformed macro-paste fragment lane",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_pp_nested_ifdef_chain_seeded_d_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash.c",
        note="diagnostics JSON should be exported for malformed nested #if/#elif chain lane",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_pp_include_cycle_guarded_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_include_cycle_guarded_no_crash.c",
        note="diagnostics JSON should be exported for recursive self-include malformed lane",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_multifile_header_tail_garbage_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_multifile_header_tail_garbage_no_crash.c",
        note="diagnostics JSON should be exported for malformed multi-file header-tail-garbage lane",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_multifile_macro_arity_mismatch_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_multifile_macro_arity_mismatch_no_crash.c",
        note="diagnostics JSON should be exported for malformed multi-file macro-arity-mismatch lane",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_header_guard_tail_mismatch_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_header_guard_tail_mismatch_no_crash.c",
        note="diagnostics JSON should be exported for malformed header guard-tail mismatch lane",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_header_macro_cycle_chain_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_header_macro_cycle_chain_no_crash.c",
        note="diagnostics JSON should be exported for malformed header macro-cycle chain lane",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_header_guard_tail_mismatch_chain_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_header_guard_tail_mismatch_chain_no_crash.c",
        note="diagnostics JSON should be exported for malformed header guard-tail mismatch chain lane",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_include_graph_guard_collision_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_include_graph_guard_collision_no_crash.c",
        note="diagnostics JSON should be exported for malformed include-graph guard-collision lane",
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_include_graph_macro_arity_chain_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_include_graph_macro_arity_chain_no_crash.c",
        note="diagnostics JSON should be exported for malformed include-graph macro-arity chain lane",
    ),
]


def run_cmd(cmd, timeout_sec):
    try:
        proc = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=timeout_sec,
        )
    except subprocess.TimeoutExpired as exc:
        out = (exc.stdout or "") + (exc.stderr or "")
        return 124, out, True
    return proc.returncode, proc.stdout, False


def run_binary(path, timeout_sec):
    try:
        proc = subprocess.run(
            [str(path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout_sec,
        )
    except subprocess.TimeoutExpired as exc:
        out = exc.stdout or ""
        err = exc.stderr or ""
        return 124, out, err, True
    return proc.returncode, proc.stdout, proc.stderr, False


def run_runtime_probe(probe, clang_path):
    with tempfile.TemporaryDirectory(prefix=f"probe-{probe.probe_id}-") as tmp:
        tmp_dir = Path(tmp)
        fisics_exe = tmp_dir / "fisics.out"
        clang_exe = tmp_dir / "clang.out"
        sources = list(probe.inputs) if probe.inputs else [probe.source]

        fisics_compile_exit, fisics_compile_out, fisics_compile_timeout = run_cmd(
            [str(FISICS)] + [str(src) for src in sources] + ["-o", str(fisics_exe)],
            COMPILE_TIMEOUT_SEC,
        )
        if fisics_compile_timeout:
            return (
                "BLOCKED",
                f"fisics compile timeout ({COMPILE_TIMEOUT_SEC}s)",
                fisics_compile_out.strip(),
            )
        if fisics_compile_exit != 0:
            return (
                "BLOCKED",
                f"fisics compile failed ({fisics_compile_exit})",
                fisics_compile_out.strip(),
            )

        fisics_run_exit, fisics_stdout, fisics_stderr, fisics_run_timeout = run_binary(
            fisics_exe, RUN_TIMEOUT_SEC
        )
        if fisics_run_timeout:
            return (
                "BLOCKED",
                f"fisics runtime timeout ({RUN_TIMEOUT_SEC}s)",
                "",
            )

        if not clang_path:
            return (
                "SKIP",
                "clang not found; differential unavailable",
                f"fisics exit={fisics_run_exit}, stdout={fisics_stdout.strip()}",
            )

        clang_compile_exit, clang_compile_out, clang_compile_timeout = run_cmd(
            [clang_path, "-std=c99", "-O0"] + [str(src) for src in sources] + ["-o", str(clang_exe)],
            COMPILE_TIMEOUT_SEC,
        )
        if clang_compile_timeout:
            return (
                "BLOCKED",
                f"clang compile timeout ({COMPILE_TIMEOUT_SEC}s)",
                clang_compile_out.strip(),
            )
        if clang_compile_exit != 0:
            return (
                "BLOCKED",
                f"clang compile failed ({clang_compile_exit})",
                clang_compile_out.strip(),
            )

        clang_run_exit, clang_stdout, clang_stderr, clang_run_timeout = run_binary(
            clang_exe, RUN_TIMEOUT_SEC
        )
        if clang_run_timeout:
            return (
                "BLOCKED",
                f"clang runtime timeout ({RUN_TIMEOUT_SEC}s)",
                "",
            )

        same = (
            fisics_run_exit == clang_run_exit
            and fisics_stdout == clang_stdout
            and fisics_stderr == clang_stderr
        )

        if same and probe.extra_differential_compiler:
            extra_compiler_name = probe.extra_differential_compiler
            extra_compiler_path = shutil.which(extra_compiler_name)
            if not extra_compiler_path:
                return (
                    "SKIP",
                    f"{extra_compiler_name} not found; extra differential unavailable",
                    f"fisics exit={fisics_run_exit}, stdout={fisics_stdout.strip()}",
                )

            extra_exe = tmp_dir / f"{extra_compiler_name}.out"
            extra_compile_exit, extra_compile_out, extra_compile_timeout = run_cmd(
                [extra_compiler_path, "-std=c99", "-O0"]
                + [str(src) for src in sources]
                + ["-o", str(extra_exe)],
                COMPILE_TIMEOUT_SEC,
            )
            if extra_compile_timeout:
                return (
                    "BLOCKED",
                    f"{extra_compiler_name} compile timeout ({COMPILE_TIMEOUT_SEC}s)",
                    extra_compile_out.strip(),
                )
            if extra_compile_exit != 0:
                return (
                    "BLOCKED",
                    f"{extra_compiler_name} compile failed ({extra_compile_exit})",
                    extra_compile_out.strip(),
                )

            extra_run_exit, extra_stdout, extra_stderr, extra_run_timeout = run_binary(
                extra_exe, RUN_TIMEOUT_SEC
            )
            if extra_run_timeout:
                return (
                    "BLOCKED",
                    f"{extra_compiler_name} runtime timeout ({RUN_TIMEOUT_SEC}s)",
                    "",
                )

            extra_same = (
                fisics_run_exit == extra_run_exit
                and fisics_stdout == extra_stdout
                and fisics_stderr == extra_stderr
            )
            if not extra_same:
                detail = (
                    f"fisics(exit={fisics_run_exit}, out={fisics_stdout.strip()}, err={fisics_stderr.strip()}) "
                    f"vs {extra_compiler_name}(exit={extra_run_exit}, out={extra_stdout.strip()}, err={extra_stderr.strip()})"
                )
                return (
                    "BLOCKED",
                    f"runtime mismatch vs {extra_compiler_name}",
                    detail,
                )

            return (
                "RESOLVED",
                f"matches clang+{extra_compiler_name} runtime behavior",
                f"stdout={fisics_stdout.strip()}",
            )

        if same:
            return (
                "RESOLVED",
                "matches clang runtime behavior",
                f"stdout={fisics_stdout.strip()}",
            )

        detail = (
            f"fisics(exit={fisics_run_exit}, out={fisics_stdout.strip()}, err={fisics_stderr.strip()}) "
            f"vs clang(exit={clang_run_exit}, out={clang_stdout.strip()}, err={clang_stderr.strip()})"
        )
        return ("BLOCKED", "runtime mismatch vs clang", detail)


def run_diag_probe(probe):
    sources = list(probe.inputs) if probe.inputs else [probe.source]
    with tempfile.TemporaryDirectory(prefix=f"probe-diag-{probe.probe_id}-") as tmp:
        cmd = [str(FISICS)] + [str(src) for src in sources]
        if len(sources) > 1:
            # Force full multi-input compilation/linking path for cross-TU diagnostics.
            cmd += ["-o", str(Path(tmp) / "diag.out")]
        _, out, _ = run_cmd(cmd, COMPILE_TIMEOUT_SEC)
    if probe.required_substrings:
        lowered = out.lower()
        for needle in probe.required_substrings:
            if needle.lower() in lowered:
                return ("RESOLVED", f"diagnostic now emitted ({needle})", "")
        return ("BLOCKED", "expected diagnostic substring missing", "")

    has_diag = "Error at (" in out or "Error:" in out or ": error:" in out
    if has_diag:
        return ("RESOLVED", "diagnostic now emitted", "")
    return ("BLOCKED", "diagnostic missing", "")


def run_diag_json_probe(probe):
    with tempfile.TemporaryDirectory(prefix=f"probe-diagjson-{probe.probe_id}-") as tmp:
        json_path = Path(tmp) / "diags.json"
        sources = list(probe.inputs) if probe.inputs else [probe.source]
        cmd = [str(FISICS), "--emit-diags-json", str(json_path)] + [str(src) for src in sources]
        if len(sources) > 1:
            # Force full multi-input compilation/linking path for cross-TU diagnostics.
            cmd += ["-o", str(Path(tmp) / "diagjson.out")]
        exit_code, out, timed_out = run_cmd(
            cmd,
            COMPILE_TIMEOUT_SEC,
        )
        if timed_out:
            return ("BLOCKED", f"compile timeout ({COMPILE_TIMEOUT_SEC}s)", "")
        if not json_path.exists():
            if exit_code != 0:
                return ("BLOCKED", f"compile exited {exit_code}", out.strip())
            return ("BLOCKED", "diagnostics JSON file missing", "")
        try:
            data = json.loads(json_path.read_text(encoding="utf-8"))
        except Exception as exc:
            return ("BLOCKED", f"diagnostics JSON unreadable ({exc})", "")
        diagnostics = data.get("diagnostics", [])
        if probe.require_any_diagnostic:
            if diagnostics:
                if probe.expected_codes:
                    actual_codes = [int(item.get("code", -1)) for item in diagnostics]
                    missing_codes = [code for code in probe.expected_codes if code not in actual_codes]
                    if missing_codes:
                        return ("BLOCKED", f"diagnostics JSON missing expected code(s): {missing_codes}", "")
                if probe.expected_line is not None:
                    actual_lines = [int(item.get("line", -1)) for item in diagnostics]
                    if probe.expected_line not in actual_lines:
                        return ("BLOCKED", f"diagnostics JSON missing expected line {probe.expected_line}", "")
                return ("RESOLVED", f"diagnostics JSON has {len(diagnostics)} item(s)", "")
            return ("BLOCKED", "diagnostics JSON unexpectedly empty", "")
        return ("RESOLVED", "diagnostics JSON exported", "")


def parse_probe_filters():
    raw = os.environ.get("PROBE_FILTER", "").strip()
    if not raw:
        return []
    return [part.strip() for part in raw.split(",") if part.strip()]


def probe_selected(probe_id, filters):
    if not filters:
        return True
    for token in filters:
        if token.endswith("*"):
            if probe_id.startswith(token[:-1]):
                return True
            continue
        if probe_id == token or probe_id.startswith(token):
            return True
    return False


def main():
    if not FISICS.exists():
        print(f"fisics binary not found at {FISICS}")
        return 1

    clang_path = shutil.which("clang")
    print("Probe Runner")
    print(f"fisics: {FISICS}")
    print(f"clang: {clang_path or 'not found'}")
    filters = parse_probe_filters()
    if filters:
        print(f"probe_filter: {', '.join(filters)}")
    else:
        print("probe_filter: <none>")
    print("")

    blocked = 0
    resolved = 0
    skipped = 0

    print("[runtime probes]")
    for probe in RUNTIME_PROBES:
        if not probe_selected(probe.probe_id, filters):
            continue
        status, summary, detail = run_runtime_probe(probe, clang_path)
        print(f"{status:8s} {probe.probe_id} - {summary}")
        print(f"         note: {probe.note}")
        if detail:
            print(f"         detail: {detail}")
        if status == "BLOCKED":
            blocked += 1
        elif status == "RESOLVED":
            resolved += 1
        else:
            skipped += 1

    print("")
    print("[diagnostic probes]")
    for probe in DIAG_PROBES:
        if not probe_selected(probe.probe_id, filters):
            continue
        status, summary, detail = run_diag_probe(probe)
        print(f"{status:8s} {probe.probe_id} - {summary}")
        print(f"         note: {probe.note}")
        if detail:
            print(f"         detail: {detail}")
        if status == "BLOCKED":
            blocked += 1
        elif status == "RESOLVED":
            resolved += 1
        else:
            skipped += 1

    print("")
    print("[diagnostic-json probes]")
    for probe in DIAG_JSON_PROBES:
        if not probe_selected(probe.probe_id, filters):
            continue
        status, summary, detail = run_diag_json_probe(probe)
        print(f"{status:8s} {probe.probe_id} - {summary}")
        print(f"         note: {probe.note}")
        if detail:
            print(f"         detail: {detail}")
        if status == "BLOCKED":
            blocked += 1
        elif status == "RESOLVED":
            resolved += 1
        else:
            skipped += 1

    print("")
    print(f"Summary: blocked={blocked}, resolved={resolved}, skipped={skipped}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
