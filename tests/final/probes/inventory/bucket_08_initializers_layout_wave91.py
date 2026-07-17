from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="08__probe_wave91_fam_assignment_prefix_only_strict",
        source=PROBE_DIR / "runtime/08__probe_wave91_fam_assignment_prefix_only_strict.c",
        note=(
            "wave91 strict: assignment between separately allocated flexible-array "
            "objects copies the named prefix but leaves destination tail bytes unchanged"
        ),
        fisics_args=("-std=c99",),
        clang_args=("-pedantic-errors",),
        promoted_test_id="08__runtime_wave91_fam_assignment_prefix_only_strict",
    ),
    RuntimeProbe(
        probe_id="08__probe_wave91_fam_struct_element_assignment_prefix_only_strict",
        source=PROBE_DIR / "runtime/08__probe_wave91_fam_struct_element_assignment_prefix_only_strict.c",
        note=(
            "wave91 strict: assignment copies the named prefix of separately "
            "allocated flexible-array objects while preserving destination "
            "tail elements with aggregate element type"
        ),
        fisics_args=("-std=c99",),
        clang_args=("-pedantic-errors",),
        promoted_test_id=(
            "08__runtime_wave91_fam_struct_element_assignment_prefix_only_strict"
        ),
    ),
    RuntimeProbe(
        probe_id="08__probe_wave91_fixed_array_assignment_full_copy_control",
        source=PROBE_DIR / "runtime/08__probe_wave91_fixed_array_assignment_full_copy_control.c",
        note=(
            "wave91 control: ordinary fixed-array struct assignment copies the "
            "complete fixed payload"
        ),
        fisics_args=("-std=c99",),
        clang_args=("-pedantic-errors",),
        promoted_test_id="08__runtime_wave91_fixed_array_assignment_full_copy_control",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
