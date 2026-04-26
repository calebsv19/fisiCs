from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='15__probe_axis3_wave1_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave1_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_edge_main.c',
        note='axis3 wave1: multi-TU variadic struct-return edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave1_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave1_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave1_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave1_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_edge.c',
        note='axis3 wave1: long-double reg/stack callchain edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_permute_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_permute_edge_main.c',
        note='axis3 wave2: multi-TU variadic struct-return permutation edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_permute_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_permute_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_permute_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_permute_edge.c',
        note='axis3 wave2: long-double reg/stack callchain permutation edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave3_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_crossmix_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave3_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_crossmix_edge_main.c',
        note='axis3 wave3: multi-TU variadic struct-return crossmix edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave3_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_crossmix_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave3_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_crossmix_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave3_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_crossmix_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave3_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_crossmix_edge.c',
        note='axis3 wave3: long-double reg/stack callchain crossmix edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave4_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_pressure_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave4_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_pressure_edge_main.c',
        note='axis3 wave4: multi-TU variadic struct-return reg/stack pressure edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave4_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_pressure_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave4_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_pressure_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave4_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_regstack_pressure_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave4_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_regstack_pressure_edge.c',
        note='axis3 wave4: long-double reg/stack callchain pressure edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave5_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_rotation_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave5_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_rotation_edge_main.c',
        note='axis3 wave5: multi-TU variadic struct-return checkpoint-rotation edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave5_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_rotation_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave5_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_rotation_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave5_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_checkpoint_rotation_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave5_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_checkpoint_rotation_edge.c',
        note='axis3 wave5: long-double reg/stack callchain checkpoint-rotation edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave6_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_frontier_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave6_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_frontier_edge_main.c',
        note='axis3 wave6: multi-TU variadic struct-return epoch/frontier edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave6_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_frontier_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave6_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_frontier_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave6_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_epoch_frontier_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave6_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_epoch_frontier_edge.c',
        note='axis3 wave6: long-double reg/stack callchain epoch/frontier edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave7_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_replay_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave7_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_replay_edge_main.c',
        note='axis3 wave7: multi-TU variadic struct-return frontier/replay edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave7_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_replay_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave7_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_replay_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave7_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_frontier_replay_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave7_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_frontier_replay_edge.c',
        note='axis3 wave7: long-double reg/stack callchain frontier/replay edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave8_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_handoff_permute_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave8_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_handoff_permute_edge_main.c',
        note='axis3 wave8: multi-TU variadic struct-return handoff/permutation edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave8_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_handoff_permute_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave8_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_handoff_permute_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave8_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_handoff_permute_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave8_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_handoff_permute_edge.c',
        note='axis3 wave8: long-double reg/stack callchain handoff/permutation edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave9_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_handoff_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave9_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_handoff_edge_main.c',
        note='axis3 wave9: multi-TU variadic struct-return epoch/handoff edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave9_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_handoff_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave9_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_handoff_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave9_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_epoch_handoff_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave9_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_epoch_handoff_edge.c',
        note='axis3 wave9: long-double reg/stack callchain epoch/handoff edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave10_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave10_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_fold_edge_main.c',
        note='axis3 wave10: multi-TU variadic struct-return frontier/fold edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave10_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_fold_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave10_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_fold_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave10_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_frontier_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave10_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_frontier_fold_edge.c',
        note='axis3 wave10: long-double reg/stack callchain frontier/fold edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave11_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave11_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_fold_edge_main.c',
        note='axis3 wave11: multi-TU variadic struct-return checkpoint/fold edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave11_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_fold_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave11_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_checkpoint_fold_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave11_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_checkpoint_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave11_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_checkpoint_fold_edge.c',
        note='axis3 wave11: long-double reg/stack callchain checkpoint/fold edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave12_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_rotation_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave12_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_rotation_fold_edge_main.c',
        note='axis3 wave12: multi-TU variadic struct-return rotation/fold edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave12_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_rotation_fold_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave12_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_rotation_fold_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave12_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_rotation_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave12_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_rotation_fold_edge.c',
        note='axis3 wave12: long-double reg/stack callchain rotation/fold edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave13_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_rotation_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave13_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_rotation_fold_edge_main.c',
        note='axis3 wave13: multi-TU variadic struct-return epoch-rotation/fold edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave13_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_rotation_fold_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave13_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_epoch_rotation_fold_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave13_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_epoch_rotation_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave13_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_epoch_rotation_fold_edge.c',
        note='axis3 wave13: long-double reg/stack callchain epoch-rotation/fold edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave14_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_epoch_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave14_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_epoch_fold_edge_main.c',
        note='axis3 wave14: multi-TU variadic struct-return frontier-epoch/fold edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave14_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_epoch_fold_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave14_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_frontier_epoch_fold_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave14_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_frontier_epoch_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave14_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_frontier_epoch_fold_edge.c',
        note='axis3 wave14: long-double reg/stack callchain frontier-epoch/fold edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave15_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_frontier_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave15_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_frontier_fold_edge_main.c',
        note='axis3 wave15: multi-TU variadic struct-return regstack/frontier fold edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave15_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_frontier_fold_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave15_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regstack_frontier_fold_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave15_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_regwindow_frontier_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave15_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_regwindow_frontier_fold_edge.c',
        note='axis3 wave15: long-double reg/stack callchain regwindow/frontier fold edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave16_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regwindow_handoff_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave16_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regwindow_handoff_fold_edge_main.c',
        note='axis3 wave16: multi-TU variadic struct-return regwindow/handoff fold edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave16_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regwindow_handoff_fold_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave16_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_regwindow_handoff_fold_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave16_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_spill_handoff_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave16_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_spill_handoff_fold_edge.c',
        note='axis3 wave16: long-double reg/stack callchain spill/handoff fold edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave17_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_spill_checkpoint_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave17_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_spill_checkpoint_fold_edge_main.c',
        note='axis3 wave17: multi-TU variadic struct-return spill/checkpoint fold edge lane should match both clang and gcc when gcc is available',
        inputs=[PROBE_DIR / 'runtime/15__probe_axis3_wave17_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_spill_checkpoint_fold_edge_main.c', PROBE_DIR / 'runtime/15__probe_axis3_wave17_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_spill_checkpoint_fold_edge_lib.c'],
        extra_differential_compiler='gcc',
    ),
    RuntimeProbe(
        probe_id='15__probe_axis3_wave17_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_spill_checkpoint_fold_edge',
        source=PROBE_DIR / 'runtime/15__probe_axis3_wave17_runtime_clang_gcc_tri_diff_long_double_regstack_callchain_spill_checkpoint_fold_edge.c',
        note='axis3 wave17: long-double reg/stack callchain spill/checkpoint fold edge lane should match both clang and gcc when gcc is available',
        extra_differential_compiler='gcc',
    ),
]

DIAG_PROBES = []

DIAG_JSON_PROBES = []
