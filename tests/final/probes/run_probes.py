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


@dataclass
class DiagnosticProbe:
    probe_id: str
    source: Path
    note: str
    required_substrings: Sequence[str] | None = None


@dataclass
class DiagnosticJsonProbe:
    probe_id: str
    source: Path
    note: str
    require_any_diagnostic: bool = True


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

        fisics_compile_exit, fisics_compile_out, fisics_compile_timeout = run_cmd(
            [str(FISICS), str(probe.source), "-o", str(fisics_exe)],
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
            [clang_path, "-std=c99", "-O0", str(probe.source), "-o", str(clang_exe)],
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
    _, out, _ = run_cmd([str(FISICS), str(probe.source)], COMPILE_TIMEOUT_SEC)
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
        exit_code, out, timed_out = run_cmd(
            [str(FISICS), "--emit-diags-json", str(json_path), str(probe.source)],
            COMPILE_TIMEOUT_SEC,
        )
        if timed_out:
            return ("BLOCKED", f"compile timeout ({COMPILE_TIMEOUT_SEC}s)", "")
        if exit_code != 0:
            return ("BLOCKED", f"compile exited {exit_code}", out.strip())
        if not json_path.exists():
            return ("BLOCKED", "diagnostics JSON file missing", "")
        try:
            data = json.loads(json_path.read_text(encoding="utf-8"))
        except Exception as exc:
            return ("BLOCKED", f"diagnostics JSON unreadable ({exc})", "")
        diagnostics = data.get("diagnostics", [])
        if probe.require_any_diagnostic:
            if diagnostics:
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
