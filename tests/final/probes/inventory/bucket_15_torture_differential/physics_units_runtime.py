from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent
REPO_ROOT = PROBE_DIR.parent.parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='15__probe_units_runtime_conversion_chain',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_conversion_chain.c',
        note='physics-units legal conversion chain should match clang runtime behavior on a deterministic speed-length-force-charge-energy bundle',
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_formula_graph_call_boundary',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_formula_graph_call_boundary.c',
        note='physics-units legal multi-helper formula graph should match clang runtime behavior while stressing annotated parameter boundaries, explicit conversion helpers, and derived energy-power pressure',
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_conversion_reduction_ladder',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_conversion_reduction_ladder.c',
        note='physics-units legal reduction ladder should match clang runtime behavior across repeated distance, energy, voltage, charge, and pressure conversions with same-unit aggregation after normalization',
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_return_bridge_pipeline',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_return_bridge_pipeline.c',
        note='physics-units legal return-bridge pipeline should match clang runtime behavior while chaining helper-returned converted values through staged distance, work, and total-energy helpers',
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_loop_normalization_mesh',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_loop_normalization_mesh.c',
        note='physics-units legal loop-carried normalization mesh should match clang runtime behavior while repeatedly converting distance, energy, and voltage families inside accumulator loops',
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_branch_conversion_mesh',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_branch_conversion_mesh.c',
        note='physics-units legal branch-selected conversion mesh should match clang runtime behavior while mixing direct and explicitly converted force, distance, and charge lanes through helper-returned values',
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_state_bridge',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_state_bridge_main.c',
        note='physics-units legal multi-TU state bridge should match clang runtime behavior while converted speed, time, force, and charge values cross translation-unit helper boundaries',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_state_bridge_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_state_bridge_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_table_dispatch_bridge',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_table_dispatch_bridge_main.c',
        note='physics-units legal multi-TU table-dispatch bridge should match clang runtime behavior while converted state crosses function-pointer dispatch selected from another translation unit',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_table_dispatch_bridge_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_table_dispatch_bridge_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_reducer_handoff',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_reducer_handoff_main.c',
        note='physics-units legal multi-TU reducer handoff should match clang runtime behavior while converted motion and reserve values fold through returned reducer state across translation units',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_reducer_handoff_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_reducer_handoff_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_callback_reserve_mesh',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_callback_reserve_mesh_main.c',
        note='physics-units legal multi-TU callback reserve mesh should match clang runtime behavior while callback-selected motion lanes and reserve accumulation cross translation-unit boundaries',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_callback_reserve_mesh_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_callback_reserve_mesh_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_feedback_window',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_feedback_window_main.c',
        note='physics-units legal multi-TU feedback window should match clang runtime behavior while seeded distance state and reserve energy fold through cross-file handoff and time-window scoring',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_feedback_window_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_feedback_window_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_checkpoint_charge_bridge',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_checkpoint_charge_bridge_main.c',
        note='physics-units legal multi-TU checkpoint-charge bridge should match clang runtime behavior while seeded motion state, reserve energy, and reserve charge cross translation-unit checkpoints',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_checkpoint_charge_bridge_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_checkpoint_charge_bridge_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_dispatch_window_fold',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_dispatch_window_fold_main.c',
        note='physics-units legal multi-TU dispatch-window fold should match clang runtime behavior while lane-selected motion distance and reserve energy fold through cross-file scoring',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_dispatch_window_fold_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_dispatch_window_fold_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_cpu_partition_fold',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_cpu_partition_fold_main.c',
        note='physics-units legal multi-TU multi-CPU partition fold should match clang runtime behavior while per-slot motion, reserve energy, and reserve charge accumulate through cross-file worker partitions',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_cpu_partition_fold_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_cpu_partition_fold_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_cpu_reconcile_window',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_cpu_reconcile_window_main.c',
        note='physics-units legal multi-TU multi-CPU reconcile window should match clang runtime behavior while cross-file worker states merge through reserve-energy and reserve-charge reconciliation steps',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_cpu_reconcile_window_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_cpu_reconcile_window_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_cpu_partition_checkpoint_mesh',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_cpu_partition_checkpoint_mesh_main.c',
        note='physics-units legal multi-TU multi-CPU partition checkpoint mesh should match clang runtime behavior while per-slot motion, reserve energy, reserve charge, and time-window scoring cross translation-unit worker checkpoints',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_cpu_partition_checkpoint_mesh_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_cpu_partition_checkpoint_mesh_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_cpu_reconcile_partition_ladder',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_cpu_reconcile_partition_ladder_main.c',
        note='physics-units legal multi-TU multi-CPU reconcile partition ladder should match clang runtime behavior while staged cross-file state merges normalize reserve energy and reserve charge through a deeper partition ladder',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_cpu_reconcile_partition_ladder_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_cpu_reconcile_partition_ladder_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_gpu_partition_blend',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_gpu_partition_blend_main.c',
        note='physics-units legal multi-TU multi-GPU partition blend should match clang runtime behavior while per-tile motion, reserve energy, and reserve charge accumulate through cross-file GPU partition blending',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_gpu_partition_blend_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_gpu_partition_blend_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_gpu_reconcile_mesh',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_gpu_reconcile_mesh_main.c',
        note='physics-units legal multi-TU multi-GPU reconcile mesh should match clang runtime behavior while staged tile merges normalize reserve energy and reserve charge through a cross-file reconcile mesh',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_gpu_reconcile_mesh_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_gpu_reconcile_mesh_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_gpu_checkpoint_fanout',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_gpu_checkpoint_fanout_main.c',
        note='physics-units legal multi-TU multi-GPU checkpoint fanout should match clang runtime behavior while per-tile motion, reserve energy, reserve charge, and score accumulation cross translation-unit GPU checkpoints',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_gpu_checkpoint_fanout_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_gpu_checkpoint_fanout_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_multitu_gpu_feedback_ladder',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_gpu_feedback_ladder_main.c',
        note='physics-units legal multi-TU multi-GPU feedback ladder should match clang runtime behavior while staged tile merges normalize reserve energy and reserve charge through a deeper feedback ladder',
        inputs=[
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_gpu_feedback_ladder_main.c',
            PROBE_DIR / 'runtime/15__probe_units_runtime_multitu_gpu_feedback_ladder_lib.c',
        ],
        fisics_args=['--overlay=physics-units'],
    ),
]
