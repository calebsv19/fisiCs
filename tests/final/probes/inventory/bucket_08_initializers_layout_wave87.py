from pathlib import Path

from lib.models import DiagnosticJsonProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []
DIAG_PROBES = []

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="08__probe_wave87_no_prototype_complex_float_array_compatible",
        source=PROBE_DIR / "diagnostics/08__probe_diagjson_wave87_no_prototype_complex_float_array_compatible.c",
        note="wave87: direct array elements preserve bidirectional compatibility between no-prototype and float-complex function pointers",
        require_any_diagnostic=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="08__wave87_no_prototype_complex_float_array_compatible_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_wave87_no_prototype_complex_float_nested_array_compatible",
        source=PROBE_DIR / "diagnostics/08__probe_diagjson_wave87_no_prototype_complex_float_nested_array_compatible.c",
        note="wave87: struct-contained array elements preserve bidirectional compatibility between no-prototype and float-complex function pointers",
        require_any_diagnostic=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="08__wave87_no_prototype_complex_float_nested_array_compatible_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_wave87_no_prototype_real_float_array_strict",
        source=PROBE_DIR / "diagnostics/08__probe_diagjson_wave87_no_prototype_real_float_array_strict.c",
        note="wave87 strict: direct array elements reject real-float prototype compatibility with no-prototype function pointers in both directions",
        expected_codes=[2000],
        expected_line=20008,
        expected_has_file=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="08__wave87_no_prototype_real_float_array_diagjson_strict",
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_wave87_no_prototype_real_float_nested_array_strict",
        source=PROBE_DIR / "diagnostics/08__probe_diagjson_wave87_no_prototype_real_float_nested_array_strict.c",
        note="wave87 strict: struct-contained array elements reject real-float prototype compatibility with no-prototype function pointers in both directions",
        expected_codes=[2000],
        expected_line=20114,
        expected_has_file=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="08__wave87_no_prototype_real_float_nested_array_diagjson_strict",
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_wave87_no_prototype_complex_double_array_compatible",
        source=PROBE_DIR / "diagnostics/08__probe_diagjson_wave87_no_prototype_complex_double_array_compatible.c",
        note="wave87: direct array elements preserve bidirectional compatibility between no-prototype and double-complex function pointers",
        require_any_diagnostic=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="08__wave87_no_prototype_complex_double_array_compatible_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="08__probe_wave87_no_prototype_complex_double_nested_array_compatible",
        source=PROBE_DIR / "diagnostics/08__probe_diagjson_wave87_no_prototype_complex_double_nested_array_compatible.c",
        note="wave87: struct-contained array elements preserve bidirectional compatibility between no-prototype and double-complex function pointers",
        require_any_diagnostic=False,
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="08__wave87_no_prototype_complex_double_nested_array_compatible_diagjson",
    ),
]
