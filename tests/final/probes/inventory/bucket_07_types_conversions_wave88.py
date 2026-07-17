from pathlib import Path

from lib.models import DiagnosticExpectation, DiagnosticJsonProbe, DiagnosticProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

DIRECT_SOURCE = PROBE_DIR / "diagnostics/07__probe_diagjson_wave88_line_directive_typedef_union_to_nested_struct_cast_strict.c"
INCLUDE_SOURCE = PROBE_DIR / "diagnostics/07__probe_diagjson_wave88_line_directive_include_typedef_union_to_nested_struct_cast_strict.c"

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="07__probe_wave88_line_directive_typedef_union_to_nested_struct_cast_clang_location_strict",
        source=DIRECT_SOURCE,
        note="wave88 strict text: Clang diagnoses the invalid typedef-wrapped union-to-nested-struct cast at the aggregate cast type token",
        required_substrings=[
            "virtual_wave88_typedef_union_to_nested_struct_cast.c:20815:12",
            "Invalid cast between non-scalar types",
        ],
        fisics_env={"DISABLE_CODEGEN": "1"},
    ),
    DiagnosticProbe(
        probe_id="07__probe_wave88_line_directive_typedef_union_to_nested_struct_cast_current_operand",
        source=DIRECT_SOURCE,
        note="wave88 promoted text regression: the invalid aggregate cast is rejected once at Clang's cast-opening token",
        required_substrings=[
            "virtual_wave88_typedef_union_to_nested_struct_cast.c:20815:12",
            "Invalid cast between non-scalar types",
        ],
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="07__wave88_line_directive_typedef_union_to_nested_struct_cast_current_operand",
    ),
    DiagnosticProbe(
        probe_id="07__probe_wave88_line_directive_include_typedef_union_to_nested_struct_cast_clang_location_strict",
        source=INCLUDE_SOURCE,
        note="wave88 strict text: include-routed #line provenance retains Clang's aggregate cast-type token anchor",
        required_substrings=[
            "virtual_wave88_include_typedef_union_to_nested_struct_cast.h:20915:12",
            "Invalid cast between non-scalar types",
        ],
        fisics_env={"DISABLE_CODEGEN": "1"},
    ),
    DiagnosticProbe(
        probe_id="07__probe_wave88_line_directive_include_typedef_union_to_nested_struct_cast_current_operand",
        source=INCLUDE_SOURCE,
        note="wave88 promoted include text regression: rejection and virtual header provenance retain Clang's cast-opening token",
        required_substrings=[
            "virtual_wave88_include_typedef_union_to_nested_struct_cast.h:20915:12",
            "Invalid cast between non-scalar types",
        ],
        fisics_env={"DISABLE_CODEGEN": "1"},
        promoted_test_id="07__wave88_line_directive_include_typedef_union_to_nested_struct_cast_current_operand",
    ),
]

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_wave88_line_directive_typedef_union_to_nested_struct_cast_clang_location_strict",
        source=DIRECT_SOURCE,
        note="wave88 strict JSON: the semantic error must match Clang's direct cast-type token at column 12",
        expected_diagnostics=(
            DiagnosticExpectation(code=2000, line=20815, column=12, has_file=True, severity="error", stage="semantic"),
        ),
        fisics_env={"DISABLE_CODEGEN": "1"},
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_wave88_line_directive_typedef_union_to_nested_struct_cast_current_operand",
        source=DIRECT_SOURCE,
        note="wave88 promoted JSON regression: direct aggregate cast rejection remains anchored at Clang's column 12",
        expected_diagnostics=(
            DiagnosticExpectation(code=2000, line=20815, column=12, has_file=True, severity="error", stage="semantic"),
        ),
        fisics_env={"DISABLE_CODEGEN": "1"},
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_wave88_line_directive_include_typedef_union_to_nested_struct_cast_clang_location_strict",
        source=INCLUDE_SOURCE,
        note="wave88 strict JSON: the virtual included header must preserve Clang's cast-type token at column 12",
        expected_diagnostics=(
            DiagnosticExpectation(code=2000, line=20915, column=12, has_file=True, severity="error", stage="semantic"),
        ),
        fisics_env={"DISABLE_CODEGEN": "1"},
    ),
    DiagnosticJsonProbe(
        probe_id="07__probe_diagjson_wave88_line_directive_include_typedef_union_to_nested_struct_cast_current_operand",
        source=INCLUDE_SOURCE,
        note="wave88 promoted include JSON regression: virtual header identity and Clang's column 12 remain stable",
        expected_diagnostics=(
            DiagnosticExpectation(code=2000, line=20915, column=12, has_file=True, severity="error", stage="semantic"),
        ),
        fisics_env={"DISABLE_CODEGEN": "1"},
    ),
]
