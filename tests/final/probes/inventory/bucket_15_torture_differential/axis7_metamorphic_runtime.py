from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


def _probe(probe_id: str, *files: str) -> RuntimeProbe:
    inputs = [PROBE_DIR / f"runtime/{name}" for name in files]
    payload = dict(
        probe_id=probe_id,
        source=inputs[0],
        note="axis7 defined-program transformation should match clang runtime behavior",
    )
    if len(inputs) > 1:
        payload["inputs"] = inputs
    return RuntimeProbe(**payload)


RUNTIME_PROBES = [
    _probe("15__probe_axis7_wave1_aggregate_return_staged_copy_equivalence", "15__probe_axis7_wave1_aggregate_return_staged_copy_equivalence.c"),
    _probe("15__probe_axis7_wave1_control_flow_normalization_equivalence", "15__probe_axis7_wave1_control_flow_normalization_equivalence.c"),
    _probe("15__probe_axis7_wave1_cross_tu_partition_recomposition_equivalence", "15__probe_axis7_wave1_cross_tu_partition_recomposition_equivalence_main.c", "15__probe_axis7_wave1_cross_tu_partition_recomposition_equivalence_lib.c"),
    _probe("15__probe_axis7_wave2_struct_assignment_fieldwise_equivalence", "15__probe_axis7_wave2_struct_assignment_fieldwise_equivalence.c"),
    _probe("15__probe_axis7_wave2_callback_table_dispatch_equivalence", "15__probe_axis7_wave2_callback_table_dispatch_equivalence.c"),
    _probe("15__probe_axis7_wave2_multitu_unsigned_reassociation_equivalence", "15__probe_axis7_wave2_multitu_unsigned_reassociation_equivalence_main.c", "15__probe_axis7_wave2_multitu_unsigned_reassociation_equivalence_lib.c"),
    _probe("15__probe_axis7_wave3_designated_positional_aggregate_equivalence", "15__probe_axis7_wave3_designated_positional_aggregate_equivalence.c"),
    _probe("15__probe_axis7_wave3_direct_wrapper_fnptr_equivalence", "15__probe_axis7_wave3_direct_wrapper_fnptr_equivalence.c"),
    _probe("15__probe_axis7_wave3_vla_row_rebased_slice_equivalence", "15__probe_axis7_wave3_vla_row_rebased_slice_equivalence.c"),
    _probe("15__probe_axis7_wave4_lexical_alpha_same_name_shadow", "15__probe_axis7_wave4_lexical_alpha_same_name_shadow.c"),
    _probe("15__probe_axis7_wave4_lexical_alpha_unique_name_control", "15__probe_axis7_wave4_lexical_alpha_unique_name_control.c"),
]
