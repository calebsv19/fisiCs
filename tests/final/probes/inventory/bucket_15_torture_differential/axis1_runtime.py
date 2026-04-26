from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='15__probe_axis1_wave1_multitu_fn_data_symbol_interleave_reloc_hash',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave1_multitu_fn_data_symbol_interleave_reloc_hash_main.c',
        note='axis1 wave1: multi-TU function+data symbol interleave relocation hash should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave1_multitu_fn_data_symbol_interleave_reloc_hash_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave1_multitu_fn_data_symbol_interleave_reloc_hash_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave1_multitu_fn_data_symbol_interleave_reloc_hash_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave2_multitu_common_vs_nocommon_emission_hash',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave2_multitu_common_vs_nocommon_emission_hash_main.c',
        note='axis1 wave2: multi-TU common-vs-nocommon symbol emission hash should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave2_multitu_common_vs_nocommon_emission_hash_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave2_multitu_common_vs_nocommon_emission_hash_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave2_multitu_common_vs_nocommon_emission_hash_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave3_multitu_symbol_partition_crc_bridge',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave3_multitu_symbol_partition_crc_bridge_main.c',
        note='axis1 wave3: multi-TU symbol partition CRC bridge should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave3_multitu_symbol_partition_crc_bridge_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave3_multitu_symbol_partition_crc_bridge_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave3_multitu_symbol_partition_crc_bridge_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave4_multitu_symbol_weight_fold_hash',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave4_multitu_symbol_weight_fold_hash_main.c',
        note='axis1 wave4: multi-TU symbol-weight fold hash should remain clang-parity stable',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave4_multitu_symbol_weight_fold_hash_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave4_multitu_symbol_weight_fold_hash_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave4_multitu_symbol_weight_fold_hash_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave5_multitu_symbol_owner_partition_reloc_chain',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave5_multitu_symbol_owner_partition_reloc_chain_main.c',
        note='axis1 wave5: multi-TU symbol-owner partition relocation chain should remain clang-parity stable',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave5_multitu_symbol_owner_partition_reloc_chain_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave5_multitu_symbol_owner_partition_reloc_chain_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave5_multitu_symbol_owner_partition_reloc_chain_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave6_multitu_owner_hop_rebase_hash',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_main.c',
        note='axis1 wave6: owner-hop pointer-indirection relocation hash should remain clang-parity stable',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_reduced',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_reduced_main.c',
        note='axis1 wave6 reduced: minimal owner-hop pointer-indirection chain should remain clang-parity stable',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_reduced_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_reduced_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave6_multitu_owner_hop_rebase_hash_reduced_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_main.c',
        note='axis1 wave7: owner-hop permutation depth hash over two-level pointer tables should remain clang-parity stable',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_current',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_current_main.c',
        note='axis1 wave7 current-threshold: single-lane owner-hop chain remains clang-parity stable while strict permutation-depth lane is blocked',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_current_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_current_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave7_multitu_owner_hop_permutation_depth_hash_current_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave8_multitu_owner_hop_routeptr_offset_depth_hash',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_offset_depth_hash_main.c',
        note='axis1 wave8: offset-heavy owner-hop route-pointer depth hash should remain clang-parity stable',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_offset_depth_hash_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_offset_depth_hash_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_offset_depth_hash_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave8_multitu_owner_hop_routeptr_nooffset_matrix',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_nooffset_matrix_main.c',
        note='axis1 wave8: no-offset owner-hop route-pointer matrix should remain clang-parity stable',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_nooffset_matrix_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_nooffset_matrix_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave8_multitu_owner_hop_routeptr_nooffset_matrix_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave9_multitu_owner_ring_route_hash',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave9_multitu_owner_ring_route_hash_main.c',
        note='axis1 wave9: owner-ring route hash over nested pointer tables should remain clang-parity stable',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave9_multitu_owner_ring_route_hash_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave9_multitu_owner_ring_route_hash_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave9_multitu_owner_ring_route_hash_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave10_multitu_owner_checkpoint_route_hash',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave10_multitu_owner_checkpoint_route_hash_main.c',
        note='axis1 wave10: owner-checkpoint route hash over partitioned plans should remain clang-parity stable',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave10_multitu_owner_checkpoint_route_hash_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave10_multitu_owner_checkpoint_route_hash_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave10_multitu_owner_checkpoint_route_hash_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave11_multitu_ptrtable_nested_decay_route_hash',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave11_multitu_ptrtable_nested_decay_route_hash_main.c',
        note='axis1 wave11: nested decay pointer-table route hash should remain clang-parity stable',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave11_multitu_ptrtable_nested_decay_route_hash_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave11_multitu_ptrtable_nested_decay_route_hash_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave11_multitu_ptrtable_nested_decay_route_hash_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave12_multitu_ptrtable_linkorder_checkpoint_hash',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave12_multitu_ptrtable_linkorder_checkpoint_hash_main.c',
        note='axis1 wave12: link-order checkpoint pointer-table hash should remain clang-parity stable',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave12_multitu_ptrtable_linkorder_checkpoint_hash_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave12_multitu_ptrtable_linkorder_checkpoint_hash_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave12_multitu_ptrtable_linkorder_checkpoint_hash_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave13_multitu_ptrtable_four_level_route_hash',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave13_multitu_ptrtable_four_level_route_hash_main.c',
        note='axis1 wave13: four-level pointer-table route hash should remain clang-parity stable',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave13_multitu_ptrtable_four_level_route_hash_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave13_multitu_ptrtable_four_level_route_hash_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave13_multitu_ptrtable_four_level_route_hash_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave14_multitu_ptrtable_relocation_order_hash',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave14_multitu_ptrtable_relocation_order_hash_main.c',
        note='axis1 wave14: relocation-order checkpoint hash should remain clang-parity stable',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave14_multitu_ptrtable_relocation_order_hash_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave14_multitu_ptrtable_relocation_order_hash_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave14_multitu_ptrtable_relocation_order_hash_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_axis1_wave15_multitu_tu_order_flip_route_hash',
        source=PROBE_DIR / 'runtime/15__probe_axis1_wave15_multitu_tu_order_flip_route_hash_main.c',
        note='axis1 wave15: TU-order flip route hash should remain clang-parity stable',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis1_wave15_multitu_tu_order_flip_route_hash_main.c', PROBE_DIR / 'runtime/15__probe_axis1_wave15_multitu_tu_order_flip_route_hash_lib.c', PROBE_DIR / 'runtime/15__probe_axis1_wave15_multitu_tu_order_flip_route_hash_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_external_decl_chain_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_external_decl_chain_smoke.c',
        note='external-corpus declaration-chain fragment should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_external_typedef_graph_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_external_typedef_graph_smoke.c',
        note='external-corpus typedef-graph fragment should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_pinned_fragment_e_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_pinned_fragment_e_smoke.c',
        note='pinned corpus fragment E should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_pinned_fragment_f_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_pinned_fragment_f_smoke.c',
        note='pinned corpus fragment F should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_pinned_fragment_g_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_pinned_fragment_g_smoke.c',
        note='pinned corpus fragment G should compile/run deterministically and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_ast_pathological_type_graph',
        source=PROBE_DIR / 'runtime/15__probe_ast_pathological_type_graph.c',
        note='pathological type-graph compile/runtime lane should remain deterministic and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_ast_pathological_decl_graph_surface',
        source=PROBE_DIR / 'runtime/15__probe_ast_pathological_decl_graph_surface.c',
        note='pathological declaration-graph surface lane should remain deterministic and match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_smoke',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_smoke.c',
        note='deterministic runtime smoke lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_control_checksum',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_control_checksum.c',
        note='control checksum lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_multitu_state_bridge',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_state_bridge_main.c',
        note='multi-TU state bridge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_state_bridge_main.c', PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_state_bridge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_abi_args_matrix',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_abi_args_matrix.c',
        note='ABI args matrix lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_loop_state_crc_matrix',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_loop_state_crc_matrix.c',
        note='loop-state CRC matrix lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_struct_array_stride_crc_matrix',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_struct_array_stride_crc_matrix.c',
        note='struct-array stride CRC lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_pointer_mix_checksum_matrix',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_pointer_mix_checksum_matrix.c',
        note='pointer-mix checksum matrix lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_multitu_fnptr_table_bridge',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_fnptr_table_bridge_main.c',
        note='multi-TU fnptr table bridge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_fnptr_table_bridge_main.c', PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_fnptr_table_bridge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_multitu_const_seed_pipeline',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_const_seed_pipeline_main.c',
        note='multi-TU const-seed pipeline lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_const_seed_pipeline_main.c', PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_const_seed_pipeline_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_multitu_layout_digest_bridge',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_layout_digest_bridge_main.c',
        note='multi-TU layout digest bridge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_layout_digest_bridge_main.c', PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_multitu_layout_digest_bridge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_policy_shift_char_matrix',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_policy_shift_char_matrix.c',
        note='policy shift/char matrix lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_policy_struct_abi_matrix',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_policy_struct_abi_matrix.c',
        note='policy struct ABI matrix lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_header_control_dispatch_matrix',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_header_control_dispatch_matrix.c',
        note='header control/dispatch matrix lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_header_layout_fold_matrix',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_header_layout_fold_matrix.c',
        note='header layout/fold matrix lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_control_flow_lattice_matrix',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_control_flow_lattice_matrix.c',
        note='control-flow lattice matrix lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_runtime_clang_gcc_tri_diff_abi_variadic_regstack_matrix',
        source=PROBE_DIR / 'runtime/15__probe_runtime_clang_gcc_tri_diff_abi_variadic_regstack_matrix.c',
        note='ABI variadic reg/stack matrix lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
]

DIAG_PROBES = []

DIAG_JSON_PROBES = []
