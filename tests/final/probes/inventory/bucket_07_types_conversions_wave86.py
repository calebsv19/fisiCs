from pathlib import Path

from lib.models import DiagnosticJsonProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []
DIAG_PROBES = []

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="07__probe_wave86_no_prototype_complex_float_initializer_compatible",
        source=PROBE_DIR / "diagnostics/07__probe_diagjson_wave86_no_prototype_complex_float_compatible.c",
        note="wave86: no-prototype and float-complex prototypes remain initializer-compatible in both directions",
        require_any_diagnostic=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="07__wave86_no_prototype_complex_float_initializer_compatible_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_wave86_no_prototype_complex_float_assignment_compatible",
        source=PROBE_DIR / "diagnostics/07__probe_diagjson_wave86_no_prototype_complex_float_assignment_compatible.c",
        note="wave86: no-prototype and float-complex prototypes remain assignment-compatible in both directions",
        require_any_diagnostic=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="07__wave86_no_prototype_complex_float_assignment_compatible_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_wave86_no_prototype_real_float_initializer_strict",
        source=PROBE_DIR / "diagnostics/07__probe_diagjson_wave86_no_prototype_real_float_initializer_strict.c",
        note="wave86 strict: real-float prototypes remain initializer-incompatible with no-prototype function pointers in both directions",
        expected_codes=[2000],
        expected_line=19508,
        expected_has_file=True,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="07__wave86_no_prototype_real_float_initializer_diagjson_strict",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_wave86_no_prototype_real_float_assignment_strict",
        source=PROBE_DIR / "diagnostics/07__probe_diagjson_wave86_no_prototype_real_float_assignment_strict.c",
        note="wave86 strict: real-float prototypes remain assignment-incompatible with no-prototype function pointers in both directions",
        expected_codes=[2000],
        expected_line=19610,
        expected_has_file=True,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="07__wave86_no_prototype_real_float_assignment_diagjson_strict",
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_wave86_no_prototype_complex_double_compatible",
        source=PROBE_DIR / "diagnostics/07__probe_diagjson_wave86_no_prototype_complex_double_compatible.c",
        note="wave86: no-prototype and double-complex prototypes remain compatible across initializer and assignment paths in both directions",
        require_any_diagnostic=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="07__wave86_no_prototype_complex_double_compatible_diagjson",
    ),
]
