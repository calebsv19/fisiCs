from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
STRICT_CLANG_ARGS = ["-std=c99", "-pedantic-errors"]

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="06__probe_wave28_volatile_aggregate_assignment_once_width_strict",
        source=(
            PROBE_DIR
            / "runtime/06__probe_wave28_volatile_aggregate_assignment_once_width_strict.c"
        ),
        note=(
            "wave28 strict: whole-object volatile aggregate assignment evaluates "
            "each pointer-producing operand once and preserves adjacent guards"
        ),
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id=(
            "06__runtime__wave28_volatile_aggregate_assignment_once_width_strict"
        ),
    ),
    RuntimeProbe(
        probe_id="06__probe_wave28_nonvolatile_aggregate_assignment_once_width_control",
        source=(
            PROBE_DIR
            / "runtime/06__probe_wave28_nonvolatile_aggregate_assignment_once_width_control.c"
        ),
        note=(
            "wave28 control: the same whole-object assignment shape without "
            "volatile qualification evaluates each operand once and preserves guards"
        ),
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id=(
            "06__runtime__wave28_nonvolatile_aggregate_assignment_once_width_control"
        ),
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
