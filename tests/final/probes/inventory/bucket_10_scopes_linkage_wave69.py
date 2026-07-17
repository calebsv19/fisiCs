from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
STRICT_CLANG_ARGS = [
    "-std=c99",
    "-pedantic-errors",
    "-Wno-strict-prototypes",
    "-Wno-deprecated-non-prototype",
]

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="10__probe_wave69_nested_extern_no_prototype_double",
        source=PROBE_DIR / "runtime/10__probe_wave69_nested_extern_no_prototype_double_runtime.c",
        note=(
            "wave69 strict: a nested block extern no-prototype declaration must "
            "rebind past a same-spelled parameter and remain compatible with a "
            "file-scope double prototype"
        ),
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="10__runtime__wave69_nested_extern_no_prototype_double",
    ),
    RuntimeProbe(
        probe_id="10__probe_wave69_nested_extern_exact_prototype_control",
        source=PROBE_DIR / "runtime/10__probe_wave69_nested_extern_exact_prototype_control.c",
        note=(
            "wave69 current-threshold control: the same nested extern rebinding "
            "remains operational when both declarations use an exact prototype"
        ),
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="10__runtime__wave69_nested_extern_exact_prototype_control",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="10__probe_wave69_nested_extern_no_prototype_float_conflict",
        source=PROBE_DIR / "diagnostics/10__probe_wave69_nested_extern_no_prototype_float_conflict.c",
        note=(
            "wave69 strict negative: a nested no-prototype extern declaration must "
            "conflict with a file-scope float prototype because float does not "
            "survive default argument promotion"
        ),
        required_substrings=["wave69_route_float"],
        promoted_test_id="10__diag__wave69_nested_extern_no_prototype_float_conflict",
    ),
]

DIAG_JSON_PROBES = []
