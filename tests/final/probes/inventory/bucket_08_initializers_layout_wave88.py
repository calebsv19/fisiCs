from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="08__probe_wave88_fam_allocated_tail_indexing_strict",
        source=PROBE_DIR / "runtime/08__probe_wave88_fam_allocated_tail_indexing_strict.c",
        note="wave88 strict: a flexible byte tail has sizeof equal to its offset and supports allocated tail indexing",
        promoted_test_id="08__runtime_wave88_fam_allocated_tail_indexing_strict",
    ),
    RuntimeProbe(
        probe_id="08__probe_wave88_fam_nested_alignment_strict",
        source=PROBE_DIR / "runtime/08__probe_wave88_fam_nested_alignment_strict.c",
        note="wave88 strict: a flexible tail of nested aggregates preserves element alignment, stride, and allocated indexing",
        promoted_test_id="08__runtime_wave88_fam_nested_alignment_strict",
    ),
    RuntimeProbe(
        probe_id="08__probe_wave88_fixed_nested_array_control",
        source=PROBE_DIR / "runtime/08__probe_wave88_fixed_nested_array_control.c",
        note="wave88 control: the same nested aggregate layout and indexing remain correct with a fixed-size tail",
        promoted_test_id="08__runtime_wave88_fixed_nested_array_control",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
