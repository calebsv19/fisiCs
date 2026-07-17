from pathlib import Path

from lib.models import DiagnosticExpectation, DiagnosticJsonProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="05__probe_wave34_fixed_sizeof_discarded_comma_controls_runtime",
        source=PROBE_DIR / "runtime/05__probe_wave34_fixed_sizeof_discarded_comma_controls_runtime.c",
        note=(
            "wave34 reduced controls: fixed-array sizeof remains an ICE and a "
            "discarded comma arm remains admissible under the strict C99 oracle"
        ),
        clang_args=["-pedantic-errors", "-Wno-unused-value"],
        promoted_test_id="05__runtime_wave34_fixed_sizeof_discarded_comma_controls",
    ),
]

DIAG_PROBES = []

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="05__probe_wave34_const_object_enum_case_ice_reject",
        source=PROBE_DIR / "diagnostics/05__probe_wave34_const_object_enum_case_ice_reject.c",
        note=(
            "wave34 strict C99 ICE: a const-qualified object is not an integer "
            "constant expression in either an enumerator or a case label"
        ),
        expected_diagnostics=(
            DiagnosticExpectation(
                code=2000,
                line=34002,
                column=8,
                has_file=True,
                severity="error",
                stage="semantic",
            ),
            DiagnosticExpectation(
                code=2000,
                line=34006,
                column=14,
                has_file=True,
                severity="error",
                stage="semantic",
            ),
        ),
        allowed_exit_codes=(1,),
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave34_const_object_enum_case_ice_diagjson",
    ),
    DiagnosticJsonProbe(
        probe_id="05__probe_wave34_discarded_vla_sizeof_ice_reject",
        source=PROBE_DIR / "diagnostics/05__probe_wave34_discarded_vla_sizeof_ice_reject.c",
        note=(
            "wave34 strict C99 ICE: discarded conditional and logical operands "
            "containing sizeof of a VLA remain non-ICE case labels"
        ),
        expected_diagnostics=(
            DiagnosticExpectation(
                code=2000,
                line=34103,
                column=14,
                has_file=True,
                severity="error",
                stage="semantic",
            ),
            DiagnosticExpectation(
                code=2000,
                line=34105,
                column=14,
                has_file=True,
                severity="error",
                stage="semantic",
            ),
        ),
        allowed_exit_codes=(1,),
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="05__wave34_discarded_vla_sizeof_ice_diagjson",
    ),
]
