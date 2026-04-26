from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id='01__probe_line_directive_virtual_line_spelling_reject',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_virtual_line_spelling_reject.c',
        note='#line virtual filename should appear in diagnostic spelling location',
        required_substrings=['virtual_phase01_probe.c'],
    ),
    DiagnosticProbe(
        probe_id='01__probe_line_directive_virtual_macro_filename_spelling_reject',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_virtual_macro_filename_spelling_reject.c',
        note='#line macro-expanded virtual filename should appear in diagnostic spelling location',
        required_substrings=['virtual_macro_phase01_probe.c'],
    ),
    DiagnosticProbe(
        probe_id='01__probe_line_mapping_real_file_baseline',
        source=PROBE_DIR / 'diagnostics/01__probe_line_mapping_real_file_baseline.c',
        note='baseline file-backed macro diagnostic should retain concrete spelling location',
        required_substrings=['01__probe_line_mapping_real_file_baseline.c'],
    ),
    DiagnosticProbe(
        probe_id='01__probe_line_directive_virtual_line_nonvoid_return_location_reject',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_virtual_line_nonvoid_return_location_reject.c',
        note='#line virtual line/file should propagate into non-void return diagnostic location',
        required_substrings=['Error at (322:'],
    ),
    DiagnosticProbe(
        probe_id='01__probe_line_directive_virtual_line_nonvoid_return_current_zerozero',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_virtual_line_nonvoid_return_current_zerozero.c',
        note='fixed baseline: non-void return diagnostic now emits remapped #line location',
        required_substrings=['Error at (322:'],
    ),
    DiagnosticProbe(
        probe_id='01__probe_line_directive_virtual_line_undeclared_identifier_location_reject',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_virtual_line_undeclared_identifier_location_reject.c',
        note='#line virtual line/file should propagate into undeclared-identifier semantic diagnostic',
        required_substrings=['Error at (322:'],
    ),
    DiagnosticProbe(
        probe_id='01__probe_nonvoid_return_plain_current_zerozero',
        source=PROBE_DIR / 'diagnostics/01__probe_nonvoid_return_plain_current_zerozero.c',
        note='fixed baseline: non-void return diagnostic now emits plain source location',
        required_substrings=['Error at (2:'],
    ),
    DiagnosticProbe(
        probe_id='01__probe_line_directive_macro_nonvoid_return_location_reject',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_macro_nonvoid_return_location_reject.c',
        note='#line + macro-expanded return should propagate virtual location in non-void return diagnostic',
        required_substrings=['Error at (453:'],
    ),
    DiagnosticProbe(
        probe_id='01__probe_line_directive_macro_nonvoid_return_current_zerozero',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_macro_nonvoid_return_current_zerozero.c',
        note='fixed baseline: #line + macro-expanded non-void return now emits mapped location',
        required_substrings=['Error at (453:'],
    ),
]

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id='01__probe_diagjson_line_directive_macro_line_map_strict',
        source=PROBE_DIR / 'diagnostics/01__probe_diagjson_line_directive_macro_line_map_strict.c',
        note='diagnostics JSON should preserve #line remapped location for macro-formed semantic diagnostics',
        expected_codes=[1000, 2000],
        expected_line=404,
    ),
    DiagnosticJsonProbe(
        probe_id='01__probe_diagjson_line_directive_nonvoid_return_location_reject',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_virtual_line_nonvoid_return_location_reject.c',
        note='diagnostics JSON should preserve #line remapped location for non-void return diagnostics',
        expected_codes=[2000],
        expected_line=322,
    ),
    DiagnosticJsonProbe(
        probe_id='01__probe_diagjson_line_directive_undeclared_identifier_location_strict',
        source=PROBE_DIR / 'diagnostics/01__probe_diagjson_line_directive_undeclared_identifier_location_strict.c',
        note='diagnostics JSON should preserve #line remapped location for undeclared-identifier diagnostics',
        expected_line=322,
    ),
    DiagnosticJsonProbe(
        probe_id='01__probe_diagjson_nonvoid_return_plain_current_zerozero',
        source=PROBE_DIR / 'diagnostics/01__probe_nonvoid_return_plain_current_zerozero.c',
        note='fixed baseline: diagnostics JSON emits source line for non-void return diagnostic',
        expected_codes=[2000],
        expected_line=2,
    ),
    DiagnosticJsonProbe(
        probe_id='01__probe_diagjson_line_directive_macro_nonvoid_return_location_reject',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_macro_nonvoid_return_location_reject.c',
        note='diagnostics JSON should preserve #line location for macro-expanded non-void return diagnostics',
        expected_codes=[2000],
        expected_line=453,
    ),
    DiagnosticJsonProbe(
        probe_id='01__probe_diagjson_line_directive_macro_nonvoid_return_current_zerozero',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_macro_nonvoid_return_current_zerozero.c',
        note='fixed baseline: diagnostics JSON emits mapped line for #line+macro non-void return diagnostics',
        expected_codes=[2000],
        expected_line=453,
    ),
]
