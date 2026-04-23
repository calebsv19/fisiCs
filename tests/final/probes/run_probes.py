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
    expect_any_diagnostic: bool = True
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
    expected_column: int | None = None
    expected_has_file: bool | None = None
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
        probe_id="04__probe_deep_declarator_typedef_factory_runtime",
        source=PROBE_DIR / "runtime/04__probe_deep_declarator_typedef_factory_runtime.c",
        note="reduced threshold: typedef-aliased factory-call function pointer lane should compile/run and match clang",
    ),
    RuntimeProbe(
        probe_id="04__probe_deep_declarator_typedef_factory_assignment_runtime",
        source=PROBE_DIR / "runtime/04__probe_deep_declarator_typedef_factory_assignment_runtime.c",
        note="reduced threshold: typedef-aliased factory assignment lane should compile/run and match clang",
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
        probe_id="07__probe_signed_unsigned_compare_boundary_runtime",
        source=PROBE_DIR / "runtime/07__probe_signed_unsigned_compare_boundary_runtime.c",
        note="signed/unsigned boundary comparison matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="07__probe_agg_nested_offsets_runtime",
        source=PROBE_DIR / "runtime/07__probe_agg_nested_offsets_runtime.c",
        note="nested aggregate offsetof/stride invariants should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="07__probe_signed_unsigned_compare_long_boundary_runtime",
        source=PROBE_DIR / "runtime/07__probe_signed_unsigned_compare_long_boundary_runtime.c",
        note="long/unsigned-long and long-long/unsigned-long-long comparison boundary matrix should match clang",
    ),
    RuntimeProbe(
        probe_id="07__probe_null_pointer_constant_runtime",
        source=PROBE_DIR / "runtime/07__probe_null_pointer_constant_runtime.c",
        note="integer constant zero should act as null pointer constant in initialization and conditional expressions",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_array_size_positive_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_array_size_positive_runtime.c",
        note="file-scope array bounds with integer constant expressions should compile and match clang runtime output",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_case_label_positive_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_case_label_positive_runtime.c",
        note="switch case labels using integer constant expressions should compile and match clang runtime output",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_static_init_positive_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_static_init_positive_runtime.c",
        note="static-storage initializers using integer constant expressions should compile and match clang runtime output",
    ),
    RuntimeProbe(
        probe_id="07__probe_agg_union_nested_member_runtime",
        source=PROBE_DIR / "runtime/07__probe_agg_union_nested_member_runtime.c",
        note="nested struct-in-union member paths should compile and match clang runtime output",
    ),
    RuntimeProbe(
        probe_id="07__probe_agg_union_copy_update_runtime",
        source=PROBE_DIR / "runtime/07__probe_agg_union_copy_update_runtime.c",
        note="aggregate copy/update through nested union-member paths should match clang runtime output",
    ),
    RuntimeProbe(
        probe_id="07__probe_cast_ptr_uintptr_roundtrip_runtime",
        source=PROBE_DIR / "runtime/07__probe_cast_ptr_uintptr_roundtrip_runtime.c",
        note="explicit pointer<->uintptr_t cast roundtrip should match clang runtime output",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_enum_cast_sizeof_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_enum_cast_sizeof_runtime.c",
        note="nested enum/cast/sizeof constant-expression forms should compile and match clang runtime output",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_large_bound_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_large_bound_runtime.c",
        note="large stress-sized constant-expression array bounds should compile and match clang runtime output",
    ),
    RuntimeProbe(
        probe_id="07__probe_policy_impldef_ptr_ulong_roundtrip_runtime",
        source=PROBE_DIR / "runtime/07__probe_policy_impldef_ptr_ulong_roundtrip_runtime.c",
        note="implementation-defined pointer<->unsigned long cast roundtrip should compile and run deterministically on host",
    ),
    RuntimeProbe(
        probe_id="07__probe_policy_impldef_int_to_ptr_nonzero_runtime",
        source=PROBE_DIR / "runtime/07__probe_policy_impldef_int_to_ptr_nonzero_runtime.c",
        note="implementation-defined nonzero integer-to-pointer cast lane should compile and run without dereference",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_fold_ternary_bitwise_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_fold_ternary_bitwise_runtime.c",
        note="compound ternary/bitwise constant-expression folds should compile and match clang runtime output",
    ),
    RuntimeProbe(
        probe_id="07__probe_policy_impldef_ptr_slong_roundtrip_runtime",
        source=PROBE_DIR / "runtime/07__probe_policy_impldef_ptr_slong_roundtrip_runtime.c",
        note="implementation-defined pointer<->signed long cast roundtrip should compile and run deterministically on host",
    ),
    RuntimeProbe(
        probe_id="07__probe_policy_explicit_nonzero_int_cast_ptr_runtime",
        source=PROBE_DIR / "runtime/07__probe_policy_explicit_nonzero_int_cast_ptr_runtime.c",
        note="explicit nonzero integer-to-pointer cast lane should compile and run without dereference",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_multidim_fold_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_multidim_fold_runtime.c",
        note="multi-dimensional array bounds and designated array initializers using compound constant-expression chains should match clang",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_array_chain_depth_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_array_chain_depth_runtime.c",
        note="deep chained integer-constant-expression array bounds should compile and match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_case_chain_depth_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_case_chain_depth_runtime.c",
        note="deep chained integer-constant-expression case labels should compile and match clang+gcc runtime behavior",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="07__probe_policy_strict_zero_cast_matrix_runtime",
        source=PROBE_DIR / "runtime/07__probe_policy_strict_zero_cast_matrix_runtime.c",
        note="strict policy-safe null-pointer constant and void*-roundtrip conversion matrix should match clang+gcc runtime behavior",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_static_chain_depth_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_static_chain_depth_runtime.c",
        note="deep chained integer-constant-expression static initializer forms should compile and match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_switch_cast_sizeof_depth_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_switch_cast_sizeof_depth_runtime.c",
        note="mixed cast/sizeof integer-constant-expression switch-selector forms should compile and match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="07__probe_policy_strict_fnptr_decay_null_runtime",
        source=PROBE_DIR / "runtime/07__probe_policy_strict_fnptr_decay_null_runtime.c",
        note="strict function-pointer decay/null-check/call conversion matrix should match clang+gcc runtime behavior",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="07__probe_policy_strict_array_decay_ptrdiff_runtime",
        source=PROBE_DIR / "runtime/07__probe_policy_strict_array_decay_ptrdiff_runtime.c",
        note="strict array-decay, void*-roundtrip, and in-array pointer-difference matrix should match clang+gcc runtime behavior",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_static_ternary_bitcast_depth_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_static_ternary_bitcast_depth_runtime.c",
        note="deeper chained static-initializer integer-constant-expression forms using ternary/bitwise/casts should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="07__probe_policy_strict_const_ptr_roundtrip_runtime",
        source=PROBE_DIR / "runtime/07__probe_policy_strict_const_ptr_roundtrip_runtime.c",
        note="strict const-qualified pointer-to-void roundtrip matrix should match clang+gcc runtime behavior",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="07__probe_policy_strict_null_conditional_matrix_runtime",
        source=PROBE_DIR / "runtime/07__probe_policy_strict_null_conditional_matrix_runtime.c",
        note="strict null-pointer conditional conversion matrix should match clang+gcc runtime behavior",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="07__probe_policy_strict_fnptr_table_null_dispatch_runtime",
        source=PROBE_DIR / "runtime/07__probe_policy_strict_fnptr_table_null_dispatch_runtime.c",
        note="strict function-pointer table null-dispatch conversion matrix should match clang+gcc runtime behavior",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_array_static_shared_chain_depth_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_array_static_shared_chain_depth_runtime.c",
        note="deeper chained ICEs shared between array bounds and static initializers should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_switch_nested_ternary_bitwise_depth_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_switch_nested_ternary_bitwise_depth_runtime.c",
        note="nested ternary+bitwise ICE switch-label matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_static_cast_sizeof_ternary_depth_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_static_cast_sizeof_ternary_depth_runtime.c",
        note="deep cast+sizeof+ternary static-initializer ICE matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="07__probe_policy_strict_const_compare_matrix_runtime",
        source=PROBE_DIR / "runtime/07__probe_policy_strict_const_compare_matrix_runtime.c",
        note="strict const-qualified pointer comparison matrix over shared array domain should match clang+gcc runtime behavior",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="07__probe_policy_strict_const_chain_assignment_runtime",
        source=PROBE_DIR / "runtime/07__probe_policy_strict_const_chain_assignment_runtime.c",
        note="strict const-qualified pointer assignment/void-roundtrip chain should match clang+gcc runtime behavior",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="07__probe_policy_strict_ptr_compare_one_past_runtime",
        source=PROBE_DIR / "runtime/07__probe_policy_strict_ptr_compare_one_past_runtime.c",
        note="strict one-past pointer comparison and ptrdiff matrix should match clang+gcc runtime behavior",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_shared_ice_matrix_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_shared_ice_matrix_runtime.c",
        note="shared ICE matrix coupling array-size, switch labels, and static initializers should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_shared_ice_matrix_nested_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_shared_ice_matrix_nested_runtime.c",
        note="nested shared ICE matrix coupling array-size, switch labels, and static initializers should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="07__probe_constexpr_shared_ice_cross_surface_depth_runtime",
        source=PROBE_DIR / "runtime/07__probe_constexpr_shared_ice_cross_surface_depth_runtime.c",
        note="cross-surface ICE depth matrix with coupled array bounds, switch labels, and static initializers should match clang runtime behavior",
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
        probe_id="14__probe_multitu_link_order_static_init_crc_matrix",
        source=PROBE_DIR / "runtime/14__probe_multitu_link_order_static_init_crc_matrix_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_link_order_static_init_crc_matrix_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_link_order_static_init_crc_matrix_lib_a.c",
            PROBE_DIR / "runtime/14__probe_multitu_link_order_static_init_crc_matrix_lib_b.c",
        ),
        note="multi-TU static-init and call-order CRC matrix should remain deterministic and match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_repeatable_multitu_symbol_permutation_hash",
        source=PROBE_DIR / "runtime/14__probe_repeatable_multitu_symbol_permutation_hash_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_repeatable_multitu_symbol_permutation_hash_main.c",
            PROBE_DIR / "runtime/14__probe_repeatable_multitu_symbol_permutation_hash_lib.c",
        ),
        note="multi-TU symbol permutation dispatch hash should remain deterministic and match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_repeatable_runtime_stdout_hash_matrix",
        source=PROBE_DIR / "runtime/14__probe_repeatable_runtime_stdout_hash_matrix.c",
        note="text-output hash matrix should remain deterministic across repeated runs and match clang runtime behavior",
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
        probe_id="14__probe_queue_push_fail_preserves_committed_slot",
        source=PROBE_DIR / "runtime/14__probe_queue_push_fail_preserves_committed_slot.c",
        note="queue push-fail path should preserve committed slot state and match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_token_generation_stale_drop",
        source=PROBE_DIR / "runtime/14__probe_multitu_token_generation_stale_drop_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_token_generation_stale_drop_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_token_generation_stale_drop_lib.c",
        ),
        note="multi-TU token generation stale-drop path should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_owner_transfer_release_once",
        source=PROBE_DIR / "runtime/14__probe_multitu_owner_transfer_release_once_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_owner_transfer_release_once_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_owner_transfer_release_once_lib.c",
        ),
        note="multi-TU owner-transfer release-once path should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_queue_pop_fail_preserves_window_state",
        source=PROBE_DIR / "runtime/14__probe_queue_pop_fail_preserves_window_state.c",
        note="queue pop-fail path should preserve read window state and match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_token_reserve_commit_abort",
        source=PROBE_DIR / "runtime/14__probe_multitu_token_reserve_commit_abort_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_token_reserve_commit_abort_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_token_reserve_commit_abort_lib.c",
        ),
        note="multi-TU token reserve/commit/abort transitions should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_layer_budget_trim_generation",
        source=PROBE_DIR / "runtime/14__probe_multitu_layer_budget_trim_generation_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_layer_budget_trim_generation_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_layer_budget_trim_generation_lib.c",
        ),
        note="multi-TU layer-budget trim and generation transitions should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_realproj_map_forge_layer_toggle_residency_trim_runtime",
        source=PROBE_DIR / "runtime/14__probe_realproj_map_forge_layer_toggle_residency_trim_runtime.c",
        note="reduced map_forge layer-toggle and residency trim behavior should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_realproj_map_forge_tile_queue_generation_guard_runtime",
        source=PROBE_DIR / "runtime/14__probe_realproj_map_forge_tile_queue_generation_guard_runtime.c",
        note="reduced map_forge tile-queue generation guard behavior should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_realproj_map_forge_owner_transfer_single_release_runtime",
        source=PROBE_DIR / "runtime/14__probe_realproj_map_forge_owner_transfer_single_release_runtime.c",
        note="reduced map_forge owner-transfer single-release behavior should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_cleanup_double_call_guard_matrix",
        source=PROBE_DIR / "runtime/14__probe_cleanup_double_call_guard_matrix.c",
        note="cleanup double-call guard matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_queue_trim_idempotent_window_matrix",
        source=PROBE_DIR / "runtime/14__probe_queue_trim_idempotent_window_matrix.c",
        note="queue trim idempotent-window matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_generation_gate_stale_entry_drop_matrix",
        source=PROBE_DIR / "runtime/14__probe_generation_gate_stale_entry_drop_matrix.c",
        note="generation-gate stale-entry drop matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_fnptr_struct_return_long_double_bridge",
        source=PROBE_DIR / "runtime/14__probe_fnptr_struct_return_long_double_bridge.c",
        note="function-pointer selected long-double struct return bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_variadic_fnptr_struct_payload_bridge",
        source=PROBE_DIR / "runtime/14__probe_variadic_fnptr_struct_payload_bridge.c",
        note="variadic function-pointer struct payload fold bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_fnptr_variadic_state_pipeline",
        source=PROBE_DIR / "runtime/14__probe_multitu_fnptr_variadic_state_pipeline_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_fnptr_variadic_state_pipeline_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_fnptr_variadic_state_pipeline_lib.c",
        ),
        note="multi-TU variadic function-pointer state pipeline should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_restrict_char_alias_rebase_matrix",
        source=PROBE_DIR / "runtime/14__probe_restrict_char_alias_rebase_matrix.c",
        note="restrict copy with char-alias rebasing matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_ptrdiff_roundtrip_through_uintptr_matrix",
        source=PROBE_DIR / "runtime/14__probe_ptrdiff_roundtrip_through_uintptr_matrix.c",
        note="ptrdiff roundtrip through uintptr bridge matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_ptr_alias_window_stability",
        source=PROBE_DIR / "runtime/14__probe_multitu_ptr_alias_window_stability_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_ptr_alias_window_stability_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_ptr_alias_window_stability_lib.c",
        ),
        note="multi-TU pointer alias window stability path should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave1_multitu_tentative_def_coalesce_reloc_matrix",
        source=PROBE_DIR / "runtime/14__probe_axis1_wave1_multitu_tentative_def_coalesce_reloc_matrix_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_axis1_wave1_multitu_tentative_def_coalesce_reloc_matrix_main.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave1_multitu_tentative_def_coalesce_reloc_matrix_lib.c",
        ),
        note="axis1 wave1: multi-TU tentative-definition coalesce and relocation matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave1_multitu_static_extern_shadow_reloc_matrix",
        source=PROBE_DIR / "runtime/14__probe_axis1_wave1_multitu_static_extern_shadow_reloc_matrix_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_axis1_wave1_multitu_static_extern_shadow_reloc_matrix_main.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave1_multitu_static_extern_shadow_reloc_matrix_lib.c",
        ),
        note="axis1 wave1: static-vs-extern same-name shadow relocation matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave2_multitu_weak_strong_symbol_precedence_matrix",
        source=PROBE_DIR / "runtime/14__probe_axis1_wave2_multitu_weak_strong_symbol_precedence_matrix_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_axis1_wave2_multitu_weak_strong_symbol_precedence_matrix_main.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave2_multitu_weak_strong_symbol_precedence_matrix_lib.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave2_multitu_weak_strong_symbol_precedence_matrix_aux.c",
        ),
        note="axis1 wave2: multi-TU weak/common-vs-strong symbol precedence matrix should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave2_multitu_extern_array_extent_reloc_bridge",
        source=PROBE_DIR / "runtime/14__probe_axis1_wave2_multitu_extern_array_extent_reloc_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_axis1_wave2_multitu_extern_array_extent_reloc_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave2_multitu_extern_array_extent_reloc_bridge_lib.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave2_multitu_extern_array_extent_reloc_bridge_aux.c",
        ),
        note="axis1 wave2: multi-TU extern-array extent relocation bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave3_multitu_const_table_reloc_crc",
        source=PROBE_DIR / "runtime/14__probe_axis1_wave3_multitu_const_table_reloc_crc_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_axis1_wave3_multitu_const_table_reloc_crc_main.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave3_multitu_const_table_reloc_crc_lib.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave3_multitu_const_table_reloc_crc_aux.c",
        ),
        note="axis1 wave3: multi-TU const-table relocation CRC fold should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave3_multitu_static_fnptr_export_bridge",
        source=PROBE_DIR / "runtime/14__probe_axis1_wave3_multitu_static_fnptr_export_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_axis1_wave3_multitu_static_fnptr_export_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave3_multitu_static_fnptr_export_bridge_lib.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave3_multitu_static_fnptr_export_bridge_aux.c",
        ),
        note="axis1 wave3: multi-TU static function-pointer export bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave3_multitu_static_fnptr_export_bridge_reduced",
        source=PROBE_DIR / "runtime/14__probe_axis1_wave3_multitu_static_fnptr_export_bridge_reduced_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_axis1_wave3_multitu_static_fnptr_export_bridge_reduced_main.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave3_multitu_static_fnptr_export_bridge_reduced_lib.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave3_multitu_static_fnptr_export_bridge_reduced_aux.c",
        ),
        note="axis1 wave3 reduced: single-path function-pointer export bridge remains stable while branch-lane strict probe is blocked",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave4_multitu_fnptr_table_rebase_dispatch_matrix",
        source=PROBE_DIR / "runtime/14__probe_axis1_wave4_multitu_fnptr_table_rebase_dispatch_matrix_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_axis1_wave4_multitu_fnptr_table_rebase_dispatch_matrix_main.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave4_multitu_fnptr_table_rebase_dispatch_matrix_lib.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave4_multitu_fnptr_table_rebase_dispatch_matrix_aux.c",
        ),
        note="axis1 wave4: multi-TU exported function-pointer table dispatch should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave5_multitu_fnptr_typedef_qualifier_bridge",
        source=PROBE_DIR / "runtime/14__probe_axis1_wave5_multitu_fnptr_typedef_qualifier_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_axis1_wave5_multitu_fnptr_typedef_qualifier_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave5_multitu_fnptr_typedef_qualifier_bridge_lib.c",
            PROBE_DIR / "runtime/14__probe_axis1_wave5_multitu_fnptr_typedef_qualifier_bridge_aux.c",
        ),
        note="axis1 wave5: multi-TU function-pointer typedef/qualifier bridge should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave6_multitu_fnptr_callback_return_reseed_matrix",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave6_multitu_fnptr_callback_return_reseed_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave6_multitu_fnptr_callback_return_reseed_matrix_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave6_multitu_fnptr_callback_return_reseed_matrix_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave6_multitu_fnptr_callback_return_reseed_matrix_aux.c",
        ),
        note="axis1 wave6: callback-returned function-pointer reseed matrix should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave6_multitu_fnptr_array_permute_dispatch_matrix",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave6_multitu_fnptr_array_permute_dispatch_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave6_multitu_fnptr_array_permute_dispatch_matrix_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave6_multitu_fnptr_array_permute_dispatch_matrix_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave6_multitu_fnptr_array_permute_dispatch_matrix_aux.c",
        ),
        note="axis1 wave6: function-pointer array permutation dispatch matrix should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave7_multitu_fnptr_owner_table_rotation_matrix",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave7_multitu_fnptr_owner_table_rotation_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave7_multitu_fnptr_owner_table_rotation_matrix_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave7_multitu_fnptr_owner_table_rotation_matrix_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave7_multitu_fnptr_owner_table_rotation_matrix_aux.c",
        ),
        note="axis1 wave7: owner-table rotation over exported function-pointer mix lanes should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave7_multitu_owner_window_pointer_depth_matrix",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave7_multitu_owner_window_pointer_depth_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave7_multitu_owner_window_pointer_depth_matrix_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave7_multitu_owner_window_pointer_depth_matrix_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave7_multitu_owner_window_pointer_depth_matrix_aux.c",
        ),
        note="axis1 wave7: const-owner window pointer-depth matrix should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave9_multitu_fnptr_owner_ring_reseed_matrix",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave9_multitu_fnptr_owner_ring_reseed_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave9_multitu_fnptr_owner_ring_reseed_matrix_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave9_multitu_fnptr_owner_ring_reseed_matrix_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave9_multitu_fnptr_owner_ring_reseed_matrix_aux.c",
        ),
        note="axis1 wave9: owner-ring function-pointer reseed matrix should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave9_multitu_owner_route_window_depth_matrix",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave9_multitu_owner_route_window_depth_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave9_multitu_owner_route_window_depth_matrix_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave9_multitu_owner_route_window_depth_matrix_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave9_multitu_owner_route_window_depth_matrix_aux.c",
        ),
        note="axis1 wave9: owner-route window depth matrix should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave10_multitu_fnptr_owner_checkpoint_dispatch_matrix",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave10_multitu_fnptr_owner_checkpoint_dispatch_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave10_multitu_fnptr_owner_checkpoint_dispatch_matrix_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave10_multitu_fnptr_owner_checkpoint_dispatch_matrix_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave10_multitu_fnptr_owner_checkpoint_dispatch_matrix_aux.c",
        ),
        note="axis1 wave10: owner-checkpoint function-pointer dispatch matrix should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave10_multitu_owner_checkpoint_window_bridge",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave10_multitu_owner_checkpoint_window_bridge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave10_multitu_owner_checkpoint_window_bridge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave10_multitu_owner_checkpoint_window_bridge_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave10_multitu_owner_checkpoint_window_bridge_aux.c",
        ),
        note="axis1 wave10: owner-checkpoint window bridge should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave11_multitu_ptrtable_nested_decay_rebase_matrix",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave11_multitu_ptrtable_nested_decay_rebase_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave11_multitu_ptrtable_nested_decay_rebase_matrix_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave11_multitu_ptrtable_nested_decay_rebase_matrix_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave11_multitu_ptrtable_nested_decay_rebase_matrix_aux.c",
        ),
        note="axis1 wave11: nested decay pointer-table rebase matrix should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave11_multitu_ptrtable_two_step_rebase_window_bridge",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave11_multitu_ptrtable_two_step_rebase_window_bridge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave11_multitu_ptrtable_two_step_rebase_window_bridge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave11_multitu_ptrtable_two_step_rebase_window_bridge_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave11_multitu_ptrtable_two_step_rebase_window_bridge_aux.c",
        ),
        note="axis1 wave11: two-step pointer-table window rebase bridge should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave12_multitu_ptrtable_checkpoint_linkorder_reseed_matrix",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave12_multitu_ptrtable_checkpoint_linkorder_reseed_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave12_multitu_ptrtable_checkpoint_linkorder_reseed_matrix_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave12_multitu_ptrtable_checkpoint_linkorder_reseed_matrix_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave12_multitu_ptrtable_checkpoint_linkorder_reseed_matrix_aux.c",
        ),
        note="axis1 wave12: checkpoint link-order pointer-table reseed matrix should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave12_multitu_ptrtable_signed_unsigned_offset_window_bridge",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave12_multitu_ptrtable_signed_unsigned_offset_window_bridge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave12_multitu_ptrtable_signed_unsigned_offset_window_bridge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave12_multitu_ptrtable_signed_unsigned_offset_window_bridge_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave12_multitu_ptrtable_signed_unsigned_offset_window_bridge_aux.c",
        ),
        note="axis1 wave12: mixed signed/unsigned offset pointer-table bridge should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave13_multitu_ptrtable_four_level_rebase_permutation_matrix",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave13_multitu_ptrtable_four_level_rebase_permutation_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave13_multitu_ptrtable_four_level_rebase_permutation_matrix_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave13_multitu_ptrtable_four_level_rebase_permutation_matrix_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave13_multitu_ptrtable_four_level_rebase_permutation_matrix_aux.c",
        ),
        note="axis1 wave13: four-level pointer-table rebase permutation matrix should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave13_multitu_ptrtable_constexpr_offset_blend_bridge",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave13_multitu_ptrtable_constexpr_offset_blend_bridge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave13_multitu_ptrtable_constexpr_offset_blend_bridge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave13_multitu_ptrtable_constexpr_offset_blend_bridge_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave13_multitu_ptrtable_constexpr_offset_blend_bridge_aux.c",
        ),
        note="axis1 wave13: constexpr signed/unsigned offset blend bridge should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave14_multitu_ptrtable_relocation_order_checkpoint_matrix",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave14_multitu_ptrtable_relocation_order_checkpoint_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave14_multitu_ptrtable_relocation_order_checkpoint_matrix_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave14_multitu_ptrtable_relocation_order_checkpoint_matrix_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave14_multitu_ptrtable_relocation_order_checkpoint_matrix_aux.c",
        ),
        note="axis1 wave14: relocation-order checkpoint matrix over signed/unsigned offsets should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave14_multitu_ptrtable_signed_unsigned_checkpoint_replay_bridge",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave14_multitu_ptrtable_signed_unsigned_checkpoint_replay_bridge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave14_multitu_ptrtable_signed_unsigned_checkpoint_replay_bridge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave14_multitu_ptrtable_signed_unsigned_checkpoint_replay_bridge_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave14_multitu_ptrtable_signed_unsigned_checkpoint_replay_bridge_aux.c",
        ),
        note="axis1 wave14: signed/unsigned checkpoint replay bridge should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave15_multitu_ptrtable_three_plan_rebase_drift_matrix",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave15_multitu_ptrtable_three_plan_rebase_drift_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave15_multitu_ptrtable_three_plan_rebase_drift_matrix_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave15_multitu_ptrtable_three_plan_rebase_drift_matrix_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave15_multitu_ptrtable_three_plan_rebase_drift_matrix_aux.c",
        ),
        note="axis1 wave15: three-plan pointer-table rebase drift matrix should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis1_wave15_multitu_ptrtable_checkpoint_replay_permutation_bridge",
        source=PROBE_DIR
        / "runtime/14__probe_axis1_wave15_multitu_ptrtable_checkpoint_replay_permutation_bridge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis1_wave15_multitu_ptrtable_checkpoint_replay_permutation_bridge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave15_multitu_ptrtable_checkpoint_replay_permutation_bridge_lib.c",
            PROBE_DIR
            / "runtime/14__probe_axis1_wave15_multitu_ptrtable_checkpoint_replay_permutation_bridge_aux.c",
        ),
        note="axis1 wave15: permutation-depth checkpoint replay bridge should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave1_multitu_abi_hfa_struct_return_variadic_edge",
        source=PROBE_DIR / "runtime/14__probe_axis3_wave1_multitu_abi_hfa_struct_return_variadic_edge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_axis3_wave1_multitu_abi_hfa_struct_return_variadic_edge_main.c",
            PROBE_DIR / "runtime/14__probe_axis3_wave1_multitu_abi_hfa_struct_return_variadic_edge_lib.c",
        ),
        note="axis3 wave1: multi-TU HFA struct-return plus variadic edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave2_multitu_abi_hfa_struct_return_variadic_permute_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave2_multitu_abi_hfa_struct_return_variadic_permute_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave2_multitu_abi_hfa_struct_return_variadic_permute_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave2_multitu_abi_hfa_struct_return_variadic_permute_edge_lib.c",
        ),
        note="axis3 wave2: multi-TU HFA struct-return plus variadic permutation edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave3_multitu_abi_hfa_struct_return_variadic_crossmix_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave3_multitu_abi_hfa_struct_return_variadic_crossmix_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave3_multitu_abi_hfa_struct_return_variadic_crossmix_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave3_multitu_abi_hfa_struct_return_variadic_crossmix_edge_lib.c",
        ),
        note="axis3 wave3: multi-TU HFA struct-return plus variadic crossmix edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave4_multitu_abi_hfa_struct_return_variadic_regstack_pressure_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave4_multitu_abi_hfa_struct_return_variadic_regstack_pressure_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave4_multitu_abi_hfa_struct_return_variadic_regstack_pressure_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave4_multitu_abi_hfa_struct_return_variadic_regstack_pressure_edge_lib.c",
        ),
        note="axis3 wave4: multi-TU HFA struct-return plus variadic reg/stack pressure edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave5_multitu_abi_hfa_struct_return_variadic_checkpoint_rotation_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave5_multitu_abi_hfa_struct_return_variadic_checkpoint_rotation_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave5_multitu_abi_hfa_struct_return_variadic_checkpoint_rotation_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave5_multitu_abi_hfa_struct_return_variadic_checkpoint_rotation_edge_lib.c",
        ),
        note="axis3 wave5: multi-TU HFA struct-return plus variadic checkpoint-rotation edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave6_multitu_abi_hfa_struct_return_variadic_epoch_frontier_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave6_multitu_abi_hfa_struct_return_variadic_epoch_frontier_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave6_multitu_abi_hfa_struct_return_variadic_epoch_frontier_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave6_multitu_abi_hfa_struct_return_variadic_epoch_frontier_edge_lib.c",
        ),
        note="axis3 wave6: multi-TU HFA struct-return plus variadic epoch/frontier edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave7_multitu_abi_hfa_struct_return_variadic_frontier_replay_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave7_multitu_abi_hfa_struct_return_variadic_frontier_replay_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave7_multitu_abi_hfa_struct_return_variadic_frontier_replay_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave7_multitu_abi_hfa_struct_return_variadic_frontier_replay_edge_lib.c",
        ),
        note="axis3 wave7: multi-TU HFA struct-return plus variadic frontier/replay edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave8_multitu_abi_hfa_struct_return_variadic_handoff_permute_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave8_multitu_abi_hfa_struct_return_variadic_handoff_permute_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave8_multitu_abi_hfa_struct_return_variadic_handoff_permute_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave8_multitu_abi_hfa_struct_return_variadic_handoff_permute_edge_lib.c",
        ),
        note="axis3 wave8: multi-TU HFA struct-return plus variadic handoff/permutation edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave9_multitu_abi_hfa_struct_return_variadic_epoch_handoff_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave9_multitu_abi_hfa_struct_return_variadic_epoch_handoff_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave9_multitu_abi_hfa_struct_return_variadic_epoch_handoff_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave9_multitu_abi_hfa_struct_return_variadic_epoch_handoff_edge_lib.c",
        ),
        note="axis3 wave9: multi-TU HFA struct-return plus variadic epoch/handoff edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave10_multitu_abi_hfa_struct_return_variadic_frontier_fold_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave10_multitu_abi_hfa_struct_return_variadic_frontier_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave10_multitu_abi_hfa_struct_return_variadic_frontier_fold_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave10_multitu_abi_hfa_struct_return_variadic_frontier_fold_edge_lib.c",
        ),
        note="axis3 wave10: multi-TU HFA struct-return plus variadic frontier/fold edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave11_multitu_abi_hfa_struct_return_variadic_checkpoint_fold_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave11_multitu_abi_hfa_struct_return_variadic_checkpoint_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave11_multitu_abi_hfa_struct_return_variadic_checkpoint_fold_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave11_multitu_abi_hfa_struct_return_variadic_checkpoint_fold_edge_lib.c",
        ),
        note="axis3 wave11: multi-TU HFA struct-return plus variadic checkpoint/fold edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave12_multitu_abi_hfa_struct_return_variadic_rotation_fold_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave12_multitu_abi_hfa_struct_return_variadic_rotation_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave12_multitu_abi_hfa_struct_return_variadic_rotation_fold_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave12_multitu_abi_hfa_struct_return_variadic_rotation_fold_edge_lib.c",
        ),
        note="axis3 wave12: multi-TU HFA struct-return plus variadic rotation/fold edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave13_multitu_abi_hfa_struct_return_variadic_epoch_rotation_fold_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave13_multitu_abi_hfa_struct_return_variadic_epoch_rotation_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave13_multitu_abi_hfa_struct_return_variadic_epoch_rotation_fold_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave13_multitu_abi_hfa_struct_return_variadic_epoch_rotation_fold_edge_lib.c",
        ),
        note="axis3 wave13: multi-TU HFA struct-return plus variadic epoch-rotation/fold edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave14_multitu_abi_hfa_struct_return_variadic_frontier_epoch_fold_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave14_multitu_abi_hfa_struct_return_variadic_frontier_epoch_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave14_multitu_abi_hfa_struct_return_variadic_frontier_epoch_fold_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave14_multitu_abi_hfa_struct_return_variadic_frontier_epoch_fold_edge_lib.c",
        ),
        note="axis3 wave14: multi-TU HFA struct-return plus variadic frontier-epoch/fold edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave15_multitu_abi_hfa_struct_return_variadic_regstack_frontier_fold_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave15_multitu_abi_hfa_struct_return_variadic_regstack_frontier_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave15_multitu_abi_hfa_struct_return_variadic_regstack_frontier_fold_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave15_multitu_abi_hfa_struct_return_variadic_regstack_frontier_fold_edge_lib.c",
        ),
        note="axis3 wave15: multi-TU HFA struct-return plus variadic regstack/frontier fold edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave16_multitu_abi_hfa_struct_return_variadic_regwindow_handoff_fold_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave16_multitu_abi_hfa_struct_return_variadic_regwindow_handoff_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave16_multitu_abi_hfa_struct_return_variadic_regwindow_handoff_fold_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave16_multitu_abi_hfa_struct_return_variadic_regwindow_handoff_fold_edge_lib.c",
        ),
        note="axis3 wave16: multi-TU HFA struct-return plus variadic regwindow/handoff fold edge lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="14__probe_axis3_wave17_multitu_abi_hfa_struct_return_variadic_spill_checkpoint_fold_edge",
        source=PROBE_DIR
        / "runtime/14__probe_axis3_wave17_multitu_abi_hfa_struct_return_variadic_spill_checkpoint_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/14__probe_axis3_wave17_multitu_abi_hfa_struct_return_variadic_spill_checkpoint_fold_edge_main.c",
            PROBE_DIR
            / "runtime/14__probe_axis3_wave17_multitu_abi_hfa_struct_return_variadic_spill_checkpoint_fold_edge_lib.c",
        ),
        note="axis3 wave17: multi-TU HFA struct-return plus variadic spill/checkpoint fold edge lane should match clang runtime behavior",
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
        probe_id="15__probe_multitu_queue_rollback_pressure",
        source=PROBE_DIR / "runtime/15__probe_multitu_queue_rollback_pressure_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_queue_rollback_pressure_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_queue_rollback_pressure_lib.c",
            PROBE_DIR / "runtime/15__probe_multitu_queue_rollback_pressure_aux.c",
        ),
        note="multi-TU queue rollback-pressure lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_inline_reset_generation_bridge",
        source=PROBE_DIR / "runtime/15__probe_multitu_inline_reset_generation_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_inline_reset_generation_bridge_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_inline_reset_generation_bridge_lib.c",
        ),
        note="multi-TU inline reset generation bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_queue_epoch_reclaim_pressure",
        source=PROBE_DIR / "runtime/15__probe_multitu_queue_epoch_reclaim_pressure_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_queue_epoch_reclaim_pressure_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_queue_epoch_reclaim_pressure_lib.c",
            PROBE_DIR / "runtime/15__probe_multitu_queue_epoch_reclaim_pressure_aux.c",
        ),
        note="multi-TU queue epoch/reclaim pressure lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_token_window_wraparound_churn",
        source=PROBE_DIR / "runtime/15__probe_multitu_token_window_wraparound_churn_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_token_window_wraparound_churn_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_token_window_wraparound_churn_lib.c",
        ),
        note="multi-TU token-window wraparound churn lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_owner_epoch_rotation_matrix",
        source=PROBE_DIR / "runtime/15__probe_multitu_owner_epoch_rotation_matrix_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_owner_epoch_rotation_matrix_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_owner_epoch_rotation_matrix_lib.c",
        ),
        note="multi-TU owner epoch-rotation matrix lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_queue_generation_reclaim_churn",
        source=PROBE_DIR / "runtime/15__probe_multitu_queue_generation_reclaim_churn_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_queue_generation_reclaim_churn_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_queue_generation_reclaim_churn_lib.c",
        ),
        note="multi-TU queue generation/reclaim churn lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_owner_transfer_toggle_storm",
        source=PROBE_DIR / "runtime/15__probe_multitu_owner_transfer_toggle_storm_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_owner_transfer_toggle_storm_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_owner_transfer_toggle_storm_lib.c",
        ),
        note="multi-TU owner transfer/toggle storm lane should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_token_epoch_wraparound_reseed",
        source=PROBE_DIR / "runtime/15__probe_multitu_token_epoch_wraparound_reseed_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_token_epoch_wraparound_reseed_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_token_epoch_wraparound_reseed_lib.c",
        ),
        note="multi-TU token epoch wraparound/reseed lane should match clang runtime behavior",
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
        probe_id="15__probe_corpus_realproj_map_forge_app_fragment_a_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_realproj_map_forge_app_fragment_a_smoke.c",
        note="map_forge reduced app fragment A corpus lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_realproj_map_forge_app_fragment_b_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_realproj_map_forge_app_fragment_b_smoke.c",
        note="map_forge reduced app fragment B corpus lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_realproj_map_forge_app_fragment_c_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_realproj_map_forge_app_fragment_c_smoke.c",
        note="map_forge reduced app fragment C corpus lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_realproj_map_forge_app_fragment_d_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_realproj_map_forge_app_fragment_d_smoke.c",
        note="map_forge reduced app fragment D corpus lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_realproj_map_forge_app_fragment_e_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_realproj_map_forge_app_fragment_e_smoke.c",
        note="map_forge reduced app fragment E corpus lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_corpus_reduction_replay_hash",
        source=PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_lib.c",
        ),
        note="multi-TU corpus reduction/replay hash lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_realproj_map_forge_app_fragment_f_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_realproj_map_forge_app_fragment_f_smoke.c",
        note="map_forge reduced app fragment F corpus lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_realproj_map_forge_app_fragment_g_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_realproj_map_forge_app_fragment_g_smoke.c",
        note="map_forge reduced app fragment G corpus lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_corpus_reduction_replay_hash_ii",
        source=PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_ii_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_ii_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_ii_lib.c",
        ),
        note="second multi-TU corpus reduction/replay hash lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_realproj_map_forge_app_fragment_h_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_realproj_map_forge_app_fragment_h_smoke.c",
        note="map_forge reduced app fragment H corpus lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_realproj_map_forge_app_fragment_i_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_realproj_map_forge_app_fragment_i_smoke.c",
        note="map_forge reduced app fragment I corpus lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_corpus_reduction_replay_hash_iii",
        source=PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_iii_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_iii_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_iii_lib.c",
        ),
        note="third multi-TU corpus reduction/replay hash lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_realproj_map_forge_app_fragment_j_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_realproj_map_forge_app_fragment_j_smoke.c",
        note="map_forge reduced app fragment J corpus lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_realproj_map_forge_app_fragment_k_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_realproj_map_forge_app_fragment_k_smoke.c",
        note="map_forge reduced app fragment K corpus lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_corpus_reduction_replay_hash_iv",
        source=PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_iv_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_iv_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_iv_lib.c",
        ),
        note="fourth multi-TU corpus reduction/replay hash lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_realproj_map_forge_app_fragment_l_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_realproj_map_forge_app_fragment_l_smoke.c",
        note="map_forge reduced app fragment L corpus lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_corpus_realproj_map_forge_app_fragment_m_smoke",
        source=PROBE_DIR / "runtime/15__probe_corpus_realproj_map_forge_app_fragment_m_smoke.c",
        note="map_forge reduced app fragment M corpus lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_multitu_corpus_reduction_replay_hash_v",
        source=PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_v_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_v_main.c",
            PROBE_DIR / "runtime/15__probe_multitu_corpus_reduction_replay_hash_v_lib.c",
        ),
        note="fifth multi-TU corpus reduction/replay hash lane should compile/run deterministically and match clang",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave1_multitu_fn_data_symbol_interleave_reloc_hash",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave1_multitu_fn_data_symbol_interleave_reloc_hash_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave1_multitu_fn_data_symbol_interleave_reloc_hash_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave1_multitu_fn_data_symbol_interleave_reloc_hash_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave1_multitu_fn_data_symbol_interleave_reloc_hash_aux.c",
        ),
        note="axis1 wave1: multi-TU function+data symbol interleave relocation hash should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave2_multitu_common_vs_nocommon_emission_hash",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave2_multitu_common_vs_nocommon_emission_hash_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave2_multitu_common_vs_nocommon_emission_hash_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave2_multitu_common_vs_nocommon_emission_hash_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave2_multitu_common_vs_nocommon_emission_hash_aux.c",
        ),
        note="axis1 wave2: multi-TU common-vs-nocommon symbol emission hash should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave3_multitu_symbol_partition_crc_bridge",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave3_multitu_symbol_partition_crc_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave3_multitu_symbol_partition_crc_bridge_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave3_multitu_symbol_partition_crc_bridge_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave3_multitu_symbol_partition_crc_bridge_aux.c",
        ),
        note="axis1 wave3: multi-TU symbol partition CRC bridge should match clang runtime behavior",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave4_multitu_symbol_weight_fold_hash",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave4_multitu_symbol_weight_fold_hash_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave4_multitu_symbol_weight_fold_hash_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave4_multitu_symbol_weight_fold_hash_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave4_multitu_symbol_weight_fold_hash_aux.c",
        ),
        note="axis1 wave4: multi-TU symbol-weight fold hash should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave5_multitu_symbol_owner_partition_reloc_chain",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave5_multitu_symbol_owner_partition_reloc_chain_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave5_multitu_symbol_owner_partition_reloc_chain_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave5_multitu_symbol_owner_partition_reloc_chain_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave5_multitu_symbol_owner_partition_reloc_chain_aux.c",
        ),
        note="axis1 wave5: multi-TU symbol-owner partition relocation chain should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave6_multitu_owner_hop_rebase_hash",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_aux.c",
        ),
        note="axis1 wave6: owner-hop pointer-indirection relocation hash should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_reduced",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_reduced_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_reduced_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_reduced_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_reduced_aux.c",
        ),
        note="axis1 wave6 reduced: minimal owner-hop pointer-indirection chain should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_aux.c",
        ),
        note="axis1 wave7: owner-hop permutation depth hash over two-level pointer tables should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_current",
        source=PROBE_DIR
        / "runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_current_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_current_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_current_lib.c",
            PROBE_DIR
            / "runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_current_aux.c",
        ),
        note="axis1 wave7 current-threshold: single-lane owner-hop chain remains clang-parity stable while strict permutation-depth lane is blocked",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave8_multitu_owner_hop_routeptr_offset_depth_hash",
        source=PROBE_DIR
        / "runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_offset_depth_hash_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_offset_depth_hash_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_offset_depth_hash_lib.c",
            PROBE_DIR
            / "runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_offset_depth_hash_aux.c",
        ),
        note="axis1 wave8: offset-heavy owner-hop route-pointer depth hash should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave8_multitu_owner_hop_routeptr_nooffset_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_nooffset_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_nooffset_matrix_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_nooffset_matrix_lib.c",
            PROBE_DIR
            / "runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_nooffset_matrix_aux.c",
        ),
        note="axis1 wave8: no-offset owner-hop route-pointer matrix should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave9_multitu_owner_ring_route_hash",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave9_multitu_owner_ring_route_hash_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave9_multitu_owner_ring_route_hash_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave9_multitu_owner_ring_route_hash_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave9_multitu_owner_ring_route_hash_aux.c",
        ),
        note="axis1 wave9: owner-ring route hash over nested pointer tables should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave10_multitu_owner_checkpoint_route_hash",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave10_multitu_owner_checkpoint_route_hash_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave10_multitu_owner_checkpoint_route_hash_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave10_multitu_owner_checkpoint_route_hash_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave10_multitu_owner_checkpoint_route_hash_aux.c",
        ),
        note="axis1 wave10: owner-checkpoint route hash over partitioned plans should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave11_multitu_ptrtable_nested_decay_route_hash",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave11_multitu_ptrtable_nested_decay_route_hash_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave11_multitu_ptrtable_nested_decay_route_hash_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave11_multitu_ptrtable_nested_decay_route_hash_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave11_multitu_ptrtable_nested_decay_route_hash_aux.c",
        ),
        note="axis1 wave11: nested decay pointer-table route hash should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave12_multitu_ptrtable_linkorder_checkpoint_hash",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave12_multitu_ptrtable_linkorder_checkpoint_hash_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave12_multitu_ptrtable_linkorder_checkpoint_hash_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave12_multitu_ptrtable_linkorder_checkpoint_hash_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave12_multitu_ptrtable_linkorder_checkpoint_hash_aux.c",
        ),
        note="axis1 wave12: link-order checkpoint pointer-table hash should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave13_multitu_ptrtable_four_level_route_hash",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave13_multitu_ptrtable_four_level_route_hash_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave13_multitu_ptrtable_four_level_route_hash_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave13_multitu_ptrtable_four_level_route_hash_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave13_multitu_ptrtable_four_level_route_hash_aux.c",
        ),
        note="axis1 wave13: four-level pointer-table route hash should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave14_multitu_ptrtable_relocation_order_hash",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave14_multitu_ptrtable_relocation_order_hash_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave14_multitu_ptrtable_relocation_order_hash_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave14_multitu_ptrtable_relocation_order_hash_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave14_multitu_ptrtable_relocation_order_hash_aux.c",
        ),
        note="axis1 wave14: relocation-order checkpoint hash should remain clang-parity stable",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis1_wave15_multitu_tu_order_flip_route_hash",
        source=PROBE_DIR / "runtime/15__probe_axis1_wave15_multitu_tu_order_flip_route_hash_main.c",
        inputs=(
            PROBE_DIR / "runtime/15__probe_axis1_wave15_multitu_tu_order_flip_route_hash_main.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave15_multitu_tu_order_flip_route_hash_lib.c",
            PROBE_DIR / "runtime/15__probe_axis1_wave15_multitu_tu_order_flip_route_hash_aux.c",
        ),
        note="axis1 wave15: TU-order flip route hash should remain clang-parity stable",
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
        probe_id="15__probe_axis3_wave1_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave1_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave1_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave1_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_edge_lib.c",
        ),
        note="axis3 wave1: multi-TU variadic struct-return edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave1_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave1_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_edge.c",
        note="axis3 wave1: long-double reg/stack callchain edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_permute_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_permute_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_permute_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_permute_edge_lib.c",
        ),
        note="axis3 wave2: multi-TU variadic struct-return permutation edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_permute_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_permute_edge.c",
        note="axis3 wave2: long-double reg/stack callchain permutation edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave3_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_crossmix_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave3_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_crossmix_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave3_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_crossmix_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave3_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_crossmix_edge_lib.c",
        ),
        note="axis3 wave3: multi-TU variadic struct-return crossmix edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave3_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_crossmix_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave3_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_crossmix_edge.c",
        note="axis3 wave3: long-double reg/stack callchain crossmix edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave4_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_pressure_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave4_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_pressure_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave4_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_pressure_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave4_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_pressure_edge_lib.c",
        ),
        note="axis3 wave4: multi-TU variadic struct-return reg/stack pressure edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave4_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_regstack_pressure_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave4_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_regstack_pressure_edge.c",
        note="axis3 wave4: long-double reg/stack callchain pressure edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave5_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_rotation_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave5_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_rotation_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave5_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_rotation_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave5_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_rotation_edge_lib.c",
        ),
        note="axis3 wave5: multi-TU variadic struct-return checkpoint-rotation edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave5_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_checkpoint_rotation_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave5_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_checkpoint_rotation_edge.c",
        note="axis3 wave5: long-double reg/stack callchain checkpoint-rotation edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave6_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_frontier_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave6_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_frontier_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave6_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_frontier_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave6_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_frontier_edge_lib.c",
        ),
        note="axis3 wave6: multi-TU variadic struct-return epoch/frontier edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave6_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_epoch_frontier_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave6_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_epoch_frontier_edge.c",
        note="axis3 wave6: long-double reg/stack callchain epoch/frontier edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave7_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_replay_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave7_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_replay_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave7_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_replay_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave7_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_replay_edge_lib.c",
        ),
        note="axis3 wave7: multi-TU variadic struct-return frontier/replay edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave7_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_frontier_replay_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave7_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_frontier_replay_edge.c",
        note="axis3 wave7: long-double reg/stack callchain frontier/replay edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave8_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_handoff_permute_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave8_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_handoff_permute_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave8_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_handoff_permute_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave8_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_handoff_permute_edge_lib.c",
        ),
        note="axis3 wave8: multi-TU variadic struct-return handoff/permutation edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave8_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_handoff_permute_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave8_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_handoff_permute_edge.c",
        note="axis3 wave8: long-double reg/stack callchain handoff/permutation edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave9_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_handoff_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave9_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_handoff_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave9_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_handoff_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave9_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_handoff_edge_lib.c",
        ),
        note="axis3 wave9: multi-TU variadic struct-return epoch/handoff edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave9_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_epoch_handoff_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave9_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_epoch_handoff_edge.c",
        note="axis3 wave9: long-double reg/stack callchain epoch/handoff edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave10_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave10_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave10_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_fold_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave10_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_fold_edge_lib.c",
        ),
        note="axis3 wave10: multi-TU variadic struct-return frontier/fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave10_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_frontier_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave10_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_frontier_fold_edge.c",
        note="axis3 wave10: long-double reg/stack callchain frontier/fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave11_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave11_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave11_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_fold_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave11_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_fold_edge_lib.c",
        ),
        note="axis3 wave11: multi-TU variadic struct-return checkpoint/fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave11_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_checkpoint_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave11_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_checkpoint_fold_edge.c",
        note="axis3 wave11: long-double reg/stack callchain checkpoint/fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave12_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_rotation_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave12_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_rotation_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave12_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_rotation_fold_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave12_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_rotation_fold_edge_lib.c",
        ),
        note="axis3 wave12: multi-TU variadic struct-return rotation/fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave12_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_rotation_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave12_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_rotation_fold_edge.c",
        note="axis3 wave12: long-double reg/stack callchain rotation/fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave13_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_rotation_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave13_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_rotation_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave13_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_rotation_fold_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave13_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_rotation_fold_edge_lib.c",
        ),
        note="axis3 wave13: multi-TU variadic struct-return epoch-rotation/fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave13_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_epoch_rotation_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave13_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_epoch_rotation_fold_edge.c",
        note="axis3 wave13: long-double reg/stack callchain epoch-rotation/fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave14_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_epoch_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave14_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_epoch_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave14_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_epoch_fold_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave14_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_epoch_fold_edge_lib.c",
        ),
        note="axis3 wave14: multi-TU variadic struct-return frontier-epoch/fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave14_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_frontier_epoch_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave14_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_frontier_epoch_fold_edge.c",
        note="axis3 wave14: long-double reg/stack callchain frontier-epoch/fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave15_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_frontier_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave15_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_frontier_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave15_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_frontier_fold_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave15_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_frontier_fold_edge_lib.c",
        ),
        note="axis3 wave15: multi-TU variadic struct-return regstack/frontier fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave15_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_regwindow_frontier_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave15_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_regwindow_frontier_fold_edge.c",
        note="axis3 wave15: long-double reg/stack callchain regwindow/frontier fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave16_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regwindow_handoff_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave16_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regwindow_handoff_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave16_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regwindow_handoff_fold_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave16_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regwindow_handoff_fold_edge_lib.c",
        ),
        note="axis3 wave16: multi-TU variadic struct-return regwindow/handoff fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave16_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_spill_handoff_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave16_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_spill_handoff_fold_edge.c",
        note="axis3 wave16: long-double reg/stack callchain spill/handoff fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave17_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_spill_checkpoint_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave17_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_spill_checkpoint_fold_edge_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave17_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_spill_checkpoint_fold_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave17_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_spill_checkpoint_fold_edge_lib.c",
        ),
        note="axis3 wave17: multi-TU variadic struct-return spill/checkpoint fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis3_wave17_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_spill_checkpoint_fold_edge",
        source=PROBE_DIR
        / "runtime/15__probe_axis3_wave17_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_spill_checkpoint_fold_edge.c",
        note="axis3 wave17: long-double reg/stack callchain spill/checkpoint fold edge lane should match both clang and gcc when gcc is available",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave1_reducer_signature_preservation_matrix",
        source=PROBE_DIR / "runtime/15__probe_axis5_wave1_reducer_signature_preservation_matrix.c",
        note="axis5 wave1: reducer signature-preservation matrix should keep canonical and encoded reduction signatures identical",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave1_reducer_minimization_idempotence_matrix",
        source=PROBE_DIR / "runtime/15__probe_axis5_wave1_reducer_minimization_idempotence_matrix.c",
        note="axis5 wave1: reducer minimization-idempotence matrix should keep first-pass and second-pass reduced signatures identical",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave1_reducer_cross_tu_signature_stability_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave1_reducer_cross_tu_signature_stability_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis5_wave1_reducer_cross_tu_signature_stability_matrix_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis5_wave1_reducer_cross_tu_signature_stability_matrix_lib.c",
        ),
        note="axis5 wave1: reducer cross-TU signature-stability matrix should keep deterministic nonzero signatures across full and partitioned folds",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave2_reducer_permutation_canonicalization_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave2_reducer_permutation_canonicalization_matrix.c",
        note="axis5 wave2: reducer permutation-canonicalization matrix should keep canonical signatures invariant across equivalent permutations",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave2_reducer_replay_window_stability_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave2_reducer_replay_window_stability_matrix.c",
        note="axis5 wave2: reducer replay-window stability matrix should preserve final reducer signature across chunked replay with state encode/decode",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave2_reducer_cross_tu_repartition_invariance_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave2_reducer_cross_tu_repartition_invariance_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis5_wave2_reducer_cross_tu_repartition_invariance_matrix_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis5_wave2_reducer_cross_tu_repartition_invariance_matrix_lib.c",
        ),
        note="axis5 wave2: reducer cross-TU repartition invariance matrix should keep full-fold and partitioned-fold signatures identical",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave3_reducer_delta_roundtrip_invariance_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave3_reducer_delta_roundtrip_invariance_matrix.c",
        note="axis5 wave3: reducer delta roundtrip invariance matrix should preserve signature after encode/decode reconstruction",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave3_reducer_shard_fold_associativity_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave3_reducer_shard_fold_associativity_matrix.c",
        note="axis5 wave3: reducer shard-fold associativity matrix should preserve signatures across equivalent merge orderings",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave3_reducer_cross_tu_checkpoint_resume_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave3_reducer_cross_tu_checkpoint_resume_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis5_wave3_reducer_cross_tu_checkpoint_resume_matrix_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis5_wave3_reducer_cross_tu_checkpoint_resume_matrix_lib.c",
        ),
        note="axis5 wave3: reducer cross-TU checkpoint/resume matrix should preserve signatures across checkpoint serialization boundaries",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave4_reducer_cancel_pair_confluence_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave4_reducer_cancel_pair_confluence_matrix.c",
        note="axis5 wave4: reducer cancel-pair confluence matrix should preserve normal-form signatures across opposing elimination directions",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave4_reducer_tombstone_compaction_invariance_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave4_reducer_tombstone_compaction_invariance_matrix.c",
        note="axis5 wave4: reducer tombstone-compaction invariance matrix should preserve live-set signatures before and after compaction",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave4_reducer_cross_tu_epoch_frontier_replay_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave4_reducer_cross_tu_epoch_frontier_replay_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis5_wave4_reducer_cross_tu_epoch_frontier_replay_matrix_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis5_wave4_reducer_cross_tu_epoch_frontier_replay_matrix_lib.c",
        ),
        note="axis5 wave4: reducer cross-TU epoch-frontier replay matrix should preserve signatures across exported frontier replay boundaries",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave5_reducer_phase_order_convergence_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave5_reducer_phase_order_convergence_matrix.c",
        note="axis5 wave5: reducer phase-order convergence matrix should preserve signatures across equivalent normalization pipelines",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave5_reducer_neutral_element_elision_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave5_reducer_neutral_element_elision_matrix.c",
        note="axis5 wave5: reducer neutral-element elision matrix should preserve signatures when no-op and zero-payload elements are present",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave5_reducer_cross_tu_speculative_rollback_frontier_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave5_reducer_cross_tu_speculative_rollback_frontier_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis5_wave5_reducer_cross_tu_speculative_rollback_frontier_matrix_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis5_wave5_reducer_cross_tu_speculative_rollback_frontier_matrix_lib.c",
        ),
        note="axis5 wave5: reducer cross-TU speculative-rollback frontier matrix should preserve signatures across rollback boundary export/import",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave6_reducer_window_rotation_hash_invariance_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave6_reducer_window_rotation_hash_invariance_matrix.c",
        note="axis5 wave6: reducer window-rotation hash invariance matrix should preserve canonical signatures under storage rotation with adjusted head offsets",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave6_reducer_eager_lazy_prune_equivalence_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave6_reducer_eager_lazy_prune_equivalence_matrix.c",
        note="axis5 wave6: reducer eager-vs-lazy prune equivalence matrix should preserve signatures across equivalent zero-elision schedules",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave6_reducer_cross_tu_shard_checkpoint_stitch_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave6_reducer_cross_tu_shard_checkpoint_stitch_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis5_wave6_reducer_cross_tu_shard_checkpoint_stitch_matrix_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis5_wave6_reducer_cross_tu_shard_checkpoint_stitch_matrix_lib.c",
        ),
        note="axis5 wave6: reducer cross-TU shard-checkpoint stitch matrix should preserve signatures after per-shard encode/decode and merge",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave7_reducer_alpha_renaming_equivalence_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave7_reducer_alpha_renaming_equivalence_matrix.c",
        note="axis5 wave7: reducer alpha-renaming equivalence matrix should preserve multiset signatures under key relabeling",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave7_reducer_chunk_boundary_decode_invariance_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave7_reducer_chunk_boundary_decode_invariance_matrix.c",
        note="axis5 wave7: reducer chunk-boundary decode invariance matrix should preserve signatures across run decoding chunk schedules",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave7_reducer_cross_tu_frontier_join_commutativity_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave7_reducer_cross_tu_frontier_join_commutativity_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis5_wave7_reducer_cross_tu_frontier_join_commutativity_matrix_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis5_wave7_reducer_cross_tu_frontier_join_commutativity_matrix_lib.c",
        ),
        note="axis5 wave7: reducer cross-TU frontier-join commutativity matrix should preserve signatures across merge orderings after frontier encode/decode",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave8_reducer_lane_projection_fold_equivalence_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave8_reducer_lane_projection_fold_equivalence_matrix.c",
        note="axis5 wave8: reducer lane-projection fold equivalence matrix should preserve signatures across direct and projected fold pipelines",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave8_reducer_snapshot_double_encode_idempotence_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave8_reducer_snapshot_double_encode_idempotence_matrix.c",
        note="axis5 wave8: reducer snapshot double-encode idempotence matrix should preserve signatures across repeated encode/decode passes",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave8_reducer_cross_tu_frontier_roundtrip_associativity_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave8_reducer_cross_tu_frontier_roundtrip_associativity_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis5_wave8_reducer_cross_tu_frontier_roundtrip_associativity_matrix_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis5_wave8_reducer_cross_tu_frontier_roundtrip_associativity_matrix_lib.c",
        ),
        note="axis5 wave8: reducer cross-TU frontier roundtrip associativity matrix should preserve signatures across frontier roundtrip and merge ordering",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave9_reducer_sparse_dense_materialization_equivalence_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave9_reducer_sparse_dense_materialization_equivalence_matrix.c",
        note="axis5 wave9: reducer sparse-vs-dense materialization equivalence matrix should preserve signatures across equivalent accumulator layouts",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave9_reducer_chunked_prefix_rebase_invariance_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave9_reducer_chunked_prefix_rebase_invariance_matrix.c",
        note="axis5 wave9: reducer chunked-prefix rebase invariance matrix should preserve signatures across chunked and direct prefix decodes",
    ),
    RuntimeProbe(
        probe_id="15__probe_axis5_wave9_reducer_cross_tu_checkpoint_chain_merge_invariance_matrix",
        source=PROBE_DIR
        / "runtime/15__probe_axis5_wave9_reducer_cross_tu_checkpoint_chain_merge_invariance_matrix_main.c",
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis5_wave9_reducer_cross_tu_checkpoint_chain_merge_invariance_matrix_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis5_wave9_reducer_cross_tu_checkpoint_chain_merge_invariance_matrix_lib.c",
        ),
        note="axis5 wave9: reducer cross-TU checkpoint-chain merge invariance matrix should preserve signatures after chained checkpoint decode and merge",
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
        probe_id="14__probe_multitu_static_array_window_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_static_array_window_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_static_array_window_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_static_array_window_bridge_lib.c",
        ),
        note="multi-TU static array window updates and fold checks should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_static_array_window_negative_seed_bridge",
        source=PROBE_DIR / "runtime/14__probe_multitu_static_array_window_negative_seed_bridge_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_static_array_window_negative_seed_bridge_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_static_array_window_negative_seed_bridge_lib.c",
        ),
        note="multi-TU static array window lane with negative seed rebasing should match clang",
    ),
    RuntimeProbe(
        probe_id="14__probe_multitu_static_array_window_reprime_sequence",
        source=PROBE_DIR / "runtime/14__probe_multitu_static_array_window_reprime_sequence_main.c",
        inputs=(
            PROBE_DIR / "runtime/14__probe_multitu_static_array_window_reprime_sequence_main.c",
            PROBE_DIR / "runtime/14__probe_multitu_static_array_window_reprime_sequence_lib.c",
        ),
        note="multi-TU static array reprime sequence should remain deterministic and match clang",
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
        probe_id="01__probe_line_directive_virtual_line_spelling_reject",
        source=PROBE_DIR / "diagnostics/01__probe_line_directive_virtual_line_spelling_reject.c",
        note="#line virtual filename should appear in diagnostic spelling location",
        required_substrings=("virtual_phase01_probe.c",),
    ),
    DiagnosticProbe(
        probe_id="01__probe_line_directive_virtual_macro_filename_spelling_reject",
        source=PROBE_DIR / "diagnostics/01__probe_line_directive_virtual_macro_filename_spelling_reject.c",
        note="#line macro-expanded virtual filename should appear in diagnostic spelling location",
        required_substrings=("virtual_macro_phase01_probe.c",),
    ),
    DiagnosticProbe(
        probe_id="01__probe_line_mapping_real_file_baseline",
        source=PROBE_DIR / "diagnostics/01__probe_line_mapping_real_file_baseline.c",
        note="baseline file-backed macro diagnostic should retain concrete spelling location",
        required_substrings=("01__probe_line_mapping_real_file_baseline.c",),
    ),
    DiagnosticProbe(
        probe_id="01__probe_line_directive_virtual_line_nonvoid_return_location_reject",
        source=PROBE_DIR
        / "diagnostics/01__probe_line_directive_virtual_line_nonvoid_return_location_reject.c",
        note="#line virtual line/file should propagate into non-void return diagnostic location",
        required_substrings=("Error at (322:",),
    ),
    DiagnosticProbe(
        probe_id="01__probe_line_directive_virtual_line_nonvoid_return_current_zerozero",
        source=PROBE_DIR
        / "diagnostics/01__probe_line_directive_virtual_line_nonvoid_return_current_zerozero.c",
        note="fixed baseline: non-void return diagnostic now emits remapped #line location",
        required_substrings=("Error at (322:",),
    ),
    DiagnosticProbe(
        probe_id="01__probe_line_directive_virtual_line_undeclared_identifier_location_reject",
        source=PROBE_DIR
        / "diagnostics/01__probe_line_directive_virtual_line_undeclared_identifier_location_reject.c",
        note="#line virtual line/file should propagate into undeclared-identifier semantic diagnostic",
        required_substrings=("Error at (322:",),
    ),
    DiagnosticProbe(
        probe_id="01__probe_nonvoid_return_plain_current_zerozero",
        source=PROBE_DIR / "diagnostics/01__probe_nonvoid_return_plain_current_zerozero.c",
        note="fixed baseline: non-void return diagnostic now emits plain source location",
        required_substrings=("Error at (2:",),
    ),
    DiagnosticProbe(
        probe_id="01__probe_line_directive_macro_nonvoid_return_location_reject",
        source=PROBE_DIR / "diagnostics/01__probe_line_directive_macro_nonvoid_return_location_reject.c",
        note="#line + macro-expanded return should propagate virtual location in non-void return diagnostic",
        required_substrings=("Error at (453:",),
    ),
    DiagnosticProbe(
        probe_id="01__probe_line_directive_macro_nonvoid_return_current_zerozero",
        source=PROBE_DIR / "diagnostics/01__probe_line_directive_macro_nonvoid_return_current_zerozero.c",
        note="fixed baseline: #line + macro-expanded non-void return now emits mapped location",
        required_substrings=("Error at (453:",),
    ),
    DiagnosticProbe(
        probe_id="02__probe_lexer_line_directive_invalid_dollar_location_reject",
        source=PROBE_DIR / "diagnostics/02__probe_lexer_line_directive_invalid_dollar_location_reject.c",
        note="lexer diagnostic should honor #line remap for invalid '$' token",
        required_substrings=(":831:9",),
    ),
    DiagnosticProbe(
        probe_id="02__probe_lexer_line_directive_invalid_dollar_current_physical_line",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_invalid_dollar_current_physical_line.c",
        note="fixed baseline: lexer invalid '$' diagnostic now reports remapped virtual line",
        required_substrings=(":831:9",),
    ),
    DiagnosticProbe(
        probe_id="02__probe_lexer_line_directive_unterminated_string_location_reject",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_unterminated_string_location_reject.c",
        note="lexer diagnostic should honor #line remap for unterminated string literal",
        required_substrings=(":721:21",),
    ),
    DiagnosticProbe(
        probe_id="02__probe_lexer_line_directive_unterminated_string_current_physical_line",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_unterminated_string_current_physical_line.c",
        note="fixed baseline: lexer unterminated-string diagnostic now reports remapped virtual line",
        required_substrings=(":721:21",),
    ),
    DiagnosticProbe(
        probe_id="02__probe_lexer_line_directive_char_invalid_hex_escape_location_reject",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_char_invalid_hex_escape_location_reject.c",
        note="lexer diagnostic should honor #line remap for invalid \\x escape in character literal",
        required_substrings=(":911:14",),
    ),
    DiagnosticProbe(
        probe_id="02__probe_lexer_line_directive_char_invalid_hex_escape_current_physical_line",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_char_invalid_hex_escape_current_physical_line.c",
        note="fixed baseline: lexer invalid \\x escape diagnostic now reports remapped virtual line",
        required_substrings=(":911:14",),
    ),
    DiagnosticProbe(
        probe_id="02__probe_lexer_line_directive_string_invalid_escape_location_reject",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_string_invalid_escape_location_reject.c",
        note="lexer diagnostic should honor #line remap for invalid string escape",
        required_substrings=(":941:21",),
    ),
    DiagnosticProbe(
        probe_id="02__probe_lexer_line_directive_string_invalid_escape_current_physical_line",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_string_invalid_escape_current_physical_line.c",
        note="fixed baseline: lexer invalid string-escape diagnostic now reports remapped virtual line",
        required_substrings=(":941:21",),
    ),
    DiagnosticProbe(
        probe_id="02__probe_lexer_line_directive_invalid_at_location_reject",
        source=PROBE_DIR / "diagnostics/02__probe_lexer_line_directive_invalid_at_location_reject.c",
        note="lexer diagnostic should honor #line remap for invalid '@' token",
        required_substrings=(":971:9",),
    ),
    DiagnosticProbe(
        probe_id="02__probe_lexer_line_directive_invalid_at_current_physical_line",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_invalid_at_current_physical_line.c",
        note="fixed baseline: lexer invalid '@' diagnostic now reports remapped virtual line",
        required_substrings=(":971:9",),
    ),
    DiagnosticProbe(
        probe_id="02__probe_lexer_line_directive_invalid_backtick_location_reject",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_invalid_backtick_location_reject.c",
        note="lexer diagnostic should honor #line remap for invalid '`' token",
        required_substrings=(":981:9",),
    ),
    DiagnosticProbe(
        probe_id="02__probe_lexer_line_directive_invalid_backtick_current_physical_line",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_invalid_backtick_current_physical_line.c",
        note="fixed baseline: lexer invalid '`' diagnostic now reports remapped virtual line",
        required_substrings=(":981:9",),
    ),
    DiagnosticProbe(
        probe_id="02__probe_lexer_line_directive_unterminated_char_location_reject",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_unterminated_char_location_reject.c",
        note="lexer diagnostic should honor #line remap for unterminated character literal",
        required_substrings=(":991:14",),
    ),
    DiagnosticProbe(
        probe_id="02__probe_lexer_line_directive_unterminated_char_current_physical_line",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_unterminated_char_current_physical_line.c",
        note="fixed baseline: lexer unterminated-char diagnostic now reports remapped virtual line",
        required_substrings=(":991:14",),
    ),
    DiagnosticProbe(
        probe_id="03__probe_line_directive_error_location_reject",
        source=PROBE_DIR / "diagnostics/03__probe_line_directive_error_location_reject.c",
        note="#line virtual line should propagate into #error diagnostic location",
        required_substrings=("Error at (777:",),
    ),
    DiagnosticProbe(
        probe_id="03__probe_line_directive_error_current_physical_line",
        source=PROBE_DIR / "diagnostics/03__probe_line_directive_error_current_physical_line.c",
        note="fixed baseline: #error diagnostic now reports remapped virtual line",
        required_substrings=("Error at (777:1): bucket03 - probe - error",),
    ),
    DiagnosticProbe(
        probe_id="03__probe_line_directive_error_filename_location_reject",
        source=PROBE_DIR / "diagnostics/03__probe_line_directive_error_filename_location_reject.c",
        note="#line virtual filename should propagate into #error diagnostic spelling location",
        required_substrings=("Spelling: virtual_pp_error_filename_probe.c:777:1",),
    ),
    DiagnosticProbe(
        probe_id="03__probe_line_directive_error_filename_current_physical",
        source=PROBE_DIR / "diagnostics/03__probe_line_directive_error_filename_current_physical.c",
        note="fixed baseline: #error diagnostic spelling now uses remapped virtual filename",
        required_substrings=(
            "virtual_pp_error_filename_probe_current.c:777:1",
        ),
    ),
    DiagnosticProbe(
        probe_id="03__probe_line_directive_warning_filename_location_reject",
        source=PROBE_DIR / "diagnostics/03__probe_line_directive_warning_filename_location_reject.c",
        note="#line virtual filename should propagate into #warning diagnostic spelling location",
        required_substrings=("Spelling: virtual_pp_warning_filename_probe.c:888:1",),
    ),
    DiagnosticProbe(
        probe_id="03__probe_line_directive_warning_filename_current_physical",
        source=PROBE_DIR / "diagnostics/03__probe_line_directive_warning_filename_current_physical.c",
        note="fixed baseline: #warning diagnostic spelling now uses remapped virtual filename",
        required_substrings=(
            "virtual_pp_warning_filename_probe_current.c:888:1",
        ),
    ),
    DiagnosticProbe(
        probe_id="03__probe_line_directive_pragma_stdc_filename_location_reject",
        source=PROBE_DIR
        / "diagnostics/03__probe_line_directive_pragma_stdc_filename_location_reject.c",
        note="#line virtual filename should propagate into #pragma STDC diagnostic spelling location",
        required_substrings=("Spelling: virtual_pragma_stdc_filename_probe.c:444:9",),
    ),
    DiagnosticProbe(
        probe_id="03__probe_line_directive_pragma_stdc_filename_current_physical",
        source=PROBE_DIR
        / "diagnostics/03__probe_line_directive_pragma_stdc_filename_current_physical.c",
        note="fixed baseline: #pragma STDC diagnostic spelling now uses remapped virtual filename",
        required_substrings=(
            "virtual_pragma_stdc_filename_probe_current.c:444:9",
        ),
    ),
    DiagnosticProbe(
        probe_id="03__probe_line_directive_include_error_filename_location_reject",
        source=PROBE_DIR
        / "diagnostics/03__probe_line_directive_include_error_filename_location_reject.c",
        note="#line virtual filename in included header should propagate into #error diagnostic spelling location",
        required_substrings=("Spelling: virtual_pp_include_error_header_probe.h:615:1",),
    ),
    DiagnosticProbe(
        probe_id="03__probe_line_directive_include_error_filename_current_physical",
        source=PROBE_DIR
        / "diagnostics/03__probe_line_directive_include_error_filename_current_physical.c",
        note="fixed baseline: include #error diagnostic spelling now uses remapped virtual header filename",
        required_substrings=(
            "virtual_pp_include_error_header_probe_current.h:615:1",
        ),
    ),
    DiagnosticProbe(
        probe_id="03__probe_line_directive_include_warning_filename_location_reject",
        source=PROBE_DIR
        / "diagnostics/03__probe_line_directive_include_warning_filename_location_reject.c",
        note="#line virtual filename in included header should propagate into #warning diagnostic spelling location",
        required_substrings=("Spelling: virtual_pp_include_warning_header_probe.h:616:1",),
    ),
    DiagnosticProbe(
        probe_id="03__probe_line_directive_include_warning_filename_current_physical",
        source=PROBE_DIR
        / "diagnostics/03__probe_line_directive_include_warning_filename_current_physical.c",
        note="fixed baseline: include #warning diagnostic spelling now uses remapped virtual header filename",
        required_substrings=(
            "virtual_pp_include_warning_header_probe_current.h:616:1",
        ),
    ),
    DiagnosticProbe(
        probe_id="03__probe_line_directive_include_pragma_stdc_filename_location_reject",
        source=PROBE_DIR
        / "diagnostics/03__probe_line_directive_include_pragma_stdc_filename_location_reject.c",
        note="#line virtual filename in included header should propagate into #pragma STDC diagnostic spelling location",
        required_substrings=("Spelling: virtual_pp_include_pragma_header_probe.h:617:9",),
    ),
    DiagnosticProbe(
        probe_id="03__probe_line_directive_include_pragma_stdc_filename_current_physical",
        source=PROBE_DIR
        / "diagnostics/03__probe_line_directive_include_pragma_stdc_filename_current_physical.c",
        note="fixed baseline: include #pragma STDC diagnostic spelling now uses remapped virtual header filename",
        required_substrings=(
            "virtual_pp_include_pragma_header_probe_current.h:617:9",
        ),
    ),
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
        probe_id="04__probe_deep_declarator_factory_initializer_current_reject",
        source=PROBE_DIR / "runtime/04__probe_deep_declarator_call_only.c",
        note="fixed baseline: factory() initializer path now compiles cleanly",
        required_substrings=("Semantic analysis: no issues found.",),
    ),
    DiagnosticProbe(
        probe_id="04__probe_deep_declarator_factory_assignment_current_reject",
        source=PROBE_DIR
        / "diagnostics/04__probe_deep_declarator_factory_assignment_current_reject.c",
        note="fixed baseline: factory() assignment path now compiles cleanly",
        required_substrings=("Semantic analysis: no issues found.",),
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
        probe_id="05__probe_line_directive_add_void_location_reject",
        source=PROBE_DIR / "diagnostics/05__probe_line_directive_add_void_location_reject.c",
        note="#line remapped virtual line should propagate into add-void expression diagnostic location",
        required_substrings=("Error at (831:",),
    ),
    DiagnosticProbe(
        probe_id="05__probe_line_directive_add_void_filename_location_reject",
        source=PROBE_DIR / "diagnostics/05__probe_line_directive_add_void_filename_location_reject.c",
        note="#line remapped virtual filename should propagate into add-void expression diagnostic spelling location",
        required_substrings=("Spelling: virtual_expr_add_void_filename_probe.c:832:",),
    ),
    DiagnosticProbe(
        probe_id="05__probe_line_directive_unary_minus_ptr_location_reject",
        source=PROBE_DIR / "diagnostics/05__probe_line_directive_unary_minus_ptr_location_reject.c",
        note="fixed baseline: multiline unary-minus-pointer lane reports remapped return-statement line under #line",
        required_substrings=("Error at (844:",),
    ),
    DiagnosticProbe(
        probe_id="05__probe_line_directive_unary_minus_ptr_reduced_location_pass",
        source=PROBE_DIR / "diagnostics/05__probe_line_directive_unary_minus_ptr_reduced_location_pass.c",
        note="reduced threshold: single-line unary-minus-pointer lane preserves remapped #line boundary",
        required_substrings=("Error at (841:74):",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_assign_incompatible_spelling_reject",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_assign_incompatible_spelling_reject.c",
        note="strict frontier: assignment-incompatible diagnostics should preserve #line virtual spelling filename",
        required_substrings=("Spelling: virtual_lv_assign_incompat_diag_probe.c:1244:",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_assign_incompatible_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_assign_incompatible_current_sparse_pass.c",
        note="fixed baseline: assignment-incompatible diagnostics under #line now include spelling payload",
        required_substrings=("Spelling: virtual_lv_assign_incompat_diag_probe.c:1244:",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_assign_qualifier_loss_spelling_reject",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_assign_qualifier_loss_spelling_reject.c",
        note="strict frontier: qualifier-loss assignment diagnostics should preserve #line virtual spelling filename",
        required_substrings=("Spelling: virtual_lv_assign_qual_loss_diag_probe.c:1265:",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_assign_qualifier_loss_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_assign_qualifier_loss_current_sparse_pass.c",
        note="fixed baseline: qualifier-loss assignment diagnostics under #line now include spelling payload",
        required_substrings=("Spelling: virtual_lv_assign_qual_loss_diag_probe.c:1265:",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_include_assign_incompatible_spelling_reject",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_include_assign_incompatible_spelling_reject.c",
        note="strict frontier: include-header assignment-incompatible diagnostics should preserve #line virtual spelling filename",
        required_substrings=("Spelling: virtual_lv_include_assign_incompat_diag_probe.h:1564:",),
        inputs=(
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_assign_incompatible_spelling_reject.c",
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_assign_incompatible_spelling_reject.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_include_assign_incompatible_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_include_assign_incompatible_current_sparse_pass.c",
        note="fixed baseline: include-header assignment-incompatible diagnostics under #line now include spelling payload",
        required_substrings=("Spelling: virtual_lv_include_assign_incompat_diag_probe.h:1564:",),
        inputs=(
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_assign_incompatible_current_sparse_pass.c",
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_assign_incompatible_current_sparse_pass.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_include_assign_qualifier_loss_spelling_reject",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_include_assign_qualifier_loss_spelling_reject.c",
        note="strict frontier: include-header qualifier-loss assignment diagnostics should preserve #line virtual spelling filename",
        required_substrings=("Spelling: virtual_lv_include_assign_qual_loss_diag_probe.h:1585:",),
        inputs=(
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_assign_qualifier_loss_spelling_reject.c",
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_assign_qualifier_loss_spelling_reject.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_include_assign_qualifier_loss_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_include_assign_qualifier_loss_current_sparse_pass.c",
        note="fixed baseline: include-header qualifier-loss assignment diagnostics under #line now include spelling payload",
        required_substrings=("Spelling: virtual_lv_include_assign_qual_loss_diag_probe.h:1585:",),
        inputs=(
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_assign_qualifier_loss_current_sparse_pass.c",
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_assign_qualifier_loss_current_sparse_pass.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_nonmodifiable_lvalue_spelling_reject",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_nonmodifiable_lvalue_spelling_reject.c",
        note="strict frontier: nonmodifiable-lvalue assignment diagnostics should preserve #line virtual spelling filename",
        required_substrings=("Spelling: virtual_lv_nonmodifiable_assign_diag_probe.c:1803:",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_nonmodifiable_lvalue_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_nonmodifiable_lvalue_current_sparse_pass.c",
        note="fixed baseline: nonmodifiable-lvalue assignment diagnostics under #line now include spelling payload",
        required_substrings=("Spelling: virtual_lv_nonmodifiable_assign_diag_probe.c:1803:",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_include_nonmodifiable_lvalue_spelling_reject",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_include_nonmodifiable_lvalue_spelling_reject.c",
        note="strict frontier: include-header nonmodifiable-lvalue assignment diagnostics should preserve #line virtual spelling filename",
        required_substrings=("Spelling: virtual_lv_include_nonmodifiable_assign_diag_probe.h:1823:",),
        inputs=(
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_nonmodifiable_lvalue_spelling_reject.c",
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_nonmodifiable_lvalue_spelling_reject.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_include_nonmodifiable_lvalue_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_include_nonmodifiable_lvalue_current_sparse_pass.c",
        note="fixed baseline: include-header nonmodifiable-lvalue assignment diagnostics under #line now include spelling payload",
        required_substrings=("Spelling: virtual_lv_include_nonmodifiable_assign_diag_probe.h:1823:",),
        inputs=(
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_nonmodifiable_lvalue_current_sparse_pass.c",
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_nonmodifiable_lvalue_current_sparse_pass.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_bitfield_address_spelling_reject",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_bitfield_address_spelling_reject.c",
        note="strict frontier: bitfield address-of diagnostics should preserve #line virtual spelling filename",
        required_substrings=("Spelling: virtual_lv_bitfield_address_diag_probe.c:2067:",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_bitfield_address_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_bitfield_address_spelling_reject.c",
        note="current threshold: bitfield address-of diagnostics under #line include spelling payload",
        required_substrings=("Spelling: virtual_lv_bitfield_address_diag_probe.c:2067:",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_include_bitfield_address_spelling_reject",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_include_bitfield_address_spelling_reject.c",
        note="strict frontier: include-header bitfield address-of diagnostics should preserve #line virtual spelling filename",
        required_substrings=("Spelling: virtual_lv_include_bitfield_address_diag_probe.h:2077:",),
        inputs=(
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_bitfield_address_spelling_reject.c",
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_bitfield_address_spelling_reject.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_include_bitfield_address_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_include_bitfield_address_spelling_reject.c",
        note="current threshold: include-header bitfield address-of diagnostics under #line include spelling payload",
        required_substrings=("Spelling: virtual_lv_include_bitfield_address_diag_probe.h:2077:",),
        inputs=(
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_bitfield_address_spelling_reject.c",
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_bitfield_address_spelling_reject.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_temp_increment_spelling_reject",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_temp_increment_spelling_reject.c",
        note="strict frontier: temporary increment diagnostics should preserve #line virtual spelling filename",
        required_substrings=("Spelling: virtual_lv_temp_increment_diag_probe.c:2082:",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_temp_increment_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_temp_increment_spelling_reject.c",
        note="current threshold: temporary increment diagnostics under #line include spelling payload",
        required_substrings=("Spelling: virtual_lv_temp_increment_diag_probe.c:2082:",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_include_temp_increment_spelling_reject",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_include_temp_increment_spelling_reject.c",
        note="strict frontier: include-header temporary increment diagnostics should preserve #line virtual spelling filename",
        required_substrings=("Spelling: virtual_lv_include_temp_increment_diag_probe.h:2092:",),
        inputs=(
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_temp_increment_spelling_reject.c",
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_temp_increment_spelling_reject.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_include_temp_increment_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_include_temp_increment_spelling_reject.c",
        note="current threshold: include-header temporary increment diagnostics under #line include spelling payload",
        required_substrings=("Spelling: virtual_lv_include_temp_increment_diag_probe.h:2092:",),
        inputs=(
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_temp_increment_spelling_reject.c",
            PROBE_DIR / "diagnostics/06__probe_line_directive_include_temp_increment_spelling_reject.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_compound_assign_pointer_plus_pointer_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/06__probe_line_directive_compound_assign_pointer_plus_pointer_spelling_reject.c",
        note="strict frontier: pointer-plus-pointer compound-assignment diagnostics should preserve #line virtual spelling filename",
        required_substrings=("Spelling: virtual_lv_compound_assign_pointer_plus_pointer_diag_probe.c:2246:",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_compound_assign_pointer_plus_pointer_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/06__probe_line_directive_compound_assign_pointer_plus_pointer_spelling_reject.c",
        note="reduced threshold: pointer-plus-pointer compound-assignment diagnostics should emit pointer-arithmetic rejection",
        required_substrings=("Operator '+' requires pointer arithmetic",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_include_compound_assign_pointer_plus_pointer_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/06__probe_line_directive_include_compound_assign_pointer_plus_pointer_spelling_reject.c",
        note="strict frontier: include-header pointer-plus-pointer compound-assignment diagnostics should preserve #line virtual spelling filename",
        required_substrings=(
            "Spelling: virtual_lv_include_compound_assign_pointer_plus_pointer_diag_probe.h:2256:",
        ),
        inputs=(
            PROBE_DIR
            / "diagnostics/06__probe_line_directive_include_compound_assign_pointer_plus_pointer_spelling_reject.c",
            PROBE_DIR
            / "diagnostics/06__probe_line_directive_include_compound_assign_pointer_plus_pointer_spelling_reject.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_include_compound_assign_pointer_plus_pointer_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/06__probe_line_directive_include_compound_assign_pointer_plus_pointer_spelling_reject.c",
        note="reduced threshold: include-header pointer-plus-pointer compound-assignment diagnostics should emit pointer-arithmetic rejection",
        required_substrings=("Operator '+' requires pointer arithmetic",),
        inputs=(
            PROBE_DIR
            / "diagnostics/06__probe_line_directive_include_compound_assign_pointer_plus_pointer_spelling_reject.c",
            PROBE_DIR
            / "diagnostics/06__probe_line_directive_include_compound_assign_pointer_plus_pointer_spelling_reject.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_compound_assign_const_lvalue_spelling_reject",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_compound_assign_const_lvalue_spelling_reject.c",
        note="strict frontier: const-lvalue compound-assignment diagnostics should preserve #line virtual spelling filename",
        required_substrings=("Spelling: virtual_lv_compound_assign_const_lvalue_diag_probe.c:2264:",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_compound_assign_const_lvalue_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_line_directive_compound_assign_const_lvalue_spelling_reject.c",
        note="reduced threshold: const-lvalue compound-assignment diagnostics should emit nonmodifiable-lvalue rejection",
        required_substrings=("Left operand of '+=' must be a modifiable lvalue",),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_include_compound_assign_const_lvalue_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/06__probe_line_directive_include_compound_assign_const_lvalue_spelling_reject.c",
        note="strict frontier: include-header const-lvalue compound-assignment diagnostics should preserve #line virtual spelling filename",
        required_substrings=("Spelling: virtual_lv_include_compound_assign_const_lvalue_diag_probe.h:2274:",),
        inputs=(
            PROBE_DIR
            / "diagnostics/06__probe_line_directive_include_compound_assign_const_lvalue_spelling_reject.c",
            PROBE_DIR
            / "diagnostics/06__probe_line_directive_include_compound_assign_const_lvalue_spelling_reject.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="06__probe_line_directive_include_compound_assign_const_lvalue_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/06__probe_line_directive_include_compound_assign_const_lvalue_spelling_reject.c",
        note="reduced threshold: include-header const-lvalue compound-assignment diagnostics should emit nonmodifiable-lvalue rejection",
        required_substrings=("Left operand of '+=' must be a modifiable lvalue",),
        inputs=(
            PROBE_DIR
            / "diagnostics/06__probe_line_directive_include_compound_assign_const_lvalue_spelling_reject.c",
            PROBE_DIR
            / "diagnostics/06__probe_line_directive_include_compound_assign_const_lvalue_spelling_reject.h",
        ),
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
        probe_id="07__probe_conv_pointer_assign_float_reject",
        source=PROBE_DIR / "diagnostics/07__probe_conv_pointer_assign_float_reject.c",
        note="pointer assignment from floating expression should be rejected",
        required_substrings=("Incompatible assignment operands",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_conv_pointer_init_struct_reject",
        source=PROBE_DIR / "diagnostics/07__probe_conv_pointer_init_struct_reject.c",
        note="pointer initialization from struct expression should be rejected",
        required_substrings=("Incompatible initializer type",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_conv_pointer_init_nonzero_int_reject",
        source=PROBE_DIR / "diagnostics/07__probe_conv_pointer_init_nonzero_int_reject.c",
        note="pointer initialization from nonzero integer constant should be rejected",
        required_substrings=("Incompatible initializer type",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_conv_assign_pointer_to_int_reject",
        source=PROBE_DIR / "diagnostics/07__probe_conv_assign_pointer_to_int_reject.c",
        note="assignment from pointer expression to integer object should be rejected",
        required_substrings=("Incompatible assignment operands",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_void_pointer_arith_reject",
        source=PROBE_DIR / "diagnostics/07__probe_void_pointer_arith_reject.c",
        note="void-pointer arithmetic assignment should be rejected with operator and assignment diagnostics",
        required_substrings=("pointer arithmetic on void*", "Incompatible assignment operands"),
    ),
    DiagnosticProbe(
        probe_id="07__probe_agg_union_missing_nested_field_reject",
        source=PROBE_DIR / "diagnostics/07__probe_agg_union_missing_nested_field_reject.c",
        note="nested aggregate member access should reject missing struct field within union variant",
        required_substrings=("Unknown field in member access",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_agg_union_arrow_nonptr_reject",
        source=PROBE_DIR / "diagnostics/07__probe_agg_union_arrow_nonptr_reject.c",
        note="arrow operator on non-pointer aggregate member should be rejected",
        required_substrings=("pointer operand",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_cast_struct_to_int_reject",
        source=PROBE_DIR / "diagnostics/07__probe_cast_struct_to_int_reject.c",
        note="explicit cast from struct value to integer should be rejected",
        required_substrings=("Invalid cast between non-scalar types",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_cast_int_to_struct_reject",
        source=PROBE_DIR / "diagnostics/07__probe_cast_int_to_struct_reject.c",
        note="explicit cast from integer to struct value should be rejected",
        required_substrings=("Invalid cast between non-scalar types",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_agg_dot_scalar_base_reject",
        source=PROBE_DIR / "diagnostics/07__probe_agg_dot_scalar_base_reject.c",
        note="dot access on scalar expression should be rejected at semantic stage",
        required_substrings=("Operator '.' requires struct/union operand",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_agg_arrow_ptr_to_scalar_reject",
        source=PROBE_DIR / "diagnostics/07__probe_agg_arrow_ptr_to_scalar_reject.c",
        note="arrow access on pointer-to-scalar expression should be rejected at semantic stage",
        required_substrings=("Operator '->' requires struct/union operand",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_agg_dot_array_base_reject",
        source=PROBE_DIR / "diagnostics/07__probe_agg_dot_array_base_reject.c",
        note="dot access on array expression should be rejected at semantic stage",
        required_substrings=("Operator '.' requires struct/union operand",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_designator_array_index_nonconst_reject",
        source=PROBE_DIR / "diagnostics/07__probe_designator_array_index_nonconst_reject.c",
        note="array designated initializer index must be an integer constant expression",
        required_substrings=("designator index must be a constant expression",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_designator_array_index_negative_reject",
        source=PROBE_DIR / "diagnostics/07__probe_designator_array_index_negative_reject.c",
        note="array designated initializer index must reject negative constants",
        required_substrings=("designator index -1 is negative",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_designator_array_index_oob_reject",
        source=PROBE_DIR / "diagnostics/07__probe_designator_array_index_oob_reject.c",
        note="array designated initializer index must reject out-of-bounds constants",
        required_substrings=("designator index 3 is out of bounds",),
    ),
    DiagnosticProbe(
        probe_id="07__probe_designator_nested_unknown_field_reject",
        source=PROBE_DIR / "diagnostics/07__probe_designator_nested_unknown_field_reject.c",
        note="nested designated initializer should reject unknown inner-aggregate field names",
        required_substrings=("Unknown field",),
    ),
    DiagnosticProbe(
        probe_id="08__probe_designator_unknown_field_reject",
        source=PROBE_DIR / "diagnostics/08__probe_designator_unknown_field_reject.c",
        note="designated initializer should reject unknown field names",
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_designator_array_index_nonconst_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_designator_array_index_nonconst_spelling_strict.c",
        note="strict frontier: non-const designator-index text diagnostics under #line remap should preserve spelling file/line",
        required_substrings=("Spelling: virtual_init_designator_array_nonconst_probe_diag_text.c:4802:",),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_designator_array_index_nonconst_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_designator_array_index_nonconst_spelling_strict.c",
        note="reduced threshold: non-const designator-index text diagnostics should emit semantic rejection message",
        required_substrings=("designator index must be a constant expression",),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_designator_array_index_negative_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_designator_array_index_negative_spelling_strict.c",
        note="strict frontier: negative designator-index text diagnostics under #line remap should preserve spelling file/line",
        required_substrings=("Spelling: virtual_init_designator_array_negative_probe_diag_text.c:4901:",),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_designator_array_index_negative_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_designator_array_index_negative_spelling_strict.c",
        note="reduced threshold: negative designator-index text diagnostics should emit semantic rejection message",
        required_substrings=("designator index -1 is negative",),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_include_designator_array_index_nonconst_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_include_designator_array_index_nonconst_spelling_strict.c",
        note="strict frontier: include-header non-const designator-index text diagnostics under #line remap should preserve spelling file/line",
        required_substrings=("Spelling: virtual_init_include_designator_array_nonconst_probe_diag_text.h:5002:",),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_designator_array_index_nonconst_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_designator_array_index_nonconst_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_include_designator_array_index_nonconst_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_include_designator_array_index_nonconst_spelling_strict.c",
        note="reduced threshold: include-header non-const designator-index text diagnostics should emit semantic rejection message",
        required_substrings=("designator index must be a constant expression",),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_designator_array_index_nonconst_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_designator_array_index_nonconst_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_include_designator_array_index_negative_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_include_designator_array_index_negative_spelling_strict.c",
        note="strict frontier: include-header negative designator-index text diagnostics under #line remap should preserve spelling file/line",
        required_substrings=("Spelling: virtual_init_include_designator_array_negative_probe_diag_text.h:5101:",),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_designator_array_index_negative_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_designator_array_index_negative_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_include_designator_array_index_negative_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_include_designator_array_index_negative_spelling_strict.c",
        note="reduced threshold: include-header negative designator-index text diagnostics should emit semantic rejection message",
        required_substrings=("designator index -1 is negative",),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_designator_array_index_negative_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_designator_array_index_negative_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_nested_designator_array_index_nonconst_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_nested_designator_array_index_nonconst_spelling_strict.c",
        note="strict frontier: nested non-const designator-index text diagnostics under #line remap should preserve spelling file/line",
        required_substrings=("Spelling: virtual_init_nested_designator_array_nonconst_probe_diag_text.c:6402:",),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_nested_designator_array_index_nonconst_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_nested_designator_array_index_nonconst_spelling_strict.c",
        note="reduced threshold: nested non-const designator-index text diagnostics should emit semantic rejection message",
        required_substrings=("designator index must be a constant expression",),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_include_nested_designator_array_index_nonconst_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_include_nested_designator_array_index_nonconst_spelling_strict.c",
        note="strict frontier: include-header nested non-const designator-index text diagnostics under #line remap should preserve spelling file/line",
        required_substrings=("Spelling: virtual_init_include_nested_designator_array_nonconst_probe_diag_text.h:6502:",),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_nested_designator_array_index_nonconst_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_nested_designator_array_index_nonconst_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_include_nested_designator_array_index_nonconst_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_include_nested_designator_array_index_nonconst_spelling_strict.c",
        note="reduced threshold: include-header nested non-const designator-index text diagnostics should emit semantic rejection message",
        required_substrings=("designator index must be a constant expression",),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_nested_designator_array_index_nonconst_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_nested_designator_array_index_nonconst_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_nested_designator_array_index_negative_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_nested_designator_array_index_negative_spelling_strict.c",
        note="strict frontier: nested negative designator-index text diagnostics under #line remap should preserve spelling file/line",
        required_substrings=("Spelling: virtual_init_nested_designator_array_negative_probe_diag_text.c:6601:",),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_nested_designator_array_index_negative_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_nested_designator_array_index_negative_spelling_strict.c",
        note="reduced threshold: nested negative designator-index text diagnostics should emit semantic rejection message",
        required_substrings=("designator index -1 is negative",),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_include_nested_designator_array_index_negative_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_include_nested_designator_array_index_negative_spelling_strict.c",
        note="strict frontier: include-header nested negative designator-index text diagnostics under #line remap should preserve spelling file/line",
        required_substrings=("Spelling: virtual_init_include_nested_designator_array_negative_probe_diag_text.h:6701:",),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_nested_designator_array_index_negative_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_nested_designator_array_index_negative_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_include_nested_designator_array_index_negative_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_include_nested_designator_array_index_negative_spelling_strict.c",
        note="reduced threshold: include-header nested negative designator-index text diagnostics should emit semantic rejection message",
        required_substrings=("designator index -1 is negative",),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_nested_designator_array_index_negative_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_nested_designator_array_index_negative_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_flex_not_last_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_flex_not_last_spelling_strict.c",
        note="strict frontier: flex-not-last text diagnostics under #line remap should preserve spelling file/line",
        required_substrings=("Spelling: virtual_init_flex_not_last_probe_diag_text.c:5902:",),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_flex_not_last_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_flex_not_last_spelling_strict.c",
        note="reduced threshold: flex-not-last text diagnostics should emit semantic rejection message",
        required_substrings=("Flexible array member must be the last field in a struct",),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_include_flex_not_last_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_include_flex_not_last_spelling_strict.c",
        note="strict frontier: include-header flex-not-last text diagnostics under #line remap should preserve spelling file/line",
        required_substrings=("Spelling: virtual_init_include_flex_not_last_probe_diag_text.h:6002:",),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_flex_not_last_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_flex_not_last_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_include_flex_not_last_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_include_flex_not_last_spelling_strict.c",
        note="reduced threshold: include-header flex-not-last text diagnostics should emit semantic rejection message",
        required_substrings=("Flexible array member must be the last field in a struct",),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_flex_not_last_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_flex_not_last_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_union_flex_member_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_union_flex_member_spelling_strict.c",
        note="strict frontier: union flexible-member text diagnostics under #line remap should preserve spelling file/line",
        required_substrings=("Spelling: virtual_init_union_flex_member_probe_diag_text.c:6102:",),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_union_flex_member_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_union_flex_member_spelling_strict.c",
        note="reduced threshold: union flexible-member text diagnostics should emit semantic rejection message",
        required_substrings=("Flexible array members are not allowed in unions",),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_include_union_flex_member_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_include_union_flex_member_spelling_strict.c",
        note="strict frontier: include-header union flexible-member text diagnostics under #line remap should preserve spelling file/line",
        required_substrings=("Spelling: virtual_init_include_union_flex_member_probe_diag_text.h:6202:",),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_union_flex_member_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_union_flex_member_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="08__probe_diag_line_directive_include_union_flex_member_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diag_line_directive_include_union_flex_member_spelling_strict.c",
        note="reduced threshold: include-header union flexible-member text diagnostics should emit semantic rejection message",
        required_substrings=("Flexible array members are not allowed in unions",),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_union_flex_member_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/08__probe_diag_line_directive_include_union_flex_member_spelling_strict.h",
        ),
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
        probe_id="09__probe_diag_line_directive_switch_pointer_condition_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diag_line_directive_switch_pointer_condition_spelling_strict.c",
        note="strict frontier: #line pointer switch-condition text diagnostics should preserve remapped line",
        required_substrings=("Error at (7805:0): switch controlling expression must be integer",),
    ),
    DiagnosticProbe(
        probe_id="09__probe_diag_line_directive_switch_pointer_condition_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diag_line_directive_switch_pointer_condition_spelling_strict.c",
        note="reduced threshold: #line pointer switch-condition text diagnostics should emit control-type rejection",
        required_substrings=("switch controlling expression must be integer",),
    ),
    DiagnosticProbe(
        probe_id="09__probe_diag_line_directive_include_switch_pointer_condition_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diag_line_directive_include_switch_pointer_condition_spelling_strict.c",
        note="strict frontier: include-header #line pointer switch-condition text diagnostics should preserve remapped line",
        required_substrings=("Error at (7815:0): switch controlling expression must be integer",),
        inputs=(
            PROBE_DIR
            / "diagnostics/09__probe_diag_line_directive_include_switch_pointer_condition_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/09__probe_diag_line_directive_include_switch_pointer_condition_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="09__probe_diag_line_directive_include_switch_pointer_condition_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diag_line_directive_include_switch_pointer_condition_spelling_strict.c",
        note="reduced threshold: include-header #line pointer switch-condition text diagnostics should emit control-type rejection",
        required_substrings=("switch controlling expression must be integer",),
        inputs=(
            PROBE_DIR
            / "diagnostics/09__probe_diag_line_directive_include_switch_pointer_condition_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/09__probe_diag_line_directive_include_switch_pointer_condition_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="09__probe_diag_line_directive_switch_string_condition_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diag_line_directive_switch_string_condition_spelling_strict.c",
        note="strict frontier: #line string-literal switch-condition text diagnostics should preserve remapped line",
        required_substrings=("Error at (7823:0): switch controlling expression must be integer",),
    ),
    DiagnosticProbe(
        probe_id="09__probe_diag_line_directive_switch_string_condition_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diag_line_directive_switch_string_condition_spelling_strict.c",
        note="reduced threshold: #line string-literal switch-condition text diagnostics should emit control-type rejection",
        required_substrings=("switch controlling expression must be integer",),
    ),
    DiagnosticProbe(
        probe_id="09__probe_diag_line_directive_include_switch_string_condition_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diag_line_directive_include_switch_string_condition_spelling_strict.c",
        note="strict frontier: include-header #line string-literal switch-condition text diagnostics should preserve remapped line",
        required_substrings=("Error at (7833:0): switch controlling expression must be integer",),
        inputs=(
            PROBE_DIR
            / "diagnostics/09__probe_diag_line_directive_include_switch_string_condition_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/09__probe_diag_line_directive_include_switch_string_condition_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="09__probe_diag_line_directive_include_switch_string_condition_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diag_line_directive_include_switch_string_condition_spelling_strict.c",
        note="reduced threshold: include-header #line string-literal switch-condition text diagnostics should emit control-type rejection",
        required_substrings=("switch controlling expression must be integer",),
        inputs=(
            PROBE_DIR
            / "diagnostics/09__probe_diag_line_directive_include_switch_string_condition_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/09__probe_diag_line_directive_include_switch_string_condition_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="09__probe_diag_line_directive_switch_double_condition_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diag_line_directive_switch_double_condition_spelling_strict.c",
        note="strict frontier: #line double switch-condition text diagnostics should preserve remapped line",
        required_substrings=("Error at (7844:0): switch controlling expression must be integer",),
    ),
    DiagnosticProbe(
        probe_id="09__probe_diag_line_directive_switch_double_condition_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diag_line_directive_switch_double_condition_spelling_strict.c",
        note="reduced threshold: #line double switch-condition text diagnostics should emit control-type rejection",
        required_substrings=("switch controlling expression must be integer",),
    ),
    DiagnosticProbe(
        probe_id="09__probe_diag_line_directive_include_switch_double_condition_spelling_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diag_line_directive_include_switch_double_condition_spelling_strict.c",
        note="strict frontier: include-header #line double switch-condition text diagnostics should preserve remapped line",
        required_substrings=("Error at (7854:0): switch controlling expression must be integer",),
        inputs=(
            PROBE_DIR
            / "diagnostics/09__probe_diag_line_directive_include_switch_double_condition_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/09__probe_diag_line_directive_include_switch_double_condition_spelling_strict.h",
        ),
    ),
    DiagnosticProbe(
        probe_id="09__probe_diag_line_directive_include_switch_double_condition_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diag_line_directive_include_switch_double_condition_spelling_strict.c",
        note="reduced threshold: include-header #line double switch-condition text diagnostics should emit control-type rejection",
        required_substrings=("switch controlling expression must be integer",),
        inputs=(
            PROBE_DIR
            / "diagnostics/09__probe_diag_line_directive_include_switch_double_condition_spelling_strict.c",
            PROBE_DIR
            / "diagnostics/09__probe_diag_line_directive_include_switch_double_condition_spelling_strict.h",
        ),
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
        probe_id="10__probe_diag_line_directive_extern_type_mismatch_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_extern_type_mismatch_spelling_strict.c",
        note="text diagnostics should preserve remapped spelling for extern type mismatch under #line",
        required_substrings=("Spelling: virtual_scope_extern_type_mismatch_probe_diag_text.c:6402:14",),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_function_redecl_conflict_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_function_redecl_conflict_spelling_strict.c",
        note="text diagnostics should preserve remapped spelling for function redeclaration conflict under #line",
        required_substrings=(
            "Spelling: virtual_scope_function_redecl_conflict_probe_diag_text.c:6502:7",
        ),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_include_extern_type_mismatch_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_include_extern_type_mismatch_spelling_strict.c",
        note="text diagnostics should preserve remapped spelling for include-header extern type mismatch under #line",
        required_substrings=(
            "Spelling: virtual_scope_include_extern_type_mismatch_probe_diag_text.h:6602:14",
        ),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_include_function_redecl_conflict_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_include_function_redecl_conflict_spelling_strict.c",
        note="text diagnostics should preserve remapped spelling for include-header function redeclaration conflict under #line",
        required_substrings=(
            "Spelling: virtual_scope_include_function_redecl_conflict_probe_diag_text.h:6702:7",
        ),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_extern_static_mismatch_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_extern_static_mismatch_spelling_strict.c",
        note="text diagnostics should preserve remapped spelling for extern/static linkage conflict under #line",
        required_substrings=(
            "Spelling: virtual_scope_extern_static_mismatch_probe_diag_text.c:7202:12",
        ),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_tentative_static_conflict_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_tentative_static_conflict_spelling_strict.c",
        note="text diagnostics should preserve remapped spelling for tentative/static linkage conflict under #line",
        required_substrings=(
            "Spelling: virtual_scope_tentative_static_conflict_probe_diag_text.c:7302:12",
        ),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_include_extern_static_mismatch_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_include_extern_static_mismatch_spelling_strict.c",
        note="text diagnostics should preserve remapped spelling for include-header extern/static linkage conflict under #line",
        required_substrings=(
            "Spelling: virtual_scope_include_extern_static_mismatch_probe_diag_text.h:7402:12",
        ),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_include_tentative_static_conflict_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_include_tentative_static_conflict_spelling_strict.c",
        note="text diagnostics should preserve remapped spelling for include-header tentative/static linkage conflict under #line",
        required_substrings=(
            "Spelling: virtual_scope_include_tentative_static_conflict_probe_diag_text.h:7502:12",
        ),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_block_extern_different_type_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_block_extern_different_type_spelling_strict.c",
        note="text diagnostics should preserve remapped spelling for block-scope extern type-conflict under #line",
        required_substrings=(
            "Spelling: virtual_scope_block_extern_type_conflict_probe_diag_text.c:8204:18",
        ),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_include_block_extern_different_type_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_include_block_extern_different_type_spelling_strict.c",
        note="text diagnostics should preserve remapped spelling for include-header block-scope extern type-conflict under #line",
        required_substrings=(
            "Spelling: virtual_scope_include_block_extern_type_conflict_probe_diag_text.h:8304:18",
        ),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_multitu_extern_type_conflict_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_multitu_extern_type_conflict_spelling_strict_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/10__probe_diag_line_directive_multitu_extern_type_conflict_spelling_strict_main.c",
            PROBE_DIR
            / "diagnostics/10__probe_diag_line_directive_multitu_extern_type_conflict_spelling_strict_lib.c",
        ),
        note="strict frontier: multi-TU extern type-conflict should preserve source spelling under #line",
        required_substrings=(
            "Spelling: virtual_scope_multitu_extern_type_conflict_probe_lib_diag_text.c:9902:7",
        ),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_multitu_extern_type_conflict_current_linkstage_pass",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_multitu_extern_type_conflict_spelling_strict_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/10__probe_diag_line_directive_multitu_extern_type_conflict_spelling_strict_main.c",
            PROBE_DIR
            / "diagnostics/10__probe_diag_line_directive_multitu_extern_type_conflict_spelling_strict_lib.c",
        ),
        note="regression guard: multi-TU extern type-conflict should keep semantic spelling under #line",
        required_substrings=(
            "Spelling: virtual_scope_multitu_extern_type_conflict_probe_lib_diag_text.c:9902:7",
        ),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_multitu_include_extern_type_conflict_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_multitu_include_extern_type_conflict_spelling_strict_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/10__probe_diag_line_directive_multitu_include_extern_type_conflict_spelling_strict_main.c",
            PROBE_DIR
            / "diagnostics/10__probe_diag_line_directive_multitu_include_extern_type_conflict_spelling_strict_lib.c",
        ),
        note="strict frontier: multi-TU include extern type-conflict should preserve source spelling under #line",
        required_substrings=(
            "Spelling: virtual_scope_multitu_include_extern_type_conflict_probe_header_diag_text.h:10002:7",
        ),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_multitu_include_extern_type_conflict_current_linkstage_pass",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_multitu_include_extern_type_conflict_spelling_strict_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/10__probe_diag_line_directive_multitu_include_extern_type_conflict_spelling_strict_main.c",
            PROBE_DIR
            / "diagnostics/10__probe_diag_line_directive_multitu_include_extern_type_conflict_spelling_strict_lib.c",
        ),
        note="regression guard: multi-TU include extern type-conflict should keep semantic spelling under #line",
        required_substrings=(
            "Spelling: virtual_scope_multitu_include_extern_type_conflict_probe_header_diag_text.h:10002:7",
        ),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_extern_array_mismatch_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_extern_array_mismatch_spelling_strict.c",
        note="strict frontier: extern-array mismatch should emit linkage conflict diagnostics under #line remap",
        required_substrings=("Conflicting types for variable",),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_include_extern_array_mismatch_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_include_extern_array_mismatch_spelling_strict.c",
        note="strict frontier: include extern-array mismatch should emit linkage conflict diagnostics under #line remap",
        required_substrings=("Conflicting types for variable",),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_extern_array_mismatch_first_decl_line_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_extern_array_mismatch_first_decl_line_spelling_strict.c",
        note="strict frontier: extern-array mismatch with first declaration remapped by #line should emit conflict diagnostics",
        required_substrings=("Conflicting types for variable",),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_extern_array_mismatch_second_decl_line_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_extern_array_mismatch_second_decl_line_spelling_strict.c",
        note="strict frontier: extern-array mismatch with second declaration remapped by #line should emit conflict diagnostics",
        required_substrings=("Conflicting types for variable",),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_extern_array_vs_scalar_conflict_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_extern_array_vs_scalar_conflict_spelling_strict.c",
        note="control lane: extern array-vs-scalar redeclaration should still emit conflict diagnostics under #line",
        required_substrings=(
            "Spelling: virtual_scope_extern_array_vs_scalar_conflict_probe_diag_text.c:15302:12",
        ),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_extern_array_def_mismatch_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_extern_array_def_mismatch_spelling_strict.c",
        note="strict frontier: extern-array declaration/definition extent mismatch should emit conflict diagnostics under #line remap",
        required_substrings=("Conflicting types for variable",),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_include_extern_array_def_mismatch_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_include_extern_array_def_mismatch_spelling_strict.c",
        note="strict frontier: include extern-array declaration/definition extent mismatch should emit conflict diagnostics under #line remap",
        required_substrings=("Conflicting types for variable",),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_multitu_extern_array_def_mismatch_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_multitu_extern_array_def_mismatch_spelling_strict_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/10__probe_diag_line_directive_multitu_extern_array_def_mismatch_spelling_strict_main.c",
            PROBE_DIR
            / "diagnostics/10__probe_diag_line_directive_multitu_extern_array_def_mismatch_spelling_strict_lib.c",
        ),
        note="strict frontier: multi-TU extern-array declaration/definition extent mismatch should emit conflict diagnostics under #line remap",
        required_substrings=("Conflicting types for variable",),
    ),
    DiagnosticProbe(
        probe_id="10__probe_diag_line_directive_multitu_include_extern_array_def_mismatch_spelling_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_multitu_include_extern_array_def_mismatch_spelling_strict_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/10__probe_diag_line_directive_multitu_include_extern_array_def_mismatch_spelling_strict_main.c",
            PROBE_DIR
            / "diagnostics/10__probe_diag_line_directive_multitu_include_extern_array_def_mismatch_spelling_strict_lib.c",
        ),
        note="strict frontier: multi-TU include extern-array declaration/definition extent mismatch should emit conflict diagnostics under #line remap",
        required_substrings=("Conflicting types for variable",),
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
        probe_id="11__probe_diag_line_directive_nonvoid_missing_return_text_strict",
        source=PROBE_DIR
        / "diagnostics/11__probe_diag_line_directive_nonvoid_missing_return_text_strict.c",
        note="text diagnostics should preserve remapped line for #line non-void-missing-return",
        required_substrings=("Error at (7501:5): Control reaches end of non-void function 'f_text'",),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_include_nonvoid_missing_return_text_strict",
        source=PROBE_DIR
        / "diagnostics/11__probe_diag_line_directive_include_nonvoid_missing_return_text_strict.c",
        note="text diagnostics should preserve remapped line for include #line non-void-missing-return",
        required_substrings=("Error at (7601:5): Control reaches end of non-void function 'f_inc_text'",),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_too_many_args_text_strict",
        source=PROBE_DIR
        / "diagnostics/11__probe_diag_line_directive_too_many_args_text_strict.c",
        note="text diagnostics should preserve remapped line for #line prototype-too-many-args",
        required_substrings=("Error at (7706:12): Too many arguments in call to 'add1_text' (expected 1, got 2)",),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_include_too_many_args_text_strict",
        source=PROBE_DIR
        / "diagnostics/11__probe_diag_line_directive_include_too_many_args_text_strict.c",
        note="text diagnostics should preserve remapped line for include #line prototype-too-many-args",
        required_substrings=(
            "Error at (7802:34): Too many arguments in call to 'add1_inc_text' (expected 1, got 2)",
        ),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_too_few_args_text_strict",
        source=PROBE_DIR
        / "diagnostics/11__probe_diag_line_directive_too_few_args_text_strict.c",
        note="text diagnostics should preserve remapped line for #line prototype-too-few-args",
        required_substrings=("Error at (8906:12): Too few arguments in call to 'add2_text' (expected 2, got 1)",),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_include_too_few_args_text_strict",
        source=PROBE_DIR
        / "diagnostics/11__probe_diag_line_directive_include_too_few_args_text_strict.c",
        note="text diagnostics should preserve remapped line for include #line prototype-too-few-args",
        required_substrings=(
            "Error at (9002:34): Too few arguments in call to 'add2_inc_text' (expected 2, got 1)",
        ),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_argument_type_mismatch_text_strict",
        source=PROBE_DIR
        / "diagnostics/11__probe_diag_line_directive_argument_type_mismatch_text_strict.c",
        note="text diagnostics should preserve remapped line for #line argument-type-mismatch",
        required_substrings=("Error at (9106:27): Argument 1 of 'takes_ptr_text' has incompatible type",),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_include_argument_type_mismatch_text_strict",
        source=PROBE_DIR
        / "diagnostics/11__probe_diag_line_directive_include_argument_type_mismatch_text_strict.c",
        note="text diagnostics should preserve remapped line for include #line argument-type-mismatch",
        required_substrings=("Error at (9202:53): Argument 1 of 'takes_ptr_inc_text' has incompatible type",),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_return_type_mismatch_text_strict",
        source=PROBE_DIR
        / "diagnostics/11__probe_diag_line_directive_return_type_mismatch_text_strict.c",
        note="text diagnostics should preserve remapped line for #line return-type-mismatch",
        required_substrings=("Error at (10102:",),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_include_return_type_mismatch_text_strict",
        source=PROBE_DIR
        / "diagnostics/11__probe_diag_line_directive_include_return_type_mismatch_text_strict.c",
        note="text diagnostics should preserve remapped line for include #line return-type-mismatch",
        required_substrings=("Error at (10201:",),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_void_return_value_text_strict",
        source=PROBE_DIR
        / "diagnostics/11__probe_diag_line_directive_void_return_value_text_strict.c",
        note="text diagnostics should preserve remapped line for #line void-return-value rejection",
        required_substrings=("Error at (10302:",),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_include_void_return_value_text_strict",
        source=PROBE_DIR
        / "diagnostics/11__probe_diag_line_directive_include_void_return_value_text_strict.c",
        note="text diagnostics should preserve remapped line for include #line void-return-value rejection",
        required_substrings=("Error at (10401:",),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_multitu_too_many_args_text_threshold_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_multitu_too_many_args_file_presence.c",
        note="text parity guard: multi-TU #line too-many-args remains stable in text diagnostics",
        required_substrings=(
            "Error at (13304:12): Too many arguments in call to 'probe_mt_add1' (expected 1, got 2)",
        ),
        inputs=(
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_too_many_args_file_presence.c",
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_too_many_args_file_presence_lib.c",
        ),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_multitu_include_too_many_args_text_threshold_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_multitu_include_too_many_args_file_presence.c",
        note="text parity guard: multi-TU include #line too-many-args remains stable in text diagnostics",
        required_substrings=(
            "Error at (13402:49): Too many arguments in call to 'probe_mt_add1_inc' (expected 1, got 2)",
        ),
        inputs=(
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_include_too_many_args_file_presence.c",
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_include_too_many_args_file_presence_lib.c",
        ),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_multitu_parserdiag_decl_missing_rparen_text_threshold_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_multitu_parserdiag_decl_missing_rparen_presence.c",
        note="text parity guard: multi-TU #line parser declarator recovery remains stable in text diagnostics",
        required_substrings=(
            "virtual_probe_fn_multitu_parserdiag_decl_missing_rparen_main.c:13501",
        ),
        inputs=(
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_parserdiag_decl_missing_rparen_presence.c",
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_parserdiag_decl_missing_rparen_presence_lib.c",
        ),
    ),
    DiagnosticProbe(
        probe_id="11__probe_diag_line_directive_multitu_include_parserdiag_decl_missing_rparen_text_threshold_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_multitu_include_parserdiag_decl_missing_rparen_presence.c",
        note="text parity guard: multi-TU include #line parser declarator recovery remains stable in text diagnostics",
        required_substrings=(
            "virtual_probe_fn_multitu_include_parserdiag_decl_missing_rparen.h:13601",
        ),
        inputs=(
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_include_parserdiag_decl_missing_rparen_presence.c",
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_include_parserdiag_decl_missing_rparen_presence_lib.c",
        ),
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
        probe_id="12__probe_diag_line_directive_for_missing_semicolon_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diag_line_directive_for_missing_semicolon_parser_text_strict.c",
        note="text diagnostics should preserve parser malformed-for-header line under #line remap",
        required_substrings=("Error: expected ';' after for-loop initializer at line 8402",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_include_for_missing_semicolon_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diag_line_directive_include_for_missing_semicolon_parser_text_strict.c",
        note="text diagnostics should preserve parser malformed-for-header line for include #line remap",
        required_substrings=("Error: expected ';' after for-loop initializer at line 8502",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_for_missing_rparen_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_for_missing_rparen_parser_presence.c",
        note="text diagnostics should preserve parser missing ')' in for header under #line remap",
        required_substrings=("Error: expected ')' after for-loop increment at line 8922",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_include_for_missing_rparen_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_for_missing_rparen_parser_presence.c",
        note="text diagnostics should preserve parser missing ')' in for header under include #line remap",
        required_substrings=("Error: expected ')' after for-loop increment at line 8932",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_if_missing_rparen_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_if_missing_rparen_parser_presence.c",
        note="text diagnostics should preserve parser missing ')' line under #line remap for if",
        required_substrings=("Error: expected ')' after condition at line 8603",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_include_if_missing_rparen_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_if_missing_rparen_parser_presence.c",
        note="text diagnostics should preserve parser missing ')' line under include #line remap for if",
        required_substrings=("Error: expected ')' after condition at line 8613",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_while_missing_lparen_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_while_missing_lparen_parser_presence.c",
        note="text diagnostics should preserve parser missing '(' line under #line remap for while",
        required_substrings=("Error: expected '(' after 'while' at line 8703",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_include_while_missing_lparen_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_while_missing_lparen_parser_presence.c",
        note="text diagnostics should preserve parser missing '(' line under include #line remap for while",
        required_substrings=("Error: expected '(' after 'while' at line 8713",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_do_while_missing_semicolon_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_do_while_missing_semicolon_parser_presence.c",
        note="text diagnostics should preserve parser missing ';' line under #line remap for do-while",
        required_substrings=("Error: expected ';' after 'do-while' statement at line 8806",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_include_do_while_missing_semicolon_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_do_while_missing_semicolon_parser_presence.c",
        note="text diagnostics should preserve parser missing ';' line under include #line remap for do-while",
        required_substrings=("Error: expected ';' after 'do-while' statement at line 8816",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_switch_missing_rparen_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_switch_missing_rparen_parser_presence.c",
        note="text diagnostics should preserve parser switch-header parse failure line under #line remap",
        required_substrings=("Error: Failed to parse expression at line 8903",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_include_switch_missing_rparen_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_switch_missing_rparen_parser_presence.c",
        note="text diagnostics should preserve parser switch-header parse failure line under include #line remap",
        required_substrings=("Error: Failed to parse expression at line 8913",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_if_missing_body_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_if_missing_body_parser_presence.c",
        note="text diagnostics should preserve parser missing-body line under #line remap for if",
        required_substrings=("line 9103",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_include_if_missing_body_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_if_missing_body_parser_presence.c",
        note="text diagnostics should preserve parser missing-body line under include #line remap for if",
        required_substrings=("line 9113",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_while_missing_body_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_while_missing_body_parser_presence.c",
        note="text diagnostics should preserve parser missing-body line under #line remap for while",
        required_substrings=("line 9203",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_include_while_missing_body_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_while_missing_body_parser_presence.c",
        note="text diagnostics should preserve parser missing-body line under include #line remap for while",
        required_substrings=("line 9213",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_for_missing_body_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_for_missing_body_parser_presence.c",
        note="text diagnostics should preserve parser missing-body line under #line remap for for",
        required_substrings=("line 9303",),
    ),
    DiagnosticProbe(
        probe_id="12__probe_diag_line_directive_include_for_missing_body_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_for_missing_body_parser_presence.c",
        note="text diagnostics should preserve parser missing-body line under include #line remap for for",
        required_substrings=("line 9313",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_line_directive_if_missing_lparen_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_if_missing_lparen_parser_presence.c",
        note="text diagnostics should preserve parser missing '(' line under #line remap for if",
        required_substrings=("Error: expected '(' after 'if' at line 132003",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_line_directive_include_if_missing_lparen_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_include_if_missing_lparen_parser_presence.c",
        note="text diagnostics should preserve parser missing '(' line under include #line remap for if",
        required_substrings=("Error: expected '(' after 'if' at line 132053",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_line_directive_switch_missing_rparen_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_switch_missing_rparen_parser_presence.c",
        note="text diagnostics should preserve parser switch-header parse failure line under #line remap",
        required_substrings=("Error: Failed to parse expression at line 132103",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_line_directive_include_switch_missing_rparen_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_include_switch_missing_rparen_parser_presence.c",
        note="text diagnostics should preserve parser switch-header parse failure line under include #line remap",
        required_substrings=("Error: Failed to parse expression at line 132153",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_line_directive_if_missing_body_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_if_missing_body_parser_presence.c",
        note="text diagnostics should preserve parser missing-body line under #line remap for if",
        required_substrings=("line 134003",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_line_directive_include_if_missing_body_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_include_if_missing_body_parser_presence.c",
        note="text diagnostics should preserve parser missing-body line under include #line remap for if",
        required_substrings=("line 134053",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_line_directive_while_missing_body_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_while_missing_body_parser_presence.c",
        note="text diagnostics should preserve parser missing-body line under #line remap for while",
        required_substrings=("line 134103",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_line_directive_include_while_missing_body_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_include_while_missing_body_parser_presence.c",
        note="text diagnostics should preserve parser missing-body line under include #line remap for while",
        required_substrings=("line 134153",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_line_directive_for_missing_body_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_for_missing_body_parser_presence.c",
        note="text diagnostics should preserve parser missing-body line under #line remap for for",
        required_substrings=("line 134203",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_line_directive_include_for_missing_body_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_include_for_missing_body_parser_presence.c",
        note="text diagnostics should preserve parser missing-body line under include #line remap for for",
        required_substrings=("line 134253",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_line_directive_switch_missing_body_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_switch_missing_body_parser_presence.c",
        note="text diagnostics should preserve parser missing-body line under #line remap for switch",
        required_substrings=("line 134303",),
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_line_directive_include_switch_missing_body_parser_text_strict",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_include_switch_missing_body_parser_presence.c",
        note="text diagnostics should preserve parser missing-body line under include #line remap for switch",
        required_substrings=("line 134353",),
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
        probe_id="13__probe_diag_parser_if_missing_lparen",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_if_missing_lparen.c",
        note="parser should emit diagnostics for missing '(' in if statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_while_missing_lparen",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_while_missing_lparen.c",
        note="parser should emit diagnostics for missing '(' in while statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_do_while_missing_lparen",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_do_while_missing_lparen.c",
        note="parser should emit diagnostics for missing '(' in do-while condition",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_for_missing_lparen",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_for_missing_lparen.c",
        note="parser should emit diagnostics for missing '(' in for statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_if_missing_rparen",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_if_missing_rparen.c",
        note="parser should emit diagnostics for missing ')' in if statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_while_missing_rparen",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_while_missing_rparen.c",
        note="parser should emit diagnostics for missing ')' in while statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_do_while_missing_rparen",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_do_while_missing_rparen.c",
        note="parser should emit diagnostics for missing ')' in do-while condition",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_for_missing_rparen",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_for_missing_rparen.c",
        note="parser should emit diagnostics for missing ')' in for statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_for_missing_first_semicolon",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_for_missing_first_semicolon.c",
        note="parser should emit diagnostics for missing first ';' in for header",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_for_missing_second_semicolon",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_for_missing_second_semicolon.c",
        note="parser should emit diagnostics for missing second ';' in for header",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_for_missing_second_semicolon_simple",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_for_missing_second_semicolon_simple.c",
        note="parser should emit diagnostics for missing second ';' in minimal for-header form",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_if_missing_condition",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_if_missing_condition.c",
        note="parser should emit diagnostics for missing condition expression in if statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_do_while_missing_while",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_do_while_missing_while.c",
        note="parser should emit diagnostics for missing 'while' in do-while statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_do_while_missing_while_simple",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_do_while_missing_while_simple.c",
        note="parser should emit diagnostics for missing 'while' in reduced do-while form",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_do_while_missing_condition",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_do_while_missing_condition.c",
        note="parser should emit diagnostics for missing condition expression in do-while statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_do_while_missing_semicolon",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_do_while_missing_semicolon.c",
        note="parser should emit diagnostics for missing ';' after do-while condition",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_for_missing_body",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_for_missing_body.c",
        note="parser should emit diagnostics for missing for-loop body statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_while_missing_body",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_while_missing_body.c",
        note="parser should emit diagnostics for missing while-loop body statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_do_while_missing_body",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_do_while_missing_body.c",
        note="parser should emit diagnostics for missing do-while loop body statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_do_while_missing_body_simple",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_do_while_missing_body_simple.c",
        note="parser should emit diagnostics for minimal missing do-while loop body statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_switch_missing_body",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_switch_missing_body.c",
        note="parser should emit diagnostics for missing switch statement body",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_switch_missing_lparen",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_switch_missing_lparen.c",
        note="parser should emit diagnostics for missing '(' in switch statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_while_empty_condition",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_while_empty_condition.c",
        note="parser should emit diagnostics for missing while condition expression",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_switch_missing_rparen",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_switch_missing_rparen.c",
        note="parser should emit diagnostics for missing ')' in switch statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_if_missing_then_stmt",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_if_missing_then_stmt.c",
        note="parser should emit diagnostics for missing then-statement in if",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_else_missing_stmt_reject",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_else_missing_stmt_reject.c",
        note="parser should emit diagnostics for missing statement in else",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_switch_missing_lbrace",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_switch_missing_lbrace.c",
        note="parser should emit diagnostics for missing '{' in switch statement",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_switch_case_missing_colon",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_switch_case_missing_colon.c",
        note="parser should emit diagnostics for missing ':' in switch case label",
    ),
    DiagnosticProbe(
        probe_id="13__probe_diag_parser_switch_default_missing_colon_reject",
        source=PROBE_DIR / "diagnostics/13__probe_diag_parser_switch_default_missing_colon.c",
        note="parser should emit diagnostics for missing ':' in switch default label",
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
        required_substrings=("linker command failed",),
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
        probe_id="15__probe_diag_pp_macro_paste_variadic_comma_extension_no_diag",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_macro_paste_fragments_no_crash.c",
        note="GNU-style ', ##__VA_ARGS__' macro-paste extension should fail closed with preprocessing diagnostics",
        required_substrings=("GNU ', ##__VA_ARGS__' extension is not supported",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_pp_macro_paste_operator_tail_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_macro_paste_operator_tail_no_crash.c",
        note="operator-tail token-paste malformed lane should fail closed with preprocessing diagnostics",
        required_substrings=("macro expansion failed (possible recursion)",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_pp_gnu_comma_vaargs_chain_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_gnu_comma_vaargs_chain_no_crash.c",
        note="GNU comma-vaargs nested chain should fail closed with preprocessing diagnostics",
        required_substrings=("GNU ', ##__VA_ARGS__' extension is not supported",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_malformed_pp_token_paste_variadic_tail_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_token_paste_variadic_tail_no_crash.c",
        note="variadic token-paste tail lane should fail closed with preprocessing diagnostics",
        required_substrings=("macro expansion failed (possible recursion)",),
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
        probe_id="15__probe_diag_axis1_wave1_multitu_duplicate_definition_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave1_multitu_duplicate_definition_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave1_multitu_duplicate_definition_reject_main.c",
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave1_multitu_duplicate_definition_reject_lib.c",
        ),
        note="axis1 wave1: multi-TU duplicate data definitions should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave2_multitu_duplicate_weak_strong_conflict_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave2_multitu_duplicate_weak_strong_conflict_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave2_multitu_duplicate_weak_strong_conflict_reject_main.c",
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave2_multitu_duplicate_weak_strong_conflict_reject_lib.c",
        ),
        note="axis1 wave2: multi-TU duplicate weak/strong conflict should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave3_multitu_duplicate_tentative_matrix_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave3_multitu_duplicate_tentative_matrix_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave3_multitu_duplicate_tentative_matrix_reject_main.c",
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave3_multitu_duplicate_tentative_matrix_reject_lib.c",
        ),
        note="axis1 wave3: multi-TU duplicate tentative-definition matrix should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave3_multitu_duplicate_tentative_matrix_current_no_diag",
        source=PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave3_multitu_duplicate_tentative_matrix_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave3_multitu_duplicate_tentative_matrix_reject_main.c",
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave3_multitu_duplicate_tentative_matrix_reject_lib.c",
        ),
        note="axis1 wave3 current-threshold: duplicate tentative-definition matrix now emits deterministic duplicate-symbol diagnostics",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave4_multitu_function_data_symbol_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave4_multitu_function_data_symbol_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave4_multitu_function_data_symbol_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave4_multitu_function_data_symbol_conflict_reject_lib.c",
        ),
        note="axis1 wave4: multi-TU function-vs-data same-name symbol conflict should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave5_multitu_duplicate_initialized_global_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave5_multitu_duplicate_initialized_global_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave5_multitu_duplicate_initialized_global_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave5_multitu_duplicate_initialized_global_reject_lib.c",
        ),
        note="axis1 wave5: multi-TU duplicate initialized globals should fail at link stage (always-hard lane)",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave5_multitu_duplicate_tentative_split_current",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave5_multitu_duplicate_tentative_split_current_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave5_multitu_duplicate_tentative_split_current_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave5_multitu_duplicate_tentative_split_current_lib.c",
        ),
        note="axis1 wave5 current-threshold: duplicate tentative split lane now emits deterministic duplicate-symbol diagnostics",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave6_multitu_function_object_alias_partition_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave6_multitu_function_object_alias_partition_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave6_multitu_function_object_alias_partition_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave6_multitu_function_object_alias_partition_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave6_multitu_function_object_alias_partition_conflict_reject_aux.c",
        ),
        note="axis1 wave6: multi-TU function-vs-object alias partition conflict should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave7_multitu_function_object_alias_perm_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave7_multitu_function_object_alias_perm_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave7_multitu_function_object_alias_perm_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave7_multitu_function_object_alias_perm_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave7_multitu_function_object_alias_perm_conflict_reject_aux.c",
        ),
        note="axis1 wave7: mixed function-vs-object alias collisions across three TU partitions should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave8_multitu_dual_function_object_alias_perm_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave8_multitu_dual_function_object_alias_perm_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave8_multitu_dual_function_object_alias_perm_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave8_multitu_dual_function_object_alias_perm_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave8_multitu_dual_function_object_alias_perm_conflict_reject_aux.c",
        ),
        note="axis1 wave8: dual mixed function-vs-object alias permutations should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave9_multitu_function_object_alias_ring_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave9_multitu_function_object_alias_ring_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave9_multitu_function_object_alias_ring_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave9_multitu_function_object_alias_ring_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave9_multitu_function_object_alias_ring_conflict_reject_aux.c",
        ),
        note="axis1 wave9: mixed function-vs-object alias ring conflict should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave10_multitu_function_object_alias_checkpoint_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave10_multitu_function_object_alias_checkpoint_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave10_multitu_function_object_alias_checkpoint_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave10_multitu_function_object_alias_checkpoint_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave10_multitu_function_object_alias_checkpoint_conflict_reject_aux.c",
        ),
        note="axis1 wave10: mixed function-vs-object alias checkpoint conflict should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave11_multitu_function_object_alias_ptrtable_rebase_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave11_multitu_function_object_alias_ptrtable_rebase_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave11_multitu_function_object_alias_ptrtable_rebase_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave11_multitu_function_object_alias_ptrtable_rebase_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave11_multitu_function_object_alias_ptrtable_rebase_conflict_reject_aux.c",
        ),
        note="axis1 wave11: mixed function-vs-object alias pointer-table rebase conflict should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave12_multitu_function_object_alias_linkorder_checkpoint_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave12_multitu_function_object_alias_linkorder_checkpoint_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave12_multitu_function_object_alias_linkorder_checkpoint_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave12_multitu_function_object_alias_linkorder_checkpoint_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave12_multitu_function_object_alias_linkorder_checkpoint_conflict_reject_aux.c",
        ),
        note="axis1 wave12: mixed function-vs-object alias link-order checkpoint conflict should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave13_multitu_function_object_alias_four_level_checkpoint_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave13_multitu_function_object_alias_four_level_checkpoint_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave13_multitu_function_object_alias_four_level_checkpoint_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave13_multitu_function_object_alias_four_level_checkpoint_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave13_multitu_function_object_alias_four_level_checkpoint_conflict_reject_aux.c",
        ),
        note="axis1 wave13: mixed function-vs-object alias four-level checkpoint conflict should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave14_multitu_function_object_alias_relocation_order_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave14_multitu_function_object_alias_relocation_order_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave14_multitu_function_object_alias_relocation_order_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave14_multitu_function_object_alias_relocation_order_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave14_multitu_function_object_alias_relocation_order_conflict_reject_aux.c",
        ),
        note="axis1 wave14: mixed function-vs-object alias relocation-order conflict should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis1_wave15_multitu_function_object_alias_three_plan_replay_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave15_multitu_function_object_alias_three_plan_replay_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave15_multitu_function_object_alias_three_plan_replay_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave15_multitu_function_object_alias_three_plan_replay_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave15_multitu_function_object_alias_three_plan_replay_conflict_reject_aux.c",
        ),
        note="axis1 wave15: mixed function-vs-object alias three-plan replay conflict should fail at link stage",
        required_substrings=("duplicate symbol",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_axis2_wave1_line_map_runtime_guard_conflict_bridge",
        source=PROBE_DIR
        / "diagnostics/14__probe_diag_axis2_wave1_line_map_runtime_guard_conflict_bridge.c",
        note="axis2 wave1: #line remap through nested include+macro guard conflict should preserve virtual spelling location",
        required_substrings=("axis2_wave1_guard_layer_a.h",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis2_wave1_line_map_nested_include_macro_chain_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis2_wave1_line_map_nested_include_macro_chain_reject.c",
        note="axis2 wave1: #line remap through nested include+macro chain should preserve virtual spelling location",
        required_substrings=("axis2_wave1_nested_layer_a.h",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_axis2_wave2_line_map_nested_include_macro_guard_reentry_reject",
        source=PROBE_DIR
        / "diagnostics/14__probe_diag_axis2_wave2_line_map_nested_include_macro_guard_reentry_reject.c",
        note="axis2 wave2: #line remap through nested include+macro guard reentry should preserve deep virtual spelling location",
        required_substrings=("axis2_wave2_guard_layer_c.h",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis2_wave2_line_map_nested_include_macro_function_chain_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis2_wave2_line_map_nested_include_macro_function_chain_reject.c",
        note="axis2 wave2: #line remap through nested include+macro function chain should preserve deep virtual spelling location",
        required_substrings=("axis2_wave2_nested_layer_c.h",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_axis2_wave3_line_map_nested_include_macro_type_bridge_reject",
        source=PROBE_DIR
        / "diagnostics/14__probe_diag_axis2_wave3_line_map_nested_include_macro_type_bridge_reject.c",
        note="axis2 wave3: #line remap through four-layer include+macro type-bridge lane should preserve deep virtual spelling location",
        required_substrings=("axis2_wave3_guard_layer_d.h",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis2_wave3_line_map_nested_include_macro_arity_bridge_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis2_wave3_line_map_nested_include_macro_arity_bridge_reject.c",
        note="axis2 wave3: #line remap through four-layer include+macro arity bridge should preserve deep virtual spelling location",
        required_substrings=("axis2_wave3_nested_layer_d.h",),
    ),
    DiagnosticProbe(
        probe_id="14__probe_diag_axis2_wave4_line_map_nested_include_macro_arity_overflow_reject",
        source=PROBE_DIR
        / "diagnostics/14__probe_diag_axis2_wave4_line_map_nested_include_macro_arity_overflow_reject.c",
        note="axis2 wave4: #line remap through five-layer include+macro arity overflow lane should preserve deep virtual spelling location",
        required_substrings=("axis2_wave4_arity_layer_e.h",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis2_wave4_line_map_nested_include_macro_token_bridge_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis2_wave4_line_map_nested_include_macro_token_bridge_reject.c",
        note="axis2 wave4: #line remap through token-paste include+macro bridge should preserve deep virtual spelling location",
        required_substrings=("axis2_wave4_token_layer_d.h",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave1_malformed_include_guard_reentry_depth_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave1_malformed_include_guard_reentry_depth_no_crash.c",
        note="axis4 wave1: deep include-guard reentry chain should fail closed and preserve deep virtual #line location",
        required_substrings=("axis4_wave1_guard_depth_layer_e.h",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave1_malformed_include_graph_mixed_arity_reentry_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave1_malformed_include_graph_mixed_arity_reentry_no_crash.c",
        note="axis4 wave1: include-graph macro reentry with arity drift should fail closed with deterministic preprocessing diagnostics",
        required_substrings=("requires 3 argument(s), got 2", "axis4_wave1_mixed_arity_layer_a.h"),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave2_malformed_include_guard_cycle_reentry_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave2_malformed_include_guard_cycle_reentry_no_crash.c",
        note="axis4 wave2: include-guard contradiction cycle should fail closed with deterministic recursive-include diagnostics",
        required_substrings=("detected recursive include of",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave2_malformed_include_macro_arity_flip_reentry_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave2_malformed_include_macro_arity_flip_reentry_no_crash.c",
        note="axis4 wave2: include-graph macro reentry should deterministically surface arity drift diagnostics",
        required_substrings=("requires 3 argument(s), got 2", "axis4_wave2_arity_flip_layer_a.h"),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave3_malformed_include_gnu_comma_vaargs_reentry_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave3_malformed_include_gnu_comma_vaargs_reentry_no_crash.c",
        note="axis4 wave3: include-graph variadic macro reentry should fail closed on GNU comma-paste extension usage",
        required_substrings=("GNU ', ##__VA_ARGS__' extension is not supported",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave3_malformed_include_token_paste_tail_reentry_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave3_malformed_include_token_paste_tail_reentry_no_crash.c",
        note="axis4 wave3: include-graph token-paste tail lane should fail closed with deterministic expansion diagnostics",
        required_substrings=("macro expansion failed (possible recursion)",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave4_line_map_deep_missing_symbol_reentry_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave4_line_map_deep_missing_symbol_reentry_no_crash.c",
        note="axis4 wave4: deep include reentry line-map lane should preserve virtual spelling location for semantic failures",
        required_substrings=("axis4_wave4_depth_layer_e.h",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave4_line_map_deep_arity_bridge_reentry_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave4_line_map_deep_arity_bridge_reentry_no_crash.c",
        note="axis4 wave4: deep include reentry arity-bridge lane should fail closed with deterministic preprocessing diagnostics",
        required_substrings=("requires 5 argument(s), got 4", "axis4_wave4_arity_layer_a.h"),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave5_conditional_dead_branch_skip_reentry_no_diag",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave5_conditional_dead_branch_skip_reentry_no_diag.c",
        note="axis4 wave5: malformed tokens hidden behind dead include-branch conditionals should be skipped without diagnostics",
        expect_any_diagnostic=False,
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave5_conditional_active_branch_arity_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave5_conditional_active_branch_arity_fail_no_crash.c",
        note="axis4 wave5: active include-branch macro reentry should fail closed with deterministic arity diagnostics",
        required_substrings=("requires 3 argument(s), got 2", "axis4_wave5_active_branch_layer_a.h"),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave6_guard_namespace_collision_reentry_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave6_guard_namespace_collision_reentry_no_crash.c",
        note="axis4 wave6: shared include-guard namespace collisions should fail closed on missing active-path symbol surfaces",
        required_substrings=("axis4_wave6_guard_collision_layer_a.h",),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave6_macro_state_undef_rebind_reentry_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave6_macro_state_undef_rebind_reentry_no_crash.c",
        note="axis4 wave6: cross-header #undef/#define macro-state churn should fail closed with deterministic arity diagnostics",
        required_substrings=("requires 3 argument(s), got 2", "axis4_wave6_state_layer_a.h"),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave7_include_order_permute_a_arity_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave7_include_order_permute_a_arity_fail_no_crash.c",
        note="axis4 wave7: include-order permutation A should fail closed with deterministic arity diagnostics",
        required_substrings=("requires 3 argument(s), got 2", "axis4_wave7_perm_a_layer_a.h"),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave7_include_order_permute_b_arity_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave7_include_order_permute_b_arity_fail_no_crash.c",
        note="axis4 wave7: include-order permutation B should fail closed with deterministic arity diagnostics",
        required_substrings=("requires 3 argument(s), got 2", "axis4_wave7_perm_b_layer_a.h"),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave8_depth_conditional_dead_branch_matrix_no_diag",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave8_depth_conditional_dead_branch_matrix_no_diag.c",
        note="axis4 wave8: depth-amplified dead-branch conditional matrix should skip malformed lanes without diagnostics",
        expect_any_diagnostic=False,
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave8_depth_conditional_active_branch_matrix_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave8_depth_conditional_active_branch_matrix_fail_no_crash.c",
        note="axis4 wave8: depth-amplified active-branch conditional matrix should fail closed with deterministic arity diagnostics",
        required_substrings=("requires 3 argument(s), got 2", "axis4_wave8_active_layer_a.h"),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave9_macro_alias_permute_a_arity_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave9_macro_alias_permute_a_arity_fail_no_crash.c",
        note="axis4 wave9: macro-alias rename permutation A should fail closed with deterministic arity diagnostics",
        required_substrings=("requires 3 argument(s), got 2", "axis4_wave9_perm_a_layer_a.h"),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave9_macro_alias_permute_b_arity_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave9_macro_alias_permute_b_arity_fail_no_crash.c",
        note="axis4 wave9: macro-alias rename permutation B should fail closed with deterministic arity diagnostics",
        required_substrings=("requires 3 argument(s), got 2", "axis4_wave9_perm_b_layer_a.h"),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave10_branch_convergence_inactive_path_no_diag",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave10_branch_convergence_inactive_path_no_diag.c",
        note="axis4 wave10: inactive branch-convergence path should keep winner-only malformed surface suppressed without diagnostics",
        expect_any_diagnostic=False,
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave10_branch_convergence_winner_arity_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave10_branch_convergence_winner_arity_fail_no_crash.c",
        note="axis4 wave10: converged macro-state winner should fail closed with deterministic arity diagnostics",
        required_substrings=("requires 3 argument(s), got 2", "axis4_wave10_winner_layer_a.h"),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave11_dual_convergence_order_a_winner_arity_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave11_dual_convergence_order_a_winner_arity_fail_no_crash.c",
        note="axis4 wave11: dual convergence order-A winner should fail closed with deterministic arity diagnostics",
        required_substrings=("requires 3 argument(s), got 2", "axis4_wave11_order_a_layer_a.h"),
    ),
    DiagnosticProbe(
        probe_id="15__probe_diag_axis4_wave11_dual_convergence_order_b_winner_arity_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave11_dual_convergence_order_b_winner_arity_fail_no_crash.c",
        note="axis4 wave11: dual convergence order-B winner should fail closed with deterministic arity diagnostics",
        required_substrings=("requires 3 argument(s), got 2", "axis4_wave11_order_b_layer_a.h"),
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
        probe_id="01__probe_diagjson_line_directive_macro_line_map_strict",
        source=PROBE_DIR / "diagnostics/01__probe_diagjson_line_directive_macro_line_map_strict.c",
        note="diagnostics JSON should preserve #line remapped location for macro-formed semantic diagnostics",
        expected_codes=(1000, 2000),
        expected_line=404,
    ),
    DiagnosticJsonProbe(
        probe_id="01__probe_diagjson_line_directive_nonvoid_return_location_reject",
        source=PROBE_DIR
        / "diagnostics/01__probe_line_directive_virtual_line_nonvoid_return_location_reject.c",
        note="diagnostics JSON should preserve #line remapped location for non-void return diagnostics",
        expected_codes=(2000,),
        expected_line=322,
    ),
    DiagnosticJsonProbe(
        probe_id="01__probe_diagjson_line_directive_undeclared_identifier_location_strict",
        source=PROBE_DIR
        / "diagnostics/01__probe_diagjson_line_directive_undeclared_identifier_location_strict.c",
        note="diagnostics JSON should preserve #line remapped location for undeclared-identifier diagnostics",
        expected_line=322,
    ),
    DiagnosticJsonProbe(
        probe_id="01__probe_diagjson_nonvoid_return_plain_current_zerozero",
        source=PROBE_DIR / "diagnostics/01__probe_nonvoid_return_plain_current_zerozero.c",
        note="fixed baseline: diagnostics JSON emits source line for non-void return diagnostic",
        expected_codes=(2000,),
        expected_line=2,
    ),
    DiagnosticJsonProbe(
        probe_id="01__probe_diagjson_line_directive_macro_nonvoid_return_location_reject",
        source=PROBE_DIR / "diagnostics/01__probe_line_directive_macro_nonvoid_return_location_reject.c",
        note="diagnostics JSON should preserve #line location for macro-expanded non-void return diagnostics",
        expected_codes=(2000,),
        expected_line=453,
    ),
    DiagnosticJsonProbe(
        probe_id="01__probe_diagjson_line_directive_macro_nonvoid_return_current_zerozero",
        source=PROBE_DIR / "diagnostics/01__probe_line_directive_macro_nonvoid_return_current_zerozero.c",
        note="fixed baseline: diagnostics JSON emits mapped line for #line+macro non-void return diagnostics",
        expected_codes=(2000,),
        expected_line=453,
    ),
    DiagnosticJsonProbe(
        probe_id="02__probe_diagjson_lexer_line_directive_invalid_dollar_location_reject",
        source=PROBE_DIR / "diagnostics/02__probe_lexer_line_directive_invalid_dollar_location_reject.c",
        note="diagnostics JSON should honor #line remap for lexer invalid '$' token",
        expected_codes=(1,),
        expected_line=831,
    ),
    DiagnosticJsonProbe(
        probe_id="02__probe_diagjson_lexer_line_directive_invalid_dollar_current_physical_line",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_invalid_dollar_current_physical_line.c",
        note="fixed baseline: diagnostics JSON reports remapped line for lexer invalid '$' token",
        expected_codes=(1,),
        expected_line=831,
    ),
    DiagnosticJsonProbe(
        probe_id="02__probe_diagjson_lexer_line_directive_unterminated_string_location_reject",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_unterminated_string_location_reject.c",
        note="diagnostics JSON should honor #line remap for lexer unterminated string literal",
        expected_codes=(1,),
        expected_line=721,
    ),
    DiagnosticJsonProbe(
        probe_id="02__probe_diagjson_lexer_line_directive_unterminated_string_current_physical_line",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_unterminated_string_current_physical_line.c",
        note="fixed baseline: diagnostics JSON reports remapped line for lexer unterminated string",
        expected_codes=(1,),
        expected_line=721,
    ),
    DiagnosticJsonProbe(
        probe_id="02__probe_diagjson_lexer_line_directive_char_invalid_hex_escape_location_reject",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_char_invalid_hex_escape_location_reject.c",
        note="diagnostics JSON should honor #line remap for lexer invalid \\x char escape",
        expected_codes=(1,),
        expected_line=911,
    ),
    DiagnosticJsonProbe(
        probe_id="02__probe_diagjson_lexer_line_directive_char_invalid_hex_escape_current_physical_line",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_char_invalid_hex_escape_current_physical_line.c",
        note="fixed baseline: diagnostics JSON reports remapped line for lexer invalid \\x char escape",
        expected_codes=(1,),
        expected_line=911,
    ),
    DiagnosticJsonProbe(
        probe_id="02__probe_diagjson_lexer_line_directive_string_invalid_escape_location_reject",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_string_invalid_escape_location_reject.c",
        note="diagnostics JSON should honor #line remap for lexer invalid string escape",
        expected_codes=(1,),
        expected_line=941,
    ),
    DiagnosticJsonProbe(
        probe_id="02__probe_diagjson_lexer_line_directive_string_invalid_escape_current_physical_line",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_string_invalid_escape_current_physical_line.c",
        note="fixed baseline: diagnostics JSON reports remapped line for lexer invalid string escape",
        expected_codes=(1,),
        expected_line=941,
    ),
    DiagnosticJsonProbe(
        probe_id="02__probe_diagjson_lexer_line_directive_invalid_at_location_reject",
        source=PROBE_DIR / "diagnostics/02__probe_lexer_line_directive_invalid_at_location_reject.c",
        note="diagnostics JSON should honor #line remap for lexer invalid '@' token",
        expected_codes=(1,),
        expected_line=971,
    ),
    DiagnosticJsonProbe(
        probe_id="02__probe_diagjson_lexer_line_directive_invalid_at_current_physical_line",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_invalid_at_current_physical_line.c",
        note="fixed baseline: diagnostics JSON reports remapped line for lexer invalid '@' token",
        expected_codes=(1,),
        expected_line=971,
    ),
    DiagnosticJsonProbe(
        probe_id="02__probe_diagjson_lexer_line_directive_invalid_backtick_location_reject",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_invalid_backtick_location_reject.c",
        note="diagnostics JSON should honor #line remap for lexer invalid '`' token",
        expected_codes=(1,),
        expected_line=981,
    ),
    DiagnosticJsonProbe(
        probe_id="02__probe_diagjson_lexer_line_directive_invalid_backtick_current_physical_line",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_invalid_backtick_current_physical_line.c",
        note="fixed baseline: diagnostics JSON reports remapped line for lexer invalid '`' token",
        expected_codes=(1,),
        expected_line=981,
    ),
    DiagnosticJsonProbe(
        probe_id="02__probe_diagjson_lexer_line_directive_unterminated_char_location_reject",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_unterminated_char_location_reject.c",
        note="diagnostics JSON should honor #line remap for lexer unterminated character literal",
        expected_codes=(1,),
        expected_line=991,
    ),
    DiagnosticJsonProbe(
        probe_id="02__probe_diagjson_lexer_line_directive_unterminated_char_current_physical_line",
        source=PROBE_DIR
        / "diagnostics/02__probe_lexer_line_directive_unterminated_char_current_physical_line.c",
        note="fixed baseline: diagnostics JSON reports remapped line for lexer unterminated character literal",
        expected_codes=(1,),
        expected_line=991,
    ),
    DiagnosticJsonProbe(
        probe_id="03__probe_diagjson_line_directive_error_location_reject",
        source=PROBE_DIR / "diagnostics/03__probe_line_directive_error_location_reject.c",
        note="diagnostics JSON should honor #line remap for #error directive diagnostics",
        expected_codes=(3000,),
        expected_line=777,
    ),
    DiagnosticJsonProbe(
        probe_id="03__probe_diagjson_line_directive_error_current_physical_line",
        source=PROBE_DIR / "diagnostics/03__probe_line_directive_error_current_physical_line.c",
        note="fixed baseline: diagnostics JSON now reports remapped line for #error directive diagnostics",
        expected_codes=(3000,),
        expected_line=777,
    ),
    DiagnosticJsonProbe(
        probe_id="03__probe_diagjson_line_directive_warning_location_reject",
        source=PROBE_DIR / "diagnostics/03__probe_line_directive_warning_location_reject.c",
        note="diagnostics JSON should honor #line remap for #warning directive diagnostics",
        expected_codes=(3000,),
        expected_line=888,
    ),
    DiagnosticJsonProbe(
        probe_id="03__probe_diagjson_line_directive_warning_current_physical_line",
        source=PROBE_DIR / "diagnostics/03__probe_line_directive_warning_current_physical_line.c",
        note="fixed baseline: diagnostics JSON now reports remapped line for #warning directive diagnostics",
        expected_codes=(3000,),
        expected_line=888,
    ),
    DiagnosticJsonProbe(
        probe_id="03__probe_diagjson_line_directive_pragma_stdc_location_reject",
        source=PROBE_DIR / "diagnostics/03__probe_diagjson_line_directive_pragma_stdc_location_reject.c",
        note="diagnostics JSON should honor #line remap for #pragma STDC diagnostics",
        expected_codes=(3000,),
        expected_line=444,
    ),
    DiagnosticJsonProbe(
        probe_id="03__probe_diagjson_line_directive_pragma_stdc_current_physical_line",
        source=PROBE_DIR / "diagnostics/03__probe_diagjson_line_directive_pragma_stdc_current_physical_line.c",
        note="fixed baseline: diagnostics JSON now reports remapped line for #pragma STDC diagnostics",
        expected_codes=(3000,),
        expected_line=444,
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_diagjson_line_directive_shift_width_location_reject",
        source=PROBE_DIR / "diagnostics/05__probe_diagjson_line_directive_shift_width_location_reject.c",
        note="diagnostics JSON should honor #line remap for shift-width expression diagnostic",
        expected_codes=(2000,),
        expected_line=851,
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_diagjson_line_directive_alignof_expr_location_reject",
        source=PROBE_DIR / "diagnostics/05__probe_diagjson_line_directive_alignof_expr_location_reject.c",
        note="fixed baseline: diagnostics JSON reports remapped return-statement line for _Alignof-expression diagnostic",
        expected_codes=(2000,),
        expected_line=863,
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_diagjson_line_directive_alignof_expr_reduced_location_pass",
        source=PROBE_DIR / "diagnostics/05__probe_diagjson_line_directive_alignof_expr_reduced_location_pass.c",
        note="reduced threshold: single-line _Alignof-expression lane preserves remapped #line boundary in diagnostics JSON",
        expected_codes=(2000,),
        expected_line=861,
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_diagjson_line_directive_shift_width_file_presence_reject",
        source=PROBE_DIR / "diagnostics/05__probe_diagjson_line_directive_shift_width_file_presence_reject.c",
        note="strict frontier: diagnostics JSON should include has_file for shift-width diagnostic under #line remap",
        expected_codes=(2000,),
        expected_line=871,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_diagjson_line_directive_shift_width_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/05__probe_diagjson_line_directive_shift_width_current_sparse_pass.c",
        note="fixed baseline: shift-width diagnostics JSON under #line remap carries file presence",
        expected_codes=(2000,),
        expected_line=871,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_diagjson_line_directive_alignof_expr_file_presence_reject",
        source=PROBE_DIR / "diagnostics/05__probe_diagjson_line_directive_alignof_expr_file_presence_reject.c",
        note="strict frontier: diagnostics JSON should include has_file for _Alignof-expression diagnostic under #line remap",
        expected_codes=(2000,),
        expected_line=881,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_diagjson_line_directive_alignof_expr_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/05__probe_diagjson_line_directive_alignof_expr_current_sparse_pass.c",
        note="fixed baseline: _Alignof-expression diagnostics JSON under #line remap carries file presence",
        expected_codes=(2000,),
        expected_line=881,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_diagjson_line_directive_macro_add_rich_presence_strict",
        source=PROBE_DIR / "diagnostics/05__probe_diagjson_line_directive_macro_add_rich_presence_strict.c",
        note="control lane: macro-expanded add-void diagnostics JSON carries rich location (line/column/file)",
        expected_codes=(2000,),
        expected_line=901,
        expected_column=37,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_diagjson_line_directive_include_shift_width_file_presence_reject",
        source=PROBE_DIR / "diagnostics/05__probe_diagjson_line_directive_include_shift_width_file_presence_reject.c",
        note="strict frontier: include-header shift-width diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=1361,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_diagjson_line_directive_include_shift_width_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/05__probe_diagjson_line_directive_include_shift_width_current_sparse_pass.c",
        note="fixed baseline: include-header shift-width diagnostics JSON carries file presence under #line remap",
        expected_codes=(2000,),
        expected_line=1361,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_diagjson_line_directive_include_alignof_expr_file_presence_reject",
        source=PROBE_DIR / "diagnostics/05__probe_diagjson_line_directive_include_alignof_expr_file_presence_reject.c",
        note="strict frontier: include-header _Alignof-expression diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=1381,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_diagjson_line_directive_include_alignof_expr_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/05__probe_diagjson_line_directive_include_alignof_expr_current_sparse_pass.c",
        note="fixed baseline: include-header _Alignof-expression diagnostics JSON carries file presence under #line remap",
        expected_codes=(2000,),
        expected_line=1381,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_diagjson_line_directive_include_macro_add_rich_presence_strict",
        source=PROBE_DIR / "diagnostics/05__probe_diagjson_line_directive_include_macro_add_rich_presence_strict.c",
        note="control lane: include-header macro-expanded add-void diagnostics JSON carries rich location (line/column/file)",
        expected_codes=(2000,),
        expected_line=1411,
        expected_column=47,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_assign_incompatible_file_presence_reject",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_assign_incompatible_file_presence_reject.c",
        note="strict frontier: assignment-incompatible diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=1044,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_assign_incompatible_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_assign_incompatible_current_sparse_pass.c",
        note="fixed baseline: assignment-incompatible diagnostics JSON under #line remap includes file presence",
        expected_codes=(2000,),
        expected_line=1044,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_assign_qualifier_loss_file_presence_reject",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_assign_qualifier_loss_file_presence_reject.c",
        note="strict frontier: qualifier-loss assignment diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=1065,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_assign_qualifier_loss_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_assign_qualifier_loss_current_sparse_pass.c",
        note="fixed baseline: qualifier-loss assignment diagnostics JSON under #line remap includes file presence",
        expected_codes=(2000,),
        expected_line=1065,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_include_assign_incompatible_file_presence_reject",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_include_assign_incompatible_file_presence_reject.c",
        note="strict frontier: include-header assignment-incompatible diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=1464,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_include_assign_incompatible_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_include_assign_incompatible_current_sparse_pass.c",
        note="fixed baseline: include-header assignment-incompatible diagnostics JSON under #line remap includes file presence",
        expected_codes=(2000,),
        expected_line=1464,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_include_assign_qualifier_loss_file_presence_reject",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_include_assign_qualifier_loss_file_presence_reject.c",
        note="strict frontier: include-header qualifier-loss assignment diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=1485,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_include_assign_qualifier_loss_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_include_assign_qualifier_loss_current_sparse_pass.c",
        note="fixed baseline: include-header qualifier-loss assignment diagnostics JSON under #line remap includes file presence",
        expected_codes=(2000,),
        expected_line=1485,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_nonmodifiable_lvalue_file_presence_reject",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_nonmodifiable_lvalue_file_presence_reject.c",
        note="strict frontier: nonmodifiable-lvalue assignment diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=1763,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_nonmodifiable_lvalue_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_nonmodifiable_lvalue_current_sparse_pass.c",
        note="fixed baseline: nonmodifiable-lvalue assignment diagnostics JSON under #line remap includes file presence",
        expected_codes=(2000,),
        expected_line=1763,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_include_nonmodifiable_lvalue_file_presence_reject",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_include_nonmodifiable_lvalue_file_presence_reject.c",
        note="strict frontier: include-header nonmodifiable-lvalue assignment diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=1783,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_include_nonmodifiable_lvalue_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_include_nonmodifiable_lvalue_current_sparse_pass.c",
        note="fixed baseline: include-header nonmodifiable-lvalue assignment diagnostics JSON under #line remap includes file presence",
        expected_codes=(2000,),
        expected_line=1783,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_bitfield_address_file_presence_reject",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_bitfield_address_file_presence_reject.c",
        note="strict frontier: bitfield address-of diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=2107,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_bitfield_address_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_bitfield_address_file_presence_reject.c",
        note="current threshold: bitfield address-of diagnostics JSON under #line remap includes file presence",
        expected_codes=(2000,),
        expected_line=2107,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_include_bitfield_address_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/06__probe_diagjson_line_directive_include_bitfield_address_file_presence_reject.c",
        note="strict frontier: include-header bitfield address-of diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=2117,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_bitfield_address_file_presence_reject.c",
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_bitfield_address_file_presence_reject.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_include_bitfield_address_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/06__probe_diagjson_line_directive_include_bitfield_address_file_presence_reject.c",
        note="current threshold: include-header bitfield address-of diagnostics JSON under #line remap includes file presence",
        expected_codes=(2000,),
        expected_line=2117,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_bitfield_address_file_presence_reject.c",
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_bitfield_address_file_presence_reject.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_temp_increment_file_presence_reject",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_temp_increment_file_presence_reject.c",
        note="strict frontier: temporary increment diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=2122,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_temp_increment_current_sparse_pass",
        source=PROBE_DIR / "diagnostics/06__probe_diagjson_line_directive_temp_increment_file_presence_reject.c",
        note="current threshold: temporary increment diagnostics JSON under #line remap includes file presence",
        expected_codes=(2000,),
        expected_line=2122,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_include_temp_increment_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/06__probe_diagjson_line_directive_include_temp_increment_file_presence_reject.c",
        note="strict frontier: include-header temporary increment diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=2132,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_temp_increment_file_presence_reject.c",
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_temp_increment_file_presence_reject.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_include_temp_increment_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/06__probe_diagjson_line_directive_include_temp_increment_file_presence_reject.c",
        note="current threshold: include-header temporary increment diagnostics JSON under #line remap includes file presence",
        expected_codes=(2000,),
        expected_line=2132,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_temp_increment_file_presence_reject.c",
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_temp_increment_file_presence_reject.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_compound_assign_pointer_plus_pointer_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/06__probe_diagjson_line_directive_compound_assign_pointer_plus_pointer_file_presence_reject.c",
        note="strict frontier: pointer-plus-pointer compound-assignment diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=2346,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_compound_assign_pointer_plus_pointer_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/06__probe_diagjson_line_directive_compound_assign_pointer_plus_pointer_file_presence_reject.c",
        note="reduced threshold: pointer-plus-pointer compound-assignment diagnostics JSON under #line remap emits diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_include_compound_assign_pointer_plus_pointer_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/06__probe_diagjson_line_directive_include_compound_assign_pointer_plus_pointer_file_presence_reject.c",
        note="strict frontier: include-header pointer-plus-pointer compound-assignment diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=2356,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_compound_assign_pointer_plus_pointer_file_presence_reject.c",
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_compound_assign_pointer_plus_pointer_file_presence_reject.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_include_compound_assign_pointer_plus_pointer_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/06__probe_diagjson_line_directive_include_compound_assign_pointer_plus_pointer_file_presence_reject.c",
        note="reduced threshold: include-header pointer-plus-pointer compound-assignment diagnostics JSON under #line remap emits diagnostic payload",
        expected_codes=(2000,),
        inputs=(
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_compound_assign_pointer_plus_pointer_file_presence_reject.c",
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_compound_assign_pointer_plus_pointer_file_presence_reject.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_compound_assign_const_lvalue_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/06__probe_diagjson_line_directive_compound_assign_const_lvalue_file_presence_reject.c",
        note="strict frontier: const-lvalue compound-assignment diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=2364,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_compound_assign_const_lvalue_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/06__probe_diagjson_line_directive_compound_assign_const_lvalue_file_presence_reject.c",
        note="reduced threshold: const-lvalue compound-assignment diagnostics JSON under #line remap emits diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_include_compound_assign_const_lvalue_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/06__probe_diagjson_line_directive_include_compound_assign_const_lvalue_file_presence_reject.c",
        note="strict frontier: include-header const-lvalue compound-assignment diagnostics JSON should include has_file under #line remap",
        expected_codes=(2000,),
        expected_line=2374,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_compound_assign_const_lvalue_file_presence_reject.c",
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_compound_assign_const_lvalue_file_presence_reject.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="06__probe_diagjson_line_directive_include_compound_assign_const_lvalue_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/06__probe_diagjson_line_directive_include_compound_assign_const_lvalue_file_presence_reject.c",
        note="reduced threshold: include-header const-lvalue compound-assignment diagnostics JSON under #line remap emits diagnostic payload",
        expected_codes=(2000,),
        inputs=(
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_compound_assign_const_lvalue_file_presence_reject.c",
            PROBE_DIR
            / "diagnostics/06__probe_diagjson_line_directive_include_compound_assign_const_lvalue_file_presence_reject.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_line_directive_conv_pointer_assign_float_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/07__probe_diagjson_line_directive_conv_pointer_assign_float_rich_presence.c",
        note="strict frontier: pointer-assignment-from-float diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=2104,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_line_directive_conv_pointer_assign_float_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/07__probe_diagjson_line_directive_conv_pointer_assign_float_rich_presence.c",
        note="reduced threshold: pointer-assignment-from-float diagnostics JSON under #line remap emits conversion diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_line_directive_void_pointer_arith_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/07__probe_diagjson_line_directive_void_pointer_arith_rich_presence.c",
        note="strict frontier: void-pointer arithmetic diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=2203,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_line_directive_void_pointer_arith_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/07__probe_diagjson_line_directive_void_pointer_arith_rich_presence.c",
        note="reduced threshold: void-pointer arithmetic diagnostics JSON under #line remap emits conversion diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_line_directive_include_conv_pointer_assign_float_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/07__probe_diagjson_line_directive_include_conv_pointer_assign_float_rich_presence.c",
        note="strict frontier: include-header pointer-assignment-from-float diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=2514,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_line_directive_include_conv_pointer_assign_float_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/07__probe_diagjson_line_directive_include_conv_pointer_assign_float_rich_presence.c",
        note="reduced threshold: include-header pointer-assignment-from-float diagnostics JSON under #line remap emits conversion diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_line_directive_include_void_pointer_arith_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/07__probe_diagjson_line_directive_include_void_pointer_arith_rich_presence.c",
        note="strict frontier: include-header void-pointer arithmetic diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=2613,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_line_directive_include_void_pointer_arith_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/07__probe_diagjson_line_directive_include_void_pointer_arith_rich_presence.c",
        note="reduced threshold: include-header void-pointer arithmetic diagnostics JSON under #line remap emits conversion diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_assign_struct_to_int_reject",
        source=PROBE_DIR / "diagnostics/07__probe_assign_struct_to_int_reject.c",
        note="diagnostics JSON should include assignment incompatibility for struct-to-int assignment",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_agg_invalid_member_reject",
        source=PROBE_DIR / "diagnostics/07__probe_agg_invalid_member_reject.c",
        note="diagnostics JSON should include invalid aggregate member access rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_constexpr_array_size_reject",
        source=PROBE_DIR / "diagnostics/07__probe_constexpr_array_size_reject.c",
        note="diagnostics JSON should include non-constant array-size rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_constexpr_case_label_reject",
        source=PROBE_DIR / "diagnostics/07__probe_constexpr_case_label_reject.c",
        note="diagnostics JSON should include non-constant case-label rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_constexpr_static_init_reject",
        source=PROBE_DIR / "diagnostics/07__probe_constexpr_static_init_reject.c",
        note="diagnostics JSON should include non-constant static-initializer rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_conv_pointer_assign_float_reject",
        source=PROBE_DIR / "diagnostics/07__probe_conv_pointer_assign_float_reject.c",
        note="diagnostics JSON should include pointer-assignment conversion rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_conv_pointer_init_struct_reject",
        source=PROBE_DIR / "diagnostics/07__probe_conv_pointer_init_struct_reject.c",
        note="diagnostics JSON should include pointer-initializer conversion rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_conv_pointer_init_nonzero_int_reject",
        source=PROBE_DIR / "diagnostics/07__probe_conv_pointer_init_nonzero_int_reject.c",
        note="diagnostics JSON should include pointer-initializer nonzero-integer rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_conv_assign_pointer_to_int_reject",
        source=PROBE_DIR / "diagnostics/07__probe_conv_assign_pointer_to_int_reject.c",
        note="diagnostics JSON should include pointer-to-integer assignment rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_void_pointer_arith_reject",
        source=PROBE_DIR / "diagnostics/07__probe_void_pointer_arith_reject.c",
        note="diagnostics JSON should include void-pointer arithmetic assignment rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_agg_union_missing_nested_field_reject",
        source=PROBE_DIR / "diagnostics/07__probe_agg_union_missing_nested_field_reject.c",
        note="diagnostics JSON should include missing nested aggregate member rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_cast_struct_to_int_reject",
        source=PROBE_DIR / "diagnostics/07__probe_cast_struct_to_int_reject.c",
        note="diagnostics JSON should include explicit struct-to-int cast rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_cast_int_to_struct_reject",
        source=PROBE_DIR / "diagnostics/07__probe_cast_int_to_struct_reject.c",
        note="diagnostics JSON should include explicit int-to-struct cast rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_agg_dot_scalar_base_reject",
        source=PROBE_DIR / "diagnostics/07__probe_agg_dot_scalar_base_reject.c",
        note="diagnostics JSON should include dot-on-scalar member-access rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_agg_arrow_ptr_to_scalar_reject",
        source=PROBE_DIR / "diagnostics/07__probe_agg_arrow_ptr_to_scalar_reject.c",
        note="diagnostics JSON should include arrow-on-pointer-to-scalar member-access rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_designator_array_index_nonconst_reject",
        source=PROBE_DIR / "diagnostics/07__probe_designator_array_index_nonconst_reject.c",
        note="diagnostics JSON should include non-constant array-designator index rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_designator_array_index_oob_reject",
        source=PROBE_DIR / "diagnostics/07__probe_designator_array_index_oob_reject.c",
        note="diagnostics JSON should include out-of-bounds array-designator index rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_designator_nested_unknown_field_reject",
        source=PROBE_DIR / "diagnostics/07__probe_designator_nested_unknown_field_reject.c",
        note="diagnostics JSON should include nested unknown-field designated-initializer rejection",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_designator_array_index_nonconst_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_designator_array_index_nonconst_reject.c",
        note="diagnostics JSON strict parity for non-constant array-designator index rejection",
        expected_codes=(2000,),
        expected_line=2,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_designator_array_index_oob_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_designator_array_index_oob_reject.c",
        note="diagnostics JSON strict parity for out-of-bounds array-designator index rejection",
        expected_codes=(2000,),
        expected_line=1,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_designator_nested_unknown_field_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_designator_nested_unknown_field_reject.c",
        note="diagnostics JSON strict parity for nested unknown-field designated-initializer rejection",
        expected_codes=(2000,),
        expected_line=10,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_agg_dot_scalar_base_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_agg_dot_scalar_base_reject.c",
        note="diagnostics JSON strict parity for dot-on-scalar member-access rejection",
        expected_codes=(2000,),
        expected_line=7,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_agg_arrow_ptr_to_scalar_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_agg_arrow_ptr_to_scalar_reject.c",
        note="diagnostics JSON strict parity for arrow-on-pointer-to-scalar member-access rejection",
        expected_codes=(2000,),
        expected_line=4,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_agg_dot_array_base_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_agg_dot_array_base_reject.c",
        note="diagnostics JSON strict parity for dot-on-array member-access rejection",
        expected_codes=(2000,),
        expected_line=7,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_conv_pointer_assign_float_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_conv_pointer_assign_float_reject.c",
        note="diagnostics JSON strict parity for pointer-assignment-from-float rejection",
        expected_codes=(2000,),
        expected_line=4,
        expected_column=5,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_conv_pointer_init_struct_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_conv_pointer_init_struct_reject.c",
        note="diagnostics JSON strict parity for pointer-initializer-from-struct rejection",
        expected_codes=(2000,),
        expected_line=7,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_conv_pointer_init_nonzero_int_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_conv_pointer_init_nonzero_int_reject.c",
        note="diagnostics JSON strict parity for pointer-initializer-from-nonzero-int rejection",
        expected_codes=(2000,),
        expected_line=2,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_conv_assign_pointer_to_int_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_conv_assign_pointer_to_int_reject.c",
        note="diagnostics JSON strict parity for pointer-to-int assignment rejection",
        expected_codes=(2000,),
        expected_line=5,
        expected_column=5,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_void_pointer_arith_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_void_pointer_arith_reject.c",
        note="diagnostics JSON strict parity for void-pointer arithmetic assignment rejection",
        expected_codes=(2000,),
        expected_line=3,
        expected_column=10,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_cast_struct_to_int_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_cast_struct_to_int_reject.c",
        note="diagnostics JSON strict parity for explicit struct-to-int cast rejection",
        expected_codes=(2000,),
        expected_line=8,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_cast_int_to_struct_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_cast_int_to_struct_reject.c",
        note="diagnostics JSON strict parity for explicit int-to-struct cast rejection",
        expected_codes=(2000,),
        expected_line=8,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_agg_union_missing_nested_field_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_agg_union_missing_nested_field_reject.c",
        note="diagnostics JSON strict parity for nested aggregate member rejection inside union variant",
        expected_codes=(2000,),
        expected_line=14,
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_agg_invalid_member_reject_strict",
        source=PROBE_DIR / "diagnostics/07__probe_agg_invalid_member_reject.c",
        note="diagnostics JSON strict parity for aggregate unknown-member rejection",
        expected_codes=(2000,),
        expected_line=8,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_designator_unknown_field_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_designator_unknown_field_rich_presence.c",
        note="strict frontier: designator unknown-field diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=3006,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_designator_unknown_field_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_designator_unknown_field_rich_presence.c",
        note="reduced threshold: designator unknown-field diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_designator_array_index_oob_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_designator_array_index_oob_rich_presence.c",
        note="strict frontier: designator array-index-oob diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=3101,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_designator_array_index_oob_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_designator_array_index_oob_rich_presence.c",
        note="reduced threshold: designator array-index-oob diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_designator_unknown_field_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_designator_unknown_field_rich_presence.c",
        note="strict frontier: include-header designator unknown-field diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=3406,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_designator_unknown_field_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_designator_unknown_field_rich_presence.c",
        note="reduced threshold: include-header designator unknown-field diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_designator_array_index_oob_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_oob_rich_presence.c",
        note="strict frontier: include-header designator array-index-oob diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=3501,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_designator_array_index_oob_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_oob_rich_presence.c",
        note="reduced threshold: include-header designator array-index-oob diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_nested_designator_unknown_field_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_nested_designator_unknown_field_rich_presence.c",
        note="strict frontier: nested designator unknown-field diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=4006,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_nested_designator_unknown_field_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_nested_designator_unknown_field_rich_presence.c",
        note="reduced threshold: nested designator unknown-field diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_nested_designator_array_index_oob_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_nested_designator_array_oob_rich_presence.c",
        note="strict frontier: nested designator array-index-oob diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=4103,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_nested_designator_array_index_oob_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_nested_designator_array_oob_rich_presence.c",
        note="reduced threshold: nested designator array-index-oob diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_nested_designator_unknown_field_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_unknown_field_rich_presence.c",
        note="strict frontier: include-header nested designator unknown-field diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=4206,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_nested_designator_unknown_field_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_unknown_field_rich_presence.c",
        note="reduced threshold: include-header nested designator unknown-field diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_nested_designator_array_index_oob_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_oob_rich_presence.c",
        note="strict frontier: include-header nested designator array-index-oob diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=4303,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_nested_designator_array_index_oob_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_oob_rich_presence.c",
        note="reduced threshold: include-header nested designator array-index-oob diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_designator_array_index_nonconst_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_designator_array_index_nonconst_rich_presence.c",
        note="strict frontier: non-const designator-index diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=4402,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_designator_array_index_nonconst_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_designator_array_index_nonconst_rich_presence.c",
        note="reduced threshold: non-const designator-index diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_designator_array_index_negative_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_designator_array_index_negative_rich_presence.c",
        note="strict frontier: negative designator-index diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=4501,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_designator_array_index_negative_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_designator_array_index_negative_rich_presence.c",
        note="reduced threshold: negative designator-index diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_designator_array_index_nonconst_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_nonconst_rich_presence.c",
        note="strict frontier: include-header non-const designator-index diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=4602,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_nonconst_rich_presence.c",
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_nonconst_rich_presence.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_designator_array_index_nonconst_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_nonconst_rich_presence.c",
        note="reduced threshold: include-header non-const designator-index diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_nonconst_rich_presence.c",
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_nonconst_rich_presence.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_designator_array_index_negative_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_negative_rich_presence.c",
        note="strict frontier: include-header negative designator-index diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=4701,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_negative_rich_presence.c",
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_negative_rich_presence.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_designator_array_index_negative_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_negative_rich_presence.c",
        note="reduced threshold: include-header negative designator-index diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_negative_rich_presence.c",
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_negative_rich_presence.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_nested_designator_array_index_nonconst_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_nested_designator_array_index_nonconst_rich_presence.c",
        note="strict frontier: nested non-const designator-index diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=6402,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_nested_designator_array_index_nonconst_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_nested_designator_array_index_nonconst_rich_presence.c",
        note="reduced threshold: nested non-const designator-index diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_nested_designator_array_index_nonconst_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_nonconst_rich_presence.c",
        note="strict frontier: include-header nested non-const designator-index diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=6502,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_nonconst_rich_presence.c",
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_nonconst_rich_presence.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_nested_designator_array_index_nonconst_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_nonconst_rich_presence.c",
        note="reduced threshold: include-header nested non-const designator-index diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_nonconst_rich_presence.c",
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_nonconst_rich_presence.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_nested_designator_array_index_negative_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_nested_designator_array_index_negative_rich_presence.c",
        note="strict frontier: nested negative designator-index diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=6602,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_nested_designator_array_index_negative_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_nested_designator_array_index_negative_rich_presence.c",
        note="reduced threshold: nested negative designator-index diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_nested_designator_array_index_negative_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_negative_rich_presence.c",
        note="strict frontier: include-header nested negative designator-index diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=6702,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_negative_rich_presence.c",
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_negative_rich_presence.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_nested_designator_array_index_negative_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_negative_rich_presence.c",
        note="reduced threshold: include-header nested negative designator-index diagnostics JSON under #line remap emits initializer diagnostic payload",
        expected_codes=(2000,),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_negative_rich_presence.c",
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_negative_rich_presence.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_flex_not_last_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_flex_not_last_rich_presence.c",
        note="strict frontier: flex-not-last diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=5502,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_flex_not_last_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_flex_not_last_rich_presence.c",
        note="reduced threshold: flex-not-last diagnostics JSON currently emits sparse semantic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_flex_not_last_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_flex_not_last_rich_presence.c",
        note="strict frontier: include-header flex-not-last diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=5602,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_flex_not_last_rich_presence.c",
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_flex_not_last_rich_presence.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_flex_not_last_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_flex_not_last_rich_presence.c",
        note="reduced threshold: include-header flex-not-last diagnostics JSON currently emits sparse semantic payload",
        expected_codes=(2000,),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_flex_not_last_rich_presence.c",
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_flex_not_last_rich_presence.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_union_flex_member_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_union_flex_member_rich_presence.c",
        note="strict frontier: union flexible-member diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=5702,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_union_flex_member_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_union_flex_member_rich_presence.c",
        note="reduced threshold: union flexible-member diagnostics JSON currently emits sparse semantic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_union_flex_member_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_union_flex_member_rich_presence.c",
        note="strict frontier: include-header union flexible-member diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=5802,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_union_flex_member_rich_presence.c",
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_union_flex_member_rich_presence.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_diagjson_line_directive_include_union_flex_member_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/08__probe_diagjson_line_directive_include_union_flex_member_rich_presence.c",
        note="reduced threshold: include-header union flexible-member diagnostics JSON currently emits sparse semantic payload",
        expected_codes=(2000,),
        inputs=(
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_union_flex_member_rich_presence.c",
            PROBE_DIR
            / "diagnostics/08__probe_diagjson_line_directive_include_union_flex_member_rich_presence.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_break_outside_loop_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_break_outside_loop_rich_presence.c",
        note="strict frontier: break-outside-loop diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=5002,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_break_outside_loop_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_break_outside_loop_rich_presence.c",
        note="reduced threshold: break-outside-loop diagnostics JSON under #line remap emits semantic diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_continue_outside_loop_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_continue_outside_loop_rich_presence.c",
        note="strict frontier: continue-outside-loop diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=5102,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_continue_outside_loop_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_continue_outside_loop_rich_presence.c",
        note="reduced threshold: continue-outside-loop diagnostics JSON under #line remap emits semantic diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_include_break_outside_loop_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_include_break_outside_loop_rich_presence.c",
        note="strict frontier: include-header break-outside-loop diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=5402,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_include_break_outside_loop_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_include_break_outside_loop_rich_presence.c",
        note="reduced threshold: include-header break-outside-loop diagnostics JSON under #line remap emits semantic diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_include_continue_outside_loop_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_include_continue_outside_loop_rich_presence.c",
        note="strict frontier: include-header continue-outside-loop diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=5502,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_include_continue_outside_loop_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_include_continue_outside_loop_rich_presence.c",
        note="reduced threshold: include-header continue-outside-loop diagnostics JSON under #line remap emits semantic diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_switch_multiple_default_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_switch_multiple_default_rich_presence.c",
        note="strict frontier: previous-default warning location in diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=5604,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_switch_multiple_default_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_switch_multiple_default_rich_presence.c",
        note="reduced threshold: switch-multiple-default diagnostics JSON under #line remap emits diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_switch_duplicate_case_folded_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_switch_duplicate_case_folded_rich_presence.c",
        note="strict frontier: previous-case warning location in diagnostics JSON under #line remap should include file presence",
        expected_codes=(2000,),
        expected_line=5704,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_switch_duplicate_case_folded_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_switch_duplicate_case_folded_rich_presence.c",
        note="reduced threshold: switch-duplicate-case-folded diagnostics JSON under #line remap emits diagnostic payload",
        expected_codes=(2000,),
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_switch_pointer_condition_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_switch_pointer_condition_file_presence_reject.c",
        note="strict frontier: #line pointer switch-condition diagnostics JSON should include has_file",
        expected_codes=(2000,),
        expected_line=7905,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_switch_pointer_condition_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_switch_pointer_condition_file_presence_reject.c",
        note="reduced threshold: #line pointer switch-condition diagnostics JSON should emit diagnostic payload at remapped line",
        expected_codes=(2000,),
        expected_line=7905,
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_include_switch_pointer_condition_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_include_switch_pointer_condition_file_presence_reject.c",
        note="strict frontier: include-header #line pointer switch-condition diagnostics JSON should include has_file",
        expected_codes=(2000,),
        expected_line=7915,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/09__probe_diagjson_line_directive_include_switch_pointer_condition_file_presence_reject.c",
            PROBE_DIR
            / "diagnostics/09__probe_diagjson_line_directive_include_switch_pointer_condition_file_presence_reject.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_include_switch_pointer_condition_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_include_switch_pointer_condition_file_presence_reject.c",
        note="reduced threshold: include-header #line pointer switch-condition diagnostics JSON should emit diagnostic payload at remapped line",
        expected_codes=(2000,),
        expected_line=7915,
        inputs=(
            PROBE_DIR
            / "diagnostics/09__probe_diagjson_line_directive_include_switch_pointer_condition_file_presence_reject.c",
            PROBE_DIR
            / "diagnostics/09__probe_diagjson_line_directive_include_switch_pointer_condition_file_presence_reject.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_switch_string_condition_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_switch_string_condition_file_presence_reject.c",
        note="strict frontier: #line string-literal switch-condition diagnostics JSON should include has_file",
        expected_codes=(2000,),
        expected_line=7923,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_switch_string_condition_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_switch_string_condition_file_presence_reject.c",
        note="reduced threshold: #line string-literal switch-condition diagnostics JSON should emit diagnostic payload at remapped line",
        expected_codes=(2000,),
        expected_line=7923,
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_include_switch_string_condition_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_include_switch_string_condition_file_presence_reject.c",
        note="strict frontier: include-header #line string-literal switch-condition diagnostics JSON should include has_file",
        expected_codes=(2000,),
        expected_line=7933,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/09__probe_diagjson_line_directive_include_switch_string_condition_file_presence_reject.c",
            PROBE_DIR
            / "diagnostics/09__probe_diagjson_line_directive_include_switch_string_condition_file_presence_reject.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_include_switch_string_condition_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_include_switch_string_condition_file_presence_reject.c",
        note="reduced threshold: include-header #line string-literal switch-condition diagnostics JSON should emit diagnostic payload at remapped line",
        expected_codes=(2000,),
        expected_line=7933,
        inputs=(
            PROBE_DIR
            / "diagnostics/09__probe_diagjson_line_directive_include_switch_string_condition_file_presence_reject.c",
            PROBE_DIR
            / "diagnostics/09__probe_diagjson_line_directive_include_switch_string_condition_file_presence_reject.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_switch_double_condition_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_switch_double_condition_file_presence_reject.c",
        note="strict frontier: #line double switch-condition diagnostics JSON should include has_file",
        expected_codes=(2000,),
        expected_line=7944,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_switch_double_condition_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_switch_double_condition_file_presence_reject.c",
        note="reduced threshold: #line double switch-condition diagnostics JSON should emit diagnostic payload at remapped line",
        expected_codes=(2000,),
        expected_line=7944,
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_include_switch_double_condition_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_include_switch_double_condition_file_presence_reject.c",
        note="strict frontier: include-header #line double switch-condition diagnostics JSON should include has_file",
        expected_codes=(2000,),
        expected_line=7954,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/09__probe_diagjson_line_directive_include_switch_double_condition_file_presence_reject.c",
            PROBE_DIR
            / "diagnostics/09__probe_diagjson_line_directive_include_switch_double_condition_file_presence_reject.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="09__probe_diagjson_line_directive_include_switch_double_condition_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/09__probe_diagjson_line_directive_include_switch_double_condition_file_presence_reject.c",
        note="reduced threshold: include-header #line double switch-condition diagnostics JSON should emit diagnostic payload at remapped line",
        expected_codes=(2000,),
        expected_line=7954,
        inputs=(
            PROBE_DIR
            / "diagnostics/09__probe_diagjson_line_directive_include_switch_double_condition_file_presence_reject.c",
            PROBE_DIR
            / "diagnostics/09__probe_diagjson_line_directive_include_switch_double_condition_file_presence_reject.h",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_extern_type_mismatch_rich_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diagjson_line_directive_extern_type_mismatch_rich_strict.c",
        note="control lane: extern type-mismatch diagnostics JSON should carry remapped line/file/hint under #line",
        expected_codes=(2000,),
        expected_line=6002,
        expected_column=14,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_function_redecl_conflict_rich_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diagjson_line_directive_function_redecl_conflict_rich_strict.c",
        note="control lane: function redeclaration-conflict diagnostics JSON should carry remapped line/file/hint under #line",
        expected_codes=(2000,),
        expected_line=6102,
        expected_column=7,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_include_extern_type_mismatch_rich_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diagjson_line_directive_include_extern_type_mismatch_rich_strict.c",
        note="control lane: include-header extern type-mismatch diagnostics JSON should carry remapped line/file/hint under #line",
        expected_codes=(2000,),
        expected_line=6202,
        expected_column=14,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_include_function_redecl_conflict_rich_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diagjson_line_directive_include_function_redecl_conflict_rich_strict.c",
        note="control lane: include-header function redeclaration-conflict diagnostics JSON should carry remapped line/file/hint under #line",
        expected_codes=(2000,),
        expected_line=6302,
        expected_column=7,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_extern_static_mismatch_rich_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diagjson_line_directive_extern_static_mismatch_rich_strict.c",
        note="frontier lane: extern/static linkage-conflict diagnostics JSON should carry remapped line/file/hint under #line",
        expected_codes=(2000,),
        expected_line=6802,
        expected_column=12,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_tentative_static_conflict_rich_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diagjson_line_directive_tentative_static_conflict_rich_strict.c",
        note="frontier lane: tentative/static linkage-conflict diagnostics JSON should carry remapped line/file/hint under #line",
        expected_codes=(2000,),
        expected_line=6902,
        expected_column=12,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_include_extern_static_mismatch_rich_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diagjson_line_directive_include_extern_static_mismatch_rich_strict.c",
        note="frontier lane: include-header extern/static linkage-conflict diagnostics JSON should carry remapped line/file/hint under #line",
        expected_codes=(2000,),
        expected_line=7002,
        expected_column=12,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_include_tentative_static_conflict_rich_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diagjson_line_directive_include_tentative_static_conflict_rich_strict.c",
        note="frontier lane: include-header tentative/static linkage-conflict diagnostics JSON should carry remapped line/file/hint under #line",
        expected_codes=(2000,),
        expected_line=7102,
        expected_column=12,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_block_extern_different_type_rich_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diagjson_line_directive_block_extern_different_type_rich_strict.c",
        note="frontier lane: block-scope extern type-conflict diagnostics JSON should carry remapped line/file/hint under #line",
        expected_codes=(2000,),
        expected_line=7804,
        expected_column=18,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_include_block_extern_different_type_rich_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diagjson_line_directive_include_block_extern_different_type_rich_strict.c",
        note="frontier lane: include-header block-scope extern type-conflict diagnostics JSON should carry remapped line/file/hint under #line",
        expected_codes=(2000,),
        expected_line=7904,
        expected_column=18,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_multitu_extern_type_conflict_rich_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diagjson_line_directive_multitu_extern_type_conflict_rich_strict_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/10__probe_diagjson_line_directive_multitu_extern_type_conflict_rich_strict_main.c",
            PROBE_DIR
            / "diagnostics/10__probe_diagjson_line_directive_multitu_extern_type_conflict_rich_strict_lib.c",
        ),
        note="frontier lane: multi-TU extern type-conflict diagnostics JSON should preserve remapped location richness under #line",
        expected_codes=(2000,),
        expected_line=9502,
        expected_column=14,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_multitu_extern_type_conflict_current_linkstage_pass",
        source=PROBE_DIR
        / "diagnostics/10__probe_diagjson_line_directive_multitu_extern_type_conflict_rich_strict_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/10__probe_diagjson_line_directive_multitu_extern_type_conflict_rich_strict_main.c",
            PROBE_DIR
            / "diagnostics/10__probe_diagjson_line_directive_multitu_extern_type_conflict_rich_strict_lib.c",
        ),
        note="regression guard: multi-TU extern type-conflict should keep semantic remapped location richness under #line",
        expected_codes=(2000,),
        expected_line=9502,
        expected_column=14,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_multitu_include_extern_type_conflict_rich_strict",
        source=PROBE_DIR
        / "diagnostics/10__probe_diagjson_line_directive_multitu_include_extern_type_conflict_rich_strict_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/10__probe_diagjson_line_directive_multitu_include_extern_type_conflict_rich_strict_main.c",
            PROBE_DIR
            / "diagnostics/10__probe_diagjson_line_directive_multitu_include_extern_type_conflict_rich_strict_lib.c",
        ),
        note="frontier lane: multi-TU include extern type-conflict diagnostics JSON should preserve remapped location richness under #line",
        expected_codes=(2000,),
        expected_line=9602,
        expected_column=14,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_multitu_include_extern_type_conflict_current_linkstage_pass",
        source=PROBE_DIR
        / "diagnostics/10__probe_diagjson_line_directive_multitu_include_extern_type_conflict_rich_strict_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/10__probe_diagjson_line_directive_multitu_include_extern_type_conflict_rich_strict_main.c",
            PROBE_DIR
            / "diagnostics/10__probe_diagjson_line_directive_multitu_include_extern_type_conflict_rich_strict_lib.c",
        ),
        note="regression guard: multi-TU include extern type-conflict should keep semantic remapped location richness under #line",
        expected_codes=(2000,),
        expected_line=9602,
        expected_column=14,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_extern_array_mismatch_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_extern_array_mismatch_spelling_strict.c",
        note="strict frontier: extern-array mismatch diagnostics JSON should preserve remapped file/line under #line",
        expected_codes=(2000,),
        expected_line=11502,
        expected_column=12,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_include_extern_array_mismatch_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_include_extern_array_mismatch_spelling_strict.c",
        note="strict frontier: include extern-array mismatch diagnostics JSON should preserve remapped file/line under #line",
        expected_codes=(2000,),
        expected_line=11602,
        expected_column=12,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_extern_array_def_mismatch_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_extern_array_def_mismatch_spelling_strict.c",
        note="strict frontier: extern-array declaration/definition mismatch diagnostics JSON should preserve remapped file/line under #line",
        expected_codes=(2000,),
        expected_line=12902,
        expected_column=5,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_include_extern_array_def_mismatch_current_physical_pass",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_include_extern_array_def_mismatch_spelling_strict.c",
        note="reduced threshold: include extern-array declaration/definition mismatch diagnostics JSON currently anchors to physical C file line",
        expected_codes=(2000,),
        expected_line=3,
        expected_column=5,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_extern_array_mismatch_first_decl_line_current_physical_pass",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_extern_array_mismatch_first_decl_line_spelling_strict.c",
        note="reduced threshold: extern-array mismatch first-decl placement diagnostics JSON currently anchors to physical remapped redeclaration line",
        expected_codes=(2000,),
        expected_line=3,
        expected_column=12,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="10__probe_diagjson_line_directive_extern_array_mismatch_second_decl_line_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/10__probe_diag_line_directive_extern_array_mismatch_second_decl_line_spelling_strict.c",
        note="strict frontier: extern-array mismatch second-decl placement diagnostics JSON should preserve remapped file/line under #line",
        expected_codes=(2000,),
        expected_line=15201,
        expected_column=12,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_nonvoid_missing_return_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_nonvoid_missing_return_file_presence.c",
        note="strict frontier: #line non-void-missing-return diagjson should preserve file presence",
        expected_codes=(2000,),
        expected_line=7101,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_nonvoid_missing_return_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_nonvoid_missing_return_file_presence.c",
        note="reduced threshold: #line non-void-missing-return diagjson preserves remapped line with sparse location metadata",
        expected_codes=(2000,),
        expected_line=7101,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_include_nonvoid_missing_return_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_include_nonvoid_missing_return_file_presence.c",
        note="strict frontier: include #line non-void-missing-return diagjson should preserve file presence",
        expected_codes=(2000,),
        expected_line=7201,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_include_nonvoid_missing_return_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_include_nonvoid_missing_return_file_presence.c",
        note="reduced threshold: include #line non-void-missing-return diagjson preserves remapped line with sparse location metadata",
        expected_codes=(2000,),
        expected_line=7201,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_too_many_args_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_too_many_args_file_presence.c",
        note="strict frontier: #line prototype-too-many-args diagjson should preserve file presence",
        expected_codes=(2000,),
        expected_line=7306,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_too_many_args_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_too_many_args_file_presence.c",
        note="reduced threshold: #line prototype-too-many-args diagjson preserves remapped line with sparse location metadata",
        expected_codes=(2000,),
        expected_line=7306,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_include_too_many_args_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_include_too_many_args_file_presence.c",
        note="strict frontier: include #line prototype-too-many-args diagjson should preserve file presence",
        expected_codes=(2000,),
        expected_line=7402,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_include_too_many_args_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_include_too_many_args_file_presence.c",
        note="reduced threshold: include #line prototype-too-many-args diagjson preserves remapped line with sparse location metadata",
        expected_codes=(2000,),
        expected_line=7402,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_too_few_args_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_too_few_args_file_presence.c",
        note="strict frontier: #line prototype-too-few-args diagjson should preserve file presence",
        expected_codes=(2000,),
        expected_line=8506,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_too_few_args_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_too_few_args_file_presence.c",
        note="reduced threshold: #line prototype-too-few-args diagjson preserves remapped line with sparse location metadata",
        expected_codes=(2000,),
        expected_line=8506,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_include_too_few_args_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_include_too_few_args_file_presence.c",
        note="strict frontier: include #line prototype-too-few-args diagjson should preserve file presence",
        expected_codes=(2000,),
        expected_line=8602,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_include_too_few_args_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_include_too_few_args_file_presence.c",
        note="reduced threshold: include #line prototype-too-few-args diagjson preserves remapped line with sparse location metadata",
        expected_codes=(2000,),
        expected_line=8602,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_argument_type_mismatch_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_argument_type_mismatch_file_presence.c",
        note="strict frontier: #line argument-type-mismatch diagjson should preserve file presence",
        expected_codes=(2000,),
        expected_line=8706,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_argument_type_mismatch_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_argument_type_mismatch_file_presence.c",
        note="reduced threshold: #line argument-type-mismatch diagjson preserves remapped line with sparse location metadata",
        expected_codes=(2000,),
        expected_line=8706,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_include_argument_type_mismatch_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_include_argument_type_mismatch_file_presence.c",
        note="strict frontier: include #line argument-type-mismatch diagjson should preserve file presence",
        expected_codes=(2000,),
        expected_line=8802,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_include_argument_type_mismatch_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_include_argument_type_mismatch_file_presence.c",
        note="reduced threshold: include #line argument-type-mismatch diagjson preserves remapped line with sparse location metadata",
        expected_codes=(2000,),
        expected_line=8802,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_return_type_mismatch_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_return_type_mismatch_file_presence.c",
        note="strict frontier: #line return-type-mismatch diagjson should preserve file presence",
        expected_codes=(2000,),
        expected_line=9302,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_return_type_mismatch_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_return_type_mismatch_file_presence.c",
        note="reduced threshold: #line return-type-mismatch diagjson preserves remapped line with sparse location metadata",
        expected_codes=(2000,),
        expected_line=9302,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_include_return_type_mismatch_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_include_return_type_mismatch_file_presence.c",
        note="strict frontier: include #line return-type-mismatch diagjson should preserve file presence",
        expected_codes=(2000,),
        expected_line=9501,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_include_return_type_mismatch_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_include_return_type_mismatch_file_presence.c",
        note="reduced threshold: include #line return-type-mismatch diagjson preserves remapped line with sparse location metadata",
        expected_codes=(2000,),
        expected_line=9501,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_void_return_value_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_void_return_value_file_presence.c",
        note="strict frontier: #line void-return-value diagjson should preserve file presence",
        expected_codes=(2000,),
        expected_line=9402,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_void_return_value_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_void_return_value_file_presence.c",
        note="reduced threshold: #line void-return-value diagjson preserves remapped line with sparse location metadata",
        expected_codes=(2000,),
        expected_line=9402,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_include_void_return_value_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_include_void_return_value_file_presence.c",
        note="strict frontier: include #line void-return-value diagjson should preserve file presence",
        expected_codes=(2000,),
        expected_line=9601,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_include_void_return_value_current_sparse_pass",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_include_void_return_value_file_presence.c",
        note="reduced threshold: include #line void-return-value diagjson preserves remapped line with sparse location metadata",
        expected_codes=(2000,),
        expected_line=9601,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_fnptr_assign_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_fnptr_assign_file_presence.c",
        note="strict frontier: #line function-pointer assignment diagjson should preserve file presence",
        expected_codes=(2000,),
        expected_line=12317,
        expected_column=5,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_include_fnptr_assign_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_include_fnptr_assign_file_presence.c",
        note="strict frontier: include #line function-pointer assignment diagjson should preserve file presence",
        expected_codes=(2000,),
        expected_line=12416,
        expected_column=5,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_multitu_too_many_args_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_multitu_too_many_args_file_presence.c",
        note="strict frontier: multi-TU #line too-many-args diagjson should preserve remapped line and file presence",
        expected_codes=(2000,),
        expected_line=13304,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_too_many_args_file_presence.c",
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_too_many_args_file_presence_lib.c",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_multitu_include_too_many_args_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_multitu_include_too_many_args_file_presence.c",
        note="strict frontier: multi-TU include #line too-many-args diagjson should preserve remapped line and file presence",
        expected_codes=(2000,),
        expected_line=13402,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_include_too_many_args_file_presence.c",
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_include_too_many_args_file_presence_lib.c",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_multitu_parserdiag_decl_missing_rparen_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_multitu_parserdiag_decl_missing_rparen_presence.c",
        note="strict frontier: multi-TU #line parser-declarator recovery should preserve parser diagjson presence",
        expected_codes=(1000,),
        expected_line=13501,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_parserdiag_decl_missing_rparen_presence.c",
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_parserdiag_decl_missing_rparen_presence_lib.c",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_multitu_include_parserdiag_decl_missing_rparen_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_multitu_include_parserdiag_decl_missing_rparen_presence.c",
        note="strict frontier: multi-TU include #line parser-declarator recovery should preserve parser diagjson presence",
        expected_codes=(1000,),
        expected_line=13601,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_include_parserdiag_decl_missing_rparen_presence.c",
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_include_parserdiag_decl_missing_rparen_presence_lib.c",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_multitu_fnptr_assign_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_multitu_fnptr_assign_file_presence.c",
        note="strict frontier: multi-TU #line function-pointer assignment diagjson should preserve remapped line and file presence",
        expected_codes=(2000,),
        expected_line=14507,
        expected_column=5,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_fnptr_assign_file_presence.c",
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_fnptr_assign_file_presence_lib.c",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_multitu_include_fnptr_assign_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_multitu_include_fnptr_assign_file_presence.c",
        note="strict frontier: multi-TU include #line function-pointer assignment diagjson should preserve remapped line and file presence",
        expected_codes=(2000,),
        expected_line=14606,
        expected_column=5,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_include_fnptr_assign_file_presence.c",
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_include_fnptr_assign_file_presence_lib.c",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_multitu_return_type_mismatch_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_multitu_return_type_mismatch_file_presence.c",
        note="strict frontier: multi-TU #line return-type-mismatch diagjson should preserve remapped line and file presence",
        expected_codes=(2000,),
        expected_line=14752,
        expected_column=12,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_return_type_mismatch_file_presence.c",
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_return_type_mismatch_file_presence_lib.c",
        ),
    ),
    DiagnosticJsonProbe(
        probe_id="11__probe_diagjson_line_directive_multitu_include_return_type_mismatch_file_presence_reject",
        source=PROBE_DIR
        / "diagnostics/11__probe_diagjson_line_directive_multitu_include_return_type_mismatch_file_presence.c",
        note="strict frontier: multi-TU include #line return-type-mismatch diagjson should preserve remapped line and file presence",
        expected_codes=(2000,),
        expected_line=14852,
        expected_column=12,
        expected_has_file=True,
        inputs=(
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_include_return_type_mismatch_file_presence.c",
            PROBE_DIR
            / "diagnostics/11__probe_diagjson_line_directive_multitu_include_return_type_mismatch_file_presence_lib.c",
        ),
    ),
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
        probe_id="12__probe_diagjson_line_directive_for_missing_semicolon_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_for_missing_semicolon_parser_presence.c",
        note="strict frontier: #line malformed for-header should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=8402,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_for_missing_semicolon_current_semantic_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_for_missing_semicolon_parser_presence.c",
        note="post-fix lane: #line malformed for-header diagnostics JSON includes parser+semantic payload",
        expected_codes=(2000,),
        expected_line=8402,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_for_missing_semicolon_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_for_missing_semicolon_parser_presence.c",
        note="strict frontier: include #line malformed for-header should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=8502,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_for_missing_semicolon_current_semantic_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_for_missing_semicolon_parser_presence.c",
        note="post-fix lane: include #line malformed for-header diagnostics JSON includes parser+semantic payload",
        expected_codes=(2000,),
        expected_line=8502,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_for_missing_rparen_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_for_missing_rparen_parser_presence.c",
        note="strict frontier: #line for-missing-')' should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=8922,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_for_missing_rparen_current_parser_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_for_missing_rparen_parser_presence.c",
        note="reduced threshold: #line for-missing-')' diagnostics JSON preserves parser payload only",
        expected_codes=(1000,),
        expected_line=8922,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_for_missing_rparen_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_for_missing_rparen_parser_presence.c",
        note="strict frontier: include #line for-missing-')' should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=8932,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_for_missing_rparen_current_parser_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_for_missing_rparen_parser_presence.c",
        note="reduced threshold: include #line for-missing-')' diagnostics JSON preserves parser payload only",
        expected_codes=(1000,),
        expected_line=8932,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_if_missing_rparen_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_if_missing_rparen_parser_presence.c",
        note="strict frontier: #line if-missing-')' should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=8603,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_if_missing_rparen_current_parser_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_if_missing_rparen_parser_presence.c",
        note="reduced threshold: #line if-missing-')' diagnostics JSON preserves parser payload only",
        expected_codes=(1000,),
        expected_line=8603,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_if_missing_rparen_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_if_missing_rparen_parser_presence.c",
        note="strict frontier: include #line if-missing-')' should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=8613,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_if_missing_rparen_current_parser_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_if_missing_rparen_parser_presence.c",
        note="reduced threshold: include #line if-missing-')' diagnostics JSON preserves parser payload only",
        expected_codes=(1000,),
        expected_line=8613,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_while_missing_lparen_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_while_missing_lparen_parser_presence.c",
        note="strict frontier: #line while-missing-'(' should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=8703,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_while_missing_lparen_current_parser_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_while_missing_lparen_parser_presence.c",
        note="reduced threshold: #line while-missing-'(' diagnostics JSON preserves parser payload only",
        expected_codes=(1000,),
        expected_line=8703,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_while_missing_lparen_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_while_missing_lparen_parser_presence.c",
        note="strict frontier: include #line while-missing-'(' should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=8713,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_while_missing_lparen_current_parser_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_while_missing_lparen_parser_presence.c",
        note="reduced threshold: include #line while-missing-'(' diagnostics JSON preserves parser payload only",
        expected_codes=(1000,),
        expected_line=8713,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_do_while_missing_semicolon_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_do_while_missing_semicolon_parser_presence.c",
        note="strict frontier: #line do-while-missing-';' should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=8806,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_do_while_missing_semicolon_current_parser_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_do_while_missing_semicolon_parser_presence.c",
        note="reduced threshold: #line do-while-missing-';' diagnostics JSON preserves parser payload only",
        expected_codes=(1000,),
        expected_line=8806,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_do_while_missing_semicolon_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_do_while_missing_semicolon_parser_presence.c",
        note="strict frontier: include #line do-while-missing-';' should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=8816,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_do_while_missing_semicolon_current_parser_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_do_while_missing_semicolon_parser_presence.c",
        note="reduced threshold: include #line do-while-missing-';' diagnostics JSON preserves parser payload only",
        expected_codes=(1000,),
        expected_line=8816,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_switch_missing_rparen_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_switch_missing_rparen_parser_presence.c",
        note="strict frontier: #line switch-missing-')' should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=8903,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_switch_missing_rparen_current_semantic_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_switch_missing_rparen_parser_presence.c",
        note="post-fix lane: #line switch-missing-')' diagnostics JSON includes parser+semantic payload",
        expected_codes=(2000,),
        expected_line=8901,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_switch_missing_rparen_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_switch_missing_rparen_parser_presence.c",
        note="strict frontier: include #line switch-missing-')' should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=8913,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_switch_missing_rparen_current_semantic_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_switch_missing_rparen_parser_presence.c",
        note="post-fix lane: include #line switch-missing-')' diagnostics JSON includes parser+semantic payload",
        expected_codes=(2000,),
        expected_line=8911,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_if_missing_body_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_if_missing_body_parser_presence.c",
        note="strict frontier: #line if-missing-body should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=9103,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_if_missing_body_current_parser_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_if_missing_body_parser_presence.c",
        note="reduced threshold: #line if-missing-body diagnostics JSON preserves parser payload only",
        expected_codes=(1000,),
        expected_line=9103,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_if_missing_body_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_if_missing_body_parser_presence.c",
        note="strict frontier: include #line if-missing-body should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=9113,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_if_missing_body_current_parser_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_if_missing_body_parser_presence.c",
        note="reduced threshold: include #line if-missing-body diagnostics JSON preserves parser payload only",
        expected_codes=(1000,),
        expected_line=9113,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_while_missing_body_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_while_missing_body_parser_presence.c",
        note="strict frontier: #line while-missing-body should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=9203,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_while_missing_body_current_parser_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_while_missing_body_parser_presence.c",
        note="reduced threshold: #line while-missing-body diagnostics JSON preserves parser payload only",
        expected_codes=(1000,),
        expected_line=9203,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_while_missing_body_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_while_missing_body_parser_presence.c",
        note="strict frontier: include #line while-missing-body should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=9213,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_while_missing_body_current_parser_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_while_missing_body_parser_presence.c",
        note="reduced threshold: include #line while-missing-body diagnostics JSON preserves parser payload only",
        expected_codes=(1000,),
        expected_line=9213,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_for_missing_body_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_for_missing_body_parser_presence.c",
        note="strict frontier: #line for-missing-body should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=9303,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_for_missing_body_current_parser_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_for_missing_body_parser_presence.c",
        note="reduced threshold: #line for-missing-body diagnostics JSON preserves parser payload only",
        expected_codes=(1000,),
        expected_line=9303,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_for_missing_body_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_for_missing_body_parser_presence.c",
        note="strict frontier: include #line for-missing-body should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=9313,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_diagjson_line_directive_include_for_missing_body_current_parser_only_pass",
        source=PROBE_DIR
        / "diagnostics/12__probe_diagjson_line_directive_include_for_missing_body_parser_presence.c",
        note="reduced threshold: include #line for-missing-body diagnostics JSON preserves parser payload only",
        expected_codes=(1000,),
        expected_line=9313,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_line_directive_if_missing_lparen_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_if_missing_lparen_parser_presence.c",
        note="reduced threshold: #line if-missing-'(' currently emits semantic-only diagjson payload at remapped function location",
        expected_codes=(2000,),
        expected_line=132001,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_line_directive_include_if_missing_lparen_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_include_if_missing_lparen_parser_presence.c",
        note="reduced threshold: include #line if-missing-'(' currently emits semantic-only diagjson payload at remapped function location",
        expected_codes=(2000,),
        expected_line=132051,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_line_directive_switch_missing_rparen_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_switch_missing_rparen_parser_presence.c",
        note="strict frontier: #line switch-missing-')' should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=132103,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_line_directive_include_switch_missing_rparen_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_include_switch_missing_rparen_parser_presence.c",
        note="strict frontier: include #line switch-missing-')' should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=132153,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_line_directive_if_missing_body_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_if_missing_body_parser_presence.c",
        note="strict frontier: #line if-missing-body should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=134003,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_line_directive_include_if_missing_body_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_include_if_missing_body_parser_presence.c",
        note="strict frontier: include #line if-missing-body should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=134053,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_line_directive_while_missing_body_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_while_missing_body_parser_presence.c",
        note="strict frontier: #line while-missing-body should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=134103,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_line_directive_include_while_missing_body_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_include_while_missing_body_parser_presence.c",
        note="strict frontier: include #line while-missing-body should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=134153,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_line_directive_for_missing_body_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_for_missing_body_parser_presence.c",
        note="strict frontier: #line for-missing-body should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=134203,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_line_directive_include_for_missing_body_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_include_for_missing_body_parser_presence.c",
        note="strict frontier: include #line for-missing-body should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=134253,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_line_directive_switch_missing_body_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_switch_missing_body_parser_presence.c",
        note="strict frontier: #line switch-missing-body should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=134303,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_line_directive_include_switch_missing_body_parser_presence_reject",
        source=PROBE_DIR
        / "diagnostics/13__probe_diagjson_line_directive_include_switch_missing_body_parser_presence.c",
        note="strict frontier: include #line switch-missing-body should preserve parser diagnostics JSON presence",
        expected_codes=(1000,),
        expected_line=134353,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_if_missing_lparen",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_if_missing_lparen.c",
        note="diagnostics JSON should be emitted for missing '(' in if statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_while_missing_lparen",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_while_missing_lparen.c",
        note="diagnostics JSON should be emitted for missing '(' in while statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_do_while_missing_lparen",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_do_while_missing_lparen.c",
        note="diagnostics JSON should be emitted for missing '(' in do-while condition parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_for_missing_lparen",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_for_missing_lparen.c",
        note="diagnostics JSON should be emitted for missing '(' in for statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_if_missing_rparen",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_if_missing_rparen.c",
        note="diagnostics JSON should be emitted for missing ')' in if statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_while_missing_rparen",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_while_missing_rparen.c",
        note="diagnostics JSON should be emitted for missing ')' in while statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_do_while_missing_rparen",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_do_while_missing_rparen.c",
        note="diagnostics JSON should be emitted for missing ')' in do-while condition parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_for_missing_rparen",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_for_missing_rparen.c",
        note="diagnostics JSON should be emitted for missing ')' in for statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_for_missing_first_semicolon",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_for_missing_first_semicolon.c",
        note="diagnostics JSON should be emitted for missing first ';' in for header",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_for_missing_second_semicolon",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_for_missing_second_semicolon.c",
        note="diagnostics JSON should be emitted for missing second ';' in for header",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_for_missing_second_semicolon_simple",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_for_missing_second_semicolon_simple.c",
        note="diagnostics JSON should be emitted for missing second ';' in minimal for-header form",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_if_missing_condition",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_if_missing_condition.c",
        note="diagnostics JSON should be emitted for missing condition expression in if statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_do_while_missing_while",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_do_while_missing_while.c",
        note="diagnostics JSON should be emitted for missing 'while' in do-while statement",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_do_while_missing_while_simple",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_do_while_missing_while_simple.c",
        note="diagnostics JSON should be emitted for missing 'while' in minimal do-while form",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_do_while_missing_condition",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_do_while_missing_condition.c",
        note="diagnostics JSON should be emitted for missing condition expression in do-while condition parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_do_while_missing_semicolon",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_do_while_missing_semicolon.c",
        note="diagnostics JSON should be emitted for missing ';' after do-while condition",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_for_missing_body",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_for_missing_body.c",
        note="diagnostics JSON should be emitted for missing for-loop body statement",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_while_missing_body",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_while_missing_body.c",
        note="diagnostics JSON should be emitted for missing while-loop body statement",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_do_while_missing_body",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_do_while_missing_body.c",
        note="diagnostics JSON should be emitted for missing do-while loop body statement",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_do_while_missing_body_simple",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_do_while_missing_body_simple.c",
        note="diagnostics JSON should be emitted for minimal missing do-while loop body statement",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_switch_missing_body",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_switch_missing_body.c",
        note="diagnostics JSON should be emitted for missing switch body statement",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_switch_missing_lparen",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_switch_missing_lparen.c",
        note="diagnostics JSON should be emitted for missing '(' in switch statement",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_while_empty_condition",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_while_empty_condition.c",
        note="diagnostics JSON should be emitted for missing while condition expression",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_switch_missing_rparen",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_switch_missing_rparen.c",
        note="diagnostics JSON should be emitted for missing ')' in switch statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_if_missing_then_stmt",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_if_missing_then_stmt.c",
        note="diagnostics JSON should be emitted for missing then-statement in if parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_else_missing_stmt_reject",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_else_missing_stmt_reject.c",
        note="diagnostics JSON should be emitted for missing statement in else parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_switch_missing_lbrace",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_switch_missing_lbrace.c",
        note="diagnostics JSON should be emitted for missing '{' in switch statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_switch_case_missing_colon",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_switch_case_missing_colon.c",
        note="diagnostics JSON should be emitted for missing ':' in switch case label parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id="13__probe_diagjson_parser_switch_default_missing_colon_reject",
        source=PROBE_DIR / "diagnostics/13__probe_diagjson_parser_switch_default_missing_colon.c",
        note="diagnostics JSON should be emitted for missing ':' in switch default label parser recovery",
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
        probe_id="14__probe_diagjson_fnptr_table_incompatible_assign_reject",
        source=PROBE_DIR / "diagnostics/14__probe_diag_fnptr_table_incompatible_assign_reject.c",
        note="diagnostics JSON should include fnptr incompatible-assignment rejection",
        expected_codes=(2000,),
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
        probe_id="15__probe_diagjson_axis1_wave1_multitu_duplicate_definition_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave1_multitu_duplicate_definition_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave1_multitu_duplicate_definition_reject_main.c",
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave1_multitu_duplicate_definition_reject_lib.c",
        ),
        note="axis1 wave1: diagnostics JSON should be exported for multi-TU duplicate data-definition link failures",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave2_multitu_duplicate_weak_strong_conflict_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave2_multitu_duplicate_weak_strong_conflict_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave2_multitu_duplicate_weak_strong_conflict_reject_main.c",
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave2_multitu_duplicate_weak_strong_conflict_reject_lib.c",
        ),
        note="axis1 wave2: diagnostics JSON should be exported for multi-TU duplicate weak/strong conflict link failures",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave3_multitu_duplicate_tentative_matrix_reject",
        source=PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave3_multitu_duplicate_tentative_matrix_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave3_multitu_duplicate_tentative_matrix_reject_main.c",
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave3_multitu_duplicate_tentative_matrix_reject_lib.c",
        ),
        note="axis1 wave3: diagnostics JSON should be exported for multi-TU duplicate tentative-definition link failures",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave3_multitu_duplicate_tentative_matrix_current_empty",
        source=PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave3_multitu_duplicate_tentative_matrix_reject_main.c",
        inputs=(
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave3_multitu_duplicate_tentative_matrix_reject_main.c",
            PROBE_DIR / "diagnostics/15__probe_diag_axis1_wave3_multitu_duplicate_tentative_matrix_reject_lib.c",
        ),
        note="axis1 wave3 current-threshold: diagnostics JSON export may be empty for duplicate tentative-definition matrix under current defaults",
        require_any_diagnostic=False,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave4_multitu_function_data_symbol_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave4_multitu_function_data_symbol_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave4_multitu_function_data_symbol_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave4_multitu_function_data_symbol_conflict_reject_lib.c",
        ),
        note="axis1 wave4: diagnostics JSON should be exported for multi-TU function-vs-data symbol conflicts",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave5_multitu_duplicate_initialized_global_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave5_multitu_duplicate_initialized_global_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave5_multitu_duplicate_initialized_global_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave5_multitu_duplicate_initialized_global_reject_lib.c",
        ),
        note="axis1 wave5: diagnostics JSON should be exported for duplicate initialized-global link conflicts",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave5_multitu_duplicate_tentative_split_current",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave5_multitu_duplicate_tentative_split_current_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave5_multitu_duplicate_tentative_split_current_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave5_multitu_duplicate_tentative_split_current_lib.c",
        ),
        note="axis1 wave5 current-threshold: diagnostics JSON may be empty for duplicate tentative split lane under current defaults",
        require_any_diagnostic=False,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave6_multitu_function_object_alias_partition_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave6_multitu_function_object_alias_partition_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave6_multitu_function_object_alias_partition_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave6_multitu_function_object_alias_partition_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave6_multitu_function_object_alias_partition_conflict_reject_aux.c",
        ),
        note="axis1 wave6: diagnostics JSON should be exported for function-vs-object alias partition link conflicts",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave7_multitu_function_object_alias_perm_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave7_multitu_function_object_alias_perm_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave7_multitu_function_object_alias_perm_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave7_multitu_function_object_alias_perm_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave7_multitu_function_object_alias_perm_conflict_reject_aux.c",
        ),
        note="axis1 wave7: diagnostics JSON should be exported for mixed function-vs-object alias permutation link conflicts",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave8_multitu_dual_function_object_alias_perm_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave8_multitu_dual_function_object_alias_perm_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave8_multitu_dual_function_object_alias_perm_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave8_multitu_dual_function_object_alias_perm_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave8_multitu_dual_function_object_alias_perm_conflict_reject_aux.c",
        ),
        note="axis1 wave8: diagnostics JSON should be exported for dual mixed function-vs-object alias permutation link conflicts",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave9_multitu_function_object_alias_ring_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave9_multitu_function_object_alias_ring_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave9_multitu_function_object_alias_ring_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave9_multitu_function_object_alias_ring_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave9_multitu_function_object_alias_ring_conflict_reject_aux.c",
        ),
        note="axis1 wave9: diagnostics JSON should be exported for mixed function-vs-object alias ring conflicts",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave10_multitu_function_object_alias_checkpoint_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave10_multitu_function_object_alias_checkpoint_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave10_multitu_function_object_alias_checkpoint_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave10_multitu_function_object_alias_checkpoint_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave10_multitu_function_object_alias_checkpoint_conflict_reject_aux.c",
        ),
        note="axis1 wave10: diagnostics JSON should be exported for mixed function-vs-object alias checkpoint conflicts",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave11_multitu_function_object_alias_ptrtable_rebase_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave11_multitu_function_object_alias_ptrtable_rebase_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave11_multitu_function_object_alias_ptrtable_rebase_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave11_multitu_function_object_alias_ptrtable_rebase_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave11_multitu_function_object_alias_ptrtable_rebase_conflict_reject_aux.c",
        ),
        note="axis1 wave11: diagnostics JSON should be exported for mixed function-vs-object alias pointer-table rebase conflicts",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave12_multitu_function_object_alias_linkorder_checkpoint_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave12_multitu_function_object_alias_linkorder_checkpoint_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave12_multitu_function_object_alias_linkorder_checkpoint_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave12_multitu_function_object_alias_linkorder_checkpoint_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave12_multitu_function_object_alias_linkorder_checkpoint_conflict_reject_aux.c",
        ),
        note="axis1 wave12: diagnostics JSON should be exported for mixed function-vs-object alias link-order checkpoint conflicts",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave13_multitu_function_object_alias_four_level_checkpoint_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave13_multitu_function_object_alias_four_level_checkpoint_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave13_multitu_function_object_alias_four_level_checkpoint_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave13_multitu_function_object_alias_four_level_checkpoint_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave13_multitu_function_object_alias_four_level_checkpoint_conflict_reject_aux.c",
        ),
        note="axis1 wave13: diagnostics JSON should be exported for mixed function-vs-object alias four-level checkpoint conflicts",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave14_multitu_function_object_alias_relocation_order_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave14_multitu_function_object_alias_relocation_order_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave14_multitu_function_object_alias_relocation_order_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave14_multitu_function_object_alias_relocation_order_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave14_multitu_function_object_alias_relocation_order_conflict_reject_aux.c",
        ),
        note="axis1 wave14: diagnostics JSON should be exported for mixed function-vs-object alias relocation-order conflicts",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis1_wave15_multitu_function_object_alias_three_plan_replay_conflict_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis1_wave15_multitu_function_object_alias_three_plan_replay_conflict_reject_main.c",
        inputs=(
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave15_multitu_function_object_alias_three_plan_replay_conflict_reject_main.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave15_multitu_function_object_alias_three_plan_replay_conflict_reject_lib.c",
            PROBE_DIR
            / "diagnostics/15__probe_diag_axis1_wave15_multitu_function_object_alias_three_plan_replay_conflict_reject_aux.c",
        ),
        note="axis1 wave15: diagnostics JSON should be exported for mixed function-vs-object alias three-plan replay conflicts",
        expected_codes=(4001,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis2_wave1_line_map_nested_include_macro_chain_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis2_wave1_line_map_nested_include_macro_chain_reject.c",
        note="axis2 wave1: diagnostics JSON should preserve line/file presence for nested include+macro #line remap lane",
        expected_codes=(2000,),
        expected_line=300299,
        expected_column=33,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_axis2_wave2_line_map_nested_include_macro_guard_reentry_reject",
        source=PROBE_DIR
        / "diagnostics/14__probe_diag_axis2_wave2_line_map_nested_include_macro_guard_reentry_reject.c",
        note="axis2 wave2: diagnostics JSON should preserve deep line/file presence for nested include+macro guard reentry #line remap lane",
        expected_codes=(2000,),
        expected_line=240308,
        expected_column=9,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis2_wave2_line_map_nested_include_macro_function_chain_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis2_wave2_line_map_nested_include_macro_function_chain_reject.c",
        note="axis2 wave2: diagnostics JSON should preserve deep line/file presence for nested include+macro function-chain #line remap lane",
        expected_codes=(2000,),
        expected_line=340304,
        expected_column=12,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_axis2_wave3_line_map_nested_include_macro_type_bridge_reject",
        source=PROBE_DIR
        / "diagnostics/14__probe_diag_axis2_wave3_line_map_nested_include_macro_type_bridge_reject.c",
        note="axis2 wave3: diagnostics JSON should preserve deep line/file presence for four-layer include+macro type-bridge #line remap lane",
        expected_codes=(2000,),
        expected_line=240697,
        expected_column=37,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis2_wave3_line_map_nested_include_macro_arity_bridge_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis2_wave3_line_map_nested_include_macro_arity_bridge_reject.c",
        note="axis2 wave3: diagnostics JSON should preserve deep line/file presence for four-layer include+macro arity-bridge #line remap lane",
        expected_codes=(2000,),
        expected_line=260697,
        expected_column=38,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="14__probe_diagjson_axis2_wave4_line_map_nested_include_macro_arity_overflow_reject",
        source=PROBE_DIR
        / "diagnostics/14__probe_diag_axis2_wave4_line_map_nested_include_macro_arity_overflow_reject.c",
        note="axis2 wave4: diagnostics JSON should preserve deep line/file presence for five-layer include+macro arity-overflow #line remap lane",
        expected_codes=(2000,),
        expected_line=320897,
        expected_column=32,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis2_wave4_line_map_nested_include_macro_token_bridge_reject",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis2_wave4_line_map_nested_include_macro_token_bridge_reject.c",
        note="axis2 wave4: diagnostics JSON should preserve deep line/file presence for token-paste include+macro bridge #line remap lane",
        expected_codes=(2000,),
        expected_line=340798,
        expected_column=34,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis4_wave1_malformed_include_guard_reentry_depth_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave1_malformed_include_guard_reentry_depth_no_crash.c",
        note="axis4 wave1: diagnostics JSON should preserve deep line/file presence for include-guard reentry depth lane",
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis4_wave2_malformed_include_guard_cycle_reentry_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave2_malformed_include_guard_cycle_reentry_no_crash.c",
        note="axis4 wave2: diagnostics JSON should preserve file-presence fields for include-guard contradiction cycle diagnostics",
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis4_wave3_malformed_include_gnu_comma_vaargs_reentry_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave3_malformed_include_gnu_comma_vaargs_reentry_no_crash.c",
        note="axis4 wave3: diagnostics JSON should preserve file-presence fields for include-graph GNU comma-paste rejection",
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis4_wave4_line_map_deep_missing_symbol_reentry_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave4_line_map_deep_missing_symbol_reentry_no_crash.c",
        note="axis4 wave4: diagnostics JSON should preserve deep virtual file-presence for line-map semantic failure lanes",
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis4_wave5_conditional_active_branch_arity_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave5_conditional_active_branch_arity_fail_no_crash.c",
        note="axis4 wave5: diagnostics JSON should preserve file-presence fields for active-branch include reentry arity failures",
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis4_wave6_macro_state_undef_rebind_reentry_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave6_macro_state_undef_rebind_reentry_no_crash.c",
        note="axis4 wave6: diagnostics JSON should preserve file-presence fields for macro-state churn include reentry failures",
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis4_wave7_include_order_permute_b_arity_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave7_include_order_permute_b_arity_fail_no_crash.c",
        note="axis4 wave7: diagnostics JSON should preserve file-presence fields across include-order permutation arity failures",
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis4_wave8_depth_conditional_active_branch_matrix_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave8_depth_conditional_active_branch_matrix_fail_no_crash.c",
        note="axis4 wave8: diagnostics JSON should preserve file-presence fields for depth-amplified active-branch conditional failures",
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis4_wave9_macro_alias_permute_b_arity_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave9_macro_alias_permute_b_arity_fail_no_crash.c",
        note="axis4 wave9: diagnostics JSON should preserve file-presence fields across macro-alias permutation failures",
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis4_wave10_branch_convergence_winner_arity_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave10_branch_convergence_winner_arity_fail_no_crash.c",
        note="axis4 wave10: diagnostics JSON should preserve file-presence fields for converged macro-state winner failures",
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_axis4_wave11_dual_convergence_order_b_winner_arity_fail_no_crash",
        source=PROBE_DIR
        / "diagnostics/15__probe_diag_axis4_wave11_dual_convergence_order_b_winner_arity_fail_no_crash.c",
        note="axis4 wave11: diagnostics JSON should preserve file-presence fields for dual convergence order-B winner failures",
        expected_has_file=True,
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
        probe_id="15__probe_diagjson_pp_macro_paste_variadic_comma_extension_no_diag",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_macro_paste_fragments_no_crash.c",
        note="GNU-style ', ##__VA_ARGS__' extension lane should export preprocessing diagnostics JSON",
        expected_codes=(3000,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_pp_macro_paste_operator_tail_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_macro_paste_operator_tail_no_crash.c",
        note="operator-tail token-paste malformed lane should export preprocessing diagnostics JSON",
        expected_codes=(3000,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_pp_gnu_comma_vaargs_chain_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_gnu_comma_vaargs_chain_no_crash.c",
        note="GNU comma-vaargs nested chain should export preprocessing diagnostics JSON",
        expected_codes=(3000,),
    ),
    DiagnosticJsonProbe(
        probe_id="15__probe_diagjson_malformed_pp_token_paste_variadic_tail_no_crash",
        source=PROBE_DIR / "diagnostics/15__probe_diag_malformed_pp_token_paste_variadic_tail_no_crash.c",
        note="variadic token-paste tail lane should export preprocessing diagnostics JSON",
        expected_codes=(3000,),
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
    if probe.expect_any_diagnostic:
        if has_diag:
            return ("RESOLVED", "diagnostic now emitted", "")
        return ("BLOCKED", "diagnostic missing", "")
    if has_diag:
        return ("BLOCKED", "unexpected diagnostic emitted", "")
    return ("RESOLVED", "no diagnostic emitted (expected for this lane)", "")


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
                if probe.expected_column is not None:
                    actual_columns = [int(item.get("column", -1)) for item in diagnostics]
                    if probe.expected_column not in actual_columns:
                        return (
                            "BLOCKED",
                            f"diagnostics JSON missing expected column {probe.expected_column}",
                            "",
                        )
                if probe.expected_has_file is not None:
                    actual_has_file = [bool(item.get("has_file", False)) for item in diagnostics]
                    if probe.expected_has_file not in actual_has_file:
                        return (
                            "BLOCKED",
                            f"diagnostics JSON missing expected has_file={probe.expected_has_file}",
                            "",
                        )
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
