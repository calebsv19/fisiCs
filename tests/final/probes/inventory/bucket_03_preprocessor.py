from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id='03__probe_line_directive_error_location_reject',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_error_location_reject.c',
        note='#line virtual line should propagate into #error diagnostic location',
        required_substrings=['Error at (777:'],
    ),
    DiagnosticProbe(
        probe_id='03__probe_line_directive_error_current_physical_line',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_error_current_physical_line.c',
        note='fixed baseline: #error diagnostic now reports remapped virtual line',
        required_substrings=['Error at (777:1): bucket03 - probe - error'],
    ),
    DiagnosticProbe(
        probe_id='03__probe_line_directive_error_filename_location_reject',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_error_filename_location_reject.c',
        note='#line virtual filename should propagate into #error diagnostic spelling location',
        required_substrings=['Spelling: virtual_pp_error_filename_probe.c:777:1'],
    ),
    DiagnosticProbe(
        probe_id='03__probe_line_directive_error_filename_current_physical',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_error_filename_current_physical.c',
        note='fixed baseline: #error diagnostic spelling now uses remapped virtual filename',
        required_substrings=['virtual_pp_error_filename_probe_current.c:777:1'],
    ),
    DiagnosticProbe(
        probe_id='03__probe_line_directive_warning_filename_location_reject',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_warning_filename_location_reject.c',
        note='#line virtual filename should propagate into #warning diagnostic spelling location',
        required_substrings=['Spelling: virtual_pp_warning_filename_probe.c:888:1'],
    ),
    DiagnosticProbe(
        probe_id='03__probe_line_directive_warning_filename_current_physical',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_warning_filename_current_physical.c',
        note='fixed baseline: #warning diagnostic spelling now uses remapped virtual filename',
        required_substrings=['virtual_pp_warning_filename_probe_current.c:888:1'],
    ),
    DiagnosticProbe(
        probe_id='03__probe_line_directive_pragma_stdc_filename_location_reject',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_pragma_stdc_filename_location_reject.c',
        note='#line virtual filename should propagate into #pragma STDC diagnostic spelling location',
        required_substrings=['Spelling: virtual_pragma_stdc_filename_probe.c:444:9'],
    ),
    DiagnosticProbe(
        probe_id='03__probe_line_directive_pragma_stdc_filename_current_physical',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_pragma_stdc_filename_current_physical.c',
        note='fixed baseline: #pragma STDC diagnostic spelling now uses remapped virtual filename',
        required_substrings=['virtual_pragma_stdc_filename_probe_current.c:444:9'],
    ),
    DiagnosticProbe(
        probe_id='03__probe_line_directive_include_error_filename_location_reject',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_include_error_filename_location_reject.c',
        note='#line virtual filename in included header should propagate into #error diagnostic spelling location',
        required_substrings=['Spelling: virtual_pp_include_error_header_probe.h:615:1'],
    ),
    DiagnosticProbe(
        probe_id='03__probe_line_directive_include_error_filename_current_physical',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_include_error_filename_current_physical.c',
        note='fixed baseline: include #error diagnostic spelling now uses remapped virtual header filename',
        required_substrings=['virtual_pp_include_error_header_probe_current.h:615:1'],
    ),
    DiagnosticProbe(
        probe_id='03__probe_line_directive_include_warning_filename_location_reject',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_include_warning_filename_location_reject.c',
        note='#line virtual filename in included header should propagate into #warning diagnostic spelling location',
        required_substrings=['Spelling: virtual_pp_include_warning_header_probe.h:616:1'],
    ),
    DiagnosticProbe(
        probe_id='03__probe_line_directive_include_warning_filename_current_physical',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_include_warning_filename_current_physical.c',
        note='fixed baseline: include #warning diagnostic spelling now uses remapped virtual header filename',
        required_substrings=['virtual_pp_include_warning_header_probe_current.h:616:1'],
    ),
    DiagnosticProbe(
        probe_id='03__probe_line_directive_include_pragma_stdc_filename_location_reject',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_include_pragma_stdc_filename_location_reject.c',
        note='#line virtual filename in included header should propagate into #pragma STDC diagnostic spelling location',
        required_substrings=['Spelling: virtual_pp_include_pragma_header_probe.h:617:9'],
    ),
    DiagnosticProbe(
        probe_id='03__probe_line_directive_include_pragma_stdc_filename_current_physical',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_include_pragma_stdc_filename_current_physical.c',
        note='fixed baseline: include #pragma STDC diagnostic spelling now uses remapped virtual header filename',
        required_substrings=['virtual_pp_include_pragma_header_probe_current.h:617:9'],
    ),
]

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id='03__probe_diagjson_line_directive_error_location_reject',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_error_location_reject.c',
        note='diagnostics JSON should honor #line remap for #error directive diagnostics',
        expected_codes=[3000],
        expected_line=777,
    ),
    DiagnosticJsonProbe(
        probe_id='03__probe_diagjson_line_directive_error_current_physical_line',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_error_current_physical_line.c',
        note='fixed baseline: diagnostics JSON now reports remapped line for #error directive diagnostics',
        expected_codes=[3000],
        expected_line=777,
    ),
    DiagnosticJsonProbe(
        probe_id='03__probe_diagjson_line_directive_warning_location_reject',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_warning_location_reject.c',
        note='diagnostics JSON should honor #line remap for #warning directive diagnostics',
        expected_codes=[3000],
        expected_line=888,
    ),
    DiagnosticJsonProbe(
        probe_id='03__probe_diagjson_line_directive_warning_current_physical_line',
        source=PROBE_DIR / 'diagnostics/03__probe_line_directive_warning_current_physical_line.c',
        note='fixed baseline: diagnostics JSON now reports remapped line for #warning directive diagnostics',
        expected_codes=[3000],
        expected_line=888,
    ),
    DiagnosticJsonProbe(
        probe_id='03__probe_diagjson_line_directive_pragma_stdc_location_reject',
        source=PROBE_DIR / 'diagnostics/03__probe_diagjson_line_directive_pragma_stdc_location_reject.c',
        note='diagnostics JSON should honor #line remap for #pragma STDC diagnostics',
        expected_codes=[3000],
        expected_line=444,
    ),
    DiagnosticJsonProbe(
        probe_id='03__probe_diagjson_line_directive_pragma_stdc_current_physical_line',
        source=PROBE_DIR / 'diagnostics/03__probe_diagjson_line_directive_pragma_stdc_current_physical_line.c',
        note='fixed baseline: diagnostics JSON now reports remapped line for #pragma STDC diagnostics',
        expected_codes=[3000],
        expected_line=444,
    ),
]
