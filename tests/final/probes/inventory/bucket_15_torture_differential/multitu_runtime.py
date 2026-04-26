from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='15__probe_multitu_const_table_crc',
        source=PROBE_DIR / 'runtime/15__probe_multitu_const_table_crc_main.c',
        note='multi-TU const-table CRC fold path should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_const_table_crc_main.c', PROBE_DIR / 'runtime/15__probe_multitu_const_table_crc_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_fnptr_state_matrix',
        source=PROBE_DIR / 'runtime/15__probe_multitu_fnptr_state_matrix_main.c',
        note='multi-TU stateful function-pointer matrix should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_fnptr_state_matrix_main.c', PROBE_DIR / 'runtime/15__probe_multitu_fnptr_state_matrix_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_struct_vla_stride_bridge',
        source=PROBE_DIR / 'runtime/15__probe_multitu_struct_vla_stride_bridge_main.c',
        note='multi-TU struct+VLA stride bridge should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_struct_vla_stride_bridge_main.c', PROBE_DIR / 'runtime/15__probe_multitu_struct_vla_stride_bridge_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_ring_digest_pipeline',
        source=PROBE_DIR / 'runtime/15__probe_multitu_ring_digest_pipeline_main.c',
        note='multi-TU ring-digest pipeline with static state should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_ring_digest_pipeline_main.c', PROBE_DIR / 'runtime/15__probe_multitu_ring_digest_pipeline_lib.c', PROBE_DIR / 'runtime/15__probe_multitu_ring_digest_pipeline_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_large_table_crc_pressure',
        source=PROBE_DIR / 'runtime/15__probe_multitu_large_table_crc_pressure_main.c',
        note='multi-TU large-table CRC pressure lane should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_large_table_crc_pressure_main.c', PROBE_DIR / 'runtime/15__probe_multitu_large_table_crc_pressure_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_recursive_dispatch_pressure',
        source=PROBE_DIR / 'runtime/15__probe_multitu_recursive_dispatch_pressure_main.c',
        note='multi-TU recursive dispatch pressure lane should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_recursive_dispatch_pressure_main.c', PROBE_DIR / 'runtime/15__probe_multitu_recursive_dispatch_pressure_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_layout_stride_pressure',
        source=PROBE_DIR / 'runtime/15__probe_multitu_layout_stride_pressure_main.c',
        note='multi-TU layout/stride pressure lane should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_layout_stride_pressure_main.c', PROBE_DIR / 'runtime/15__probe_multitu_layout_stride_pressure_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_queue_rollback_pressure',
        source=PROBE_DIR / 'runtime/15__probe_multitu_queue_rollback_pressure_main.c',
        note='multi-TU queue rollback-pressure lane should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_queue_rollback_pressure_main.c', PROBE_DIR / 'runtime/15__probe_multitu_queue_rollback_pressure_lib.c', PROBE_DIR / 'runtime/15__probe_multitu_queue_rollback_pressure_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_inline_reset_generation_bridge',
        source=PROBE_DIR / 'runtime/15__probe_multitu_inline_reset_generation_bridge_main.c',
        note='multi-TU inline reset generation bridge should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_inline_reset_generation_bridge_main.c', PROBE_DIR / 'runtime/15__probe_multitu_inline_reset_generation_bridge_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_queue_epoch_reclaim_pressure',
        source=PROBE_DIR / 'runtime/15__probe_multitu_queue_epoch_reclaim_pressure_main.c',
        note='multi-TU queue epoch/reclaim pressure lane should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_queue_epoch_reclaim_pressure_main.c', PROBE_DIR / 'runtime/15__probe_multitu_queue_epoch_reclaim_pressure_lib.c', PROBE_DIR / 'runtime/15__probe_multitu_queue_epoch_reclaim_pressure_aux.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_token_window_wraparound_churn',
        source=PROBE_DIR / 'runtime/15__probe_multitu_token_window_wraparound_churn_main.c',
        note='multi-TU token-window wraparound churn lane should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_token_window_wraparound_churn_main.c', PROBE_DIR / 'runtime/15__probe_multitu_token_window_wraparound_churn_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_owner_epoch_rotation_matrix',
        source=PROBE_DIR / 'runtime/15__probe_multitu_owner_epoch_rotation_matrix_main.c',
        note='multi-TU owner epoch-rotation matrix lane should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_owner_epoch_rotation_matrix_main.c', PROBE_DIR / 'runtime/15__probe_multitu_owner_epoch_rotation_matrix_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_queue_generation_reclaim_churn',
        source=PROBE_DIR / 'runtime/15__probe_multitu_queue_generation_reclaim_churn_main.c',
        note='multi-TU queue generation/reclaim churn lane should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_queue_generation_reclaim_churn_main.c', PROBE_DIR / 'runtime/15__probe_multitu_queue_generation_reclaim_churn_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_owner_transfer_toggle_storm',
        source=PROBE_DIR / 'runtime/15__probe_multitu_owner_transfer_toggle_storm_main.c',
        note='multi-TU owner transfer/toggle storm lane should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_owner_transfer_toggle_storm_main.c', PROBE_DIR / 'runtime/15__probe_multitu_owner_transfer_toggle_storm_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_token_epoch_wraparound_reseed',
        source=PROBE_DIR / 'runtime/15__probe_multitu_token_epoch_wraparound_reseed_main.c',
        note='multi-TU token epoch wraparound/reseed lane should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_token_epoch_wraparound_reseed_main.c', PROBE_DIR / 'runtime/15__probe_multitu_token_epoch_wraparound_reseed_lib.c'],
    ),
]

DIAG_PROBES = []

DIAG_JSON_PROBES = []
