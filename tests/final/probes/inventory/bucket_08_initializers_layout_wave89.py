from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="08__probe_wave89_static_pointer_conditional_address_strict",
        source=PROBE_DIR / "runtime/08__probe_wave89_static_pointer_conditional_address_strict.c",
        note="wave89 strict: a selected address constant and discarded null-pointer arm remain valid in a static pointer conditional initializer",
        fisics_args=("-std=c99",),
        clang_args=("-pedantic-errors",),
        promoted_test_id="08__runtime_wave89_static_pointer_conditional_address_strict",
    ),
    RuntimeProbe(
        probe_id="08__probe_wave89_static_pointer_direct_address_control",
        source=PROBE_DIR / "runtime/08__probe_wave89_static_pointer_direct_address_control.c",
        note="wave89 control: the same object and runtime observation pass with a direct static address initializer",
        fisics_args=("-std=c99",),
        clang_args=("-pedantic-errors",),
        promoted_test_id="08__runtime_wave89_static_pointer_direct_address_control",
    ),
    RuntimeProbe(
        probe_id="08__probe_wave89_static_general_arithmetic_constant_strict",
        source=PROBE_DIR / "runtime/08__probe_wave89_static_general_arithmetic_constant_strict.c",
        note="wave89 strict: static arithmetic constants accept floating operations, a floating relational result, and a conditional whose discarded arm contains a call",
        fisics_args=("-std=c99",),
        clang_args=("-pedantic-errors",),
        promoted_test_id="08__runtime_wave89_static_general_arithmetic_constant_strict",
    ),
    RuntimeProbe(
        probe_id="08__probe_wave89_static_simple_float_control",
        source=PROBE_DIR / "runtime/08__probe_wave89_static_simple_float_control.c",
        note="wave89 control: simple static floating and integer constants preserve the strict probe's expected runtime payload",
        fisics_args=("-std=c99",),
        clang_args=("-pedantic-errors",),
        promoted_test_id="08__runtime_wave89_static_simple_float_control",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
