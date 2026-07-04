from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='01__probe_runtime_include_stringize_remap',
        source=PROBE_DIR / 'runtime/01__probe_runtime_include_stringize_remap.c',
        note='single-include #line stringize lane should preserve remapped __FILE__ and stable __LINE__ at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_include_nested_stringize_depth',
        source=PROBE_DIR / 'runtime/01__probe_runtime_include_nested_stringize_depth.c',
        note='nested include + stringize rescan lane should preserve inner and outer remapped source coordinates at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_include_tokenpaste_depth',
        source=PROBE_DIR / 'runtime/01__probe_runtime_include_tokenpaste_depth.c',
        note='include-header token-paste lane should preserve remapped source coordinates for token-formed identifiers at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_include_file_line_bridge',
        source=PROBE_DIR / 'runtime/01__probe_runtime_include_file_line_bridge.c',
        note='single-include #line provenance lane should preserve remapped __LINE__ through a clean runtime bridge',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_nested_include_provenance_bridge',
        source=PROBE_DIR / 'runtime/01__probe_runtime_nested_include_provenance_bridge.c',
        note='nested include provenance lane should preserve inner remapped lines while outer mapping resumes after the include',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_include_tokenpaste_stringize_bridge',
        source=PROBE_DIR / 'runtime/01__probe_runtime_include_tokenpaste_stringize_bridge.c',
        note='clean token-paste plus stringize lane should preserve remapped line values for token-built runtime values',
    ),
]

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
    DiagnosticProbe(
        probe_id='01__probe_line_directive_include_nested_macro_undeclared_identifier_location_strict',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_include_nested_macro_undeclared_identifier_location_strict.c',
        note='#line include-header nested macro rescan should preserve remapped undeclared-identifier location',
        required_substrings=['Error at (3619:'],
    ),
    DiagnosticProbe(
        probe_id='01__probe_line_directive_include_tokenpaste_undeclared_identifier_location_strict',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_include_tokenpaste_undeclared_identifier_location_strict.c',
        note='#line include-header token-paste should preserve remapped undeclared-identifier location',
        required_substrings=['Error at (3819:'],
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
    DiagnosticJsonProbe(
        probe_id='01__probe_diagjson_line_directive_include_nested_macro_undeclared_identifier_location_strict',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_include_nested_macro_undeclared_identifier_location_strict.c',
        note='diagnostics JSON should preserve remapped include-header nested macro rescan location',
        expected_codes=[2000],
        expected_line=3619,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='01__probe_diagjson_line_directive_include_tokenpaste_undeclared_identifier_location_strict',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_include_tokenpaste_undeclared_identifier_location_strict.c',
        note='diagnostics JSON should preserve remapped include-header token-paste location',
        expected_codes=[2000],
        expected_line=3819,
        expected_has_file=True,
    ),
]
