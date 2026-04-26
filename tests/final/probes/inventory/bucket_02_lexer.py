from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id='02__probe_lexer_line_directive_invalid_dollar_location_reject',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_invalid_dollar_location_reject.c',
        note="lexer diagnostic should honor #line remap for invalid '$' token",
        required_substrings=[':831:9'],
    ),
    DiagnosticProbe(
        probe_id='02__probe_lexer_line_directive_invalid_dollar_current_physical_line',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_invalid_dollar_current_physical_line.c',
        note="fixed baseline: lexer invalid '$' diagnostic now reports remapped virtual line",
        required_substrings=[':831:9'],
    ),
    DiagnosticProbe(
        probe_id='02__probe_lexer_line_directive_unterminated_string_location_reject',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_unterminated_string_location_reject.c',
        note='lexer diagnostic should honor #line remap for unterminated string literal',
        required_substrings=[':721:21'],
    ),
    DiagnosticProbe(
        probe_id='02__probe_lexer_line_directive_unterminated_string_current_physical_line',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_unterminated_string_current_physical_line.c',
        note='fixed baseline: lexer unterminated-string diagnostic now reports remapped virtual line',
        required_substrings=[':721:21'],
    ),
    DiagnosticProbe(
        probe_id='02__probe_lexer_line_directive_char_invalid_hex_escape_location_reject',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_char_invalid_hex_escape_location_reject.c',
        note='lexer diagnostic should honor #line remap for invalid \\x escape in character literal',
        required_substrings=[':911:14'],
    ),
    DiagnosticProbe(
        probe_id='02__probe_lexer_line_directive_char_invalid_hex_escape_current_physical_line',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_char_invalid_hex_escape_current_physical_line.c',
        note='fixed baseline: lexer invalid \\x escape diagnostic now reports remapped virtual line',
        required_substrings=[':911:14'],
    ),
    DiagnosticProbe(
        probe_id='02__probe_lexer_line_directive_string_invalid_escape_location_reject',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_string_invalid_escape_location_reject.c',
        note='lexer diagnostic should honor #line remap for invalid string escape',
        required_substrings=[':941:21'],
    ),
    DiagnosticProbe(
        probe_id='02__probe_lexer_line_directive_string_invalid_escape_current_physical_line',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_string_invalid_escape_current_physical_line.c',
        note='fixed baseline: lexer invalid string-escape diagnostic now reports remapped virtual line',
        required_substrings=[':941:21'],
    ),
    DiagnosticProbe(
        probe_id='02__probe_lexer_line_directive_invalid_at_location_reject',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_invalid_at_location_reject.c',
        note="lexer diagnostic should honor #line remap for invalid '@' token",
        required_substrings=[':971:9'],
    ),
    DiagnosticProbe(
        probe_id='02__probe_lexer_line_directive_invalid_at_current_physical_line',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_invalid_at_current_physical_line.c',
        note="fixed baseline: lexer invalid '@' diagnostic now reports remapped virtual line",
        required_substrings=[':971:9'],
    ),
    DiagnosticProbe(
        probe_id='02__probe_lexer_line_directive_invalid_backtick_location_reject',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_invalid_backtick_location_reject.c',
        note="lexer diagnostic should honor #line remap for invalid '`' token",
        required_substrings=[':981:9'],
    ),
    DiagnosticProbe(
        probe_id='02__probe_lexer_line_directive_invalid_backtick_current_physical_line',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_invalid_backtick_current_physical_line.c',
        note="fixed baseline: lexer invalid '`' diagnostic now reports remapped virtual line",
        required_substrings=[':981:9'],
    ),
    DiagnosticProbe(
        probe_id='02__probe_lexer_line_directive_unterminated_char_location_reject',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_unterminated_char_location_reject.c',
        note='lexer diagnostic should honor #line remap for unterminated character literal',
        required_substrings=[':991:14'],
    ),
    DiagnosticProbe(
        probe_id='02__probe_lexer_line_directive_unterminated_char_current_physical_line',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_unterminated_char_current_physical_line.c',
        note='fixed baseline: lexer unterminated-char diagnostic now reports remapped virtual line',
        required_substrings=[':991:14'],
    ),
]

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id='02__probe_diagjson_lexer_line_directive_invalid_dollar_location_reject',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_invalid_dollar_location_reject.c',
        note="diagnostics JSON should honor #line remap for lexer invalid '$' token",
        expected_codes=[1],
        expected_line=831,
    ),
    DiagnosticJsonProbe(
        probe_id='02__probe_diagjson_lexer_line_directive_invalid_dollar_current_physical_line',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_invalid_dollar_current_physical_line.c',
        note="fixed baseline: diagnostics JSON reports remapped line for lexer invalid '$' token",
        expected_codes=[1],
        expected_line=831,
    ),
    DiagnosticJsonProbe(
        probe_id='02__probe_diagjson_lexer_line_directive_unterminated_string_location_reject',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_unterminated_string_location_reject.c',
        note='diagnostics JSON should honor #line remap for lexer unterminated string literal',
        expected_codes=[1],
        expected_line=721,
    ),
    DiagnosticJsonProbe(
        probe_id='02__probe_diagjson_lexer_line_directive_unterminated_string_current_physical_line',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_unterminated_string_current_physical_line.c',
        note='fixed baseline: diagnostics JSON reports remapped line for lexer unterminated string',
        expected_codes=[1],
        expected_line=721,
    ),
    DiagnosticJsonProbe(
        probe_id='02__probe_diagjson_lexer_line_directive_char_invalid_hex_escape_location_reject',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_char_invalid_hex_escape_location_reject.c',
        note='diagnostics JSON should honor #line remap for lexer invalid \\x char escape',
        expected_codes=[1],
        expected_line=911,
    ),
    DiagnosticJsonProbe(
        probe_id='02__probe_diagjson_lexer_line_directive_char_invalid_hex_escape_current_physical_line',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_char_invalid_hex_escape_current_physical_line.c',
        note='fixed baseline: diagnostics JSON reports remapped line for lexer invalid \\x char escape',
        expected_codes=[1],
        expected_line=911,
    ),
    DiagnosticJsonProbe(
        probe_id='02__probe_diagjson_lexer_line_directive_string_invalid_escape_location_reject',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_string_invalid_escape_location_reject.c',
        note='diagnostics JSON should honor #line remap for lexer invalid string escape',
        expected_codes=[1],
        expected_line=941,
    ),
    DiagnosticJsonProbe(
        probe_id='02__probe_diagjson_lexer_line_directive_string_invalid_escape_current_physical_line',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_string_invalid_escape_current_physical_line.c',
        note='fixed baseline: diagnostics JSON reports remapped line for lexer invalid string escape',
        expected_codes=[1],
        expected_line=941,
    ),
    DiagnosticJsonProbe(
        probe_id='02__probe_diagjson_lexer_line_directive_invalid_at_location_reject',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_invalid_at_location_reject.c',
        note="diagnostics JSON should honor #line remap for lexer invalid '@' token",
        expected_codes=[1],
        expected_line=971,
    ),
    DiagnosticJsonProbe(
        probe_id='02__probe_diagjson_lexer_line_directive_invalid_at_current_physical_line',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_invalid_at_current_physical_line.c',
        note="fixed baseline: diagnostics JSON reports remapped line for lexer invalid '@' token",
        expected_codes=[1],
        expected_line=971,
    ),
    DiagnosticJsonProbe(
        probe_id='02__probe_diagjson_lexer_line_directive_invalid_backtick_location_reject',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_invalid_backtick_location_reject.c',
        note="diagnostics JSON should honor #line remap for lexer invalid '`' token",
        expected_codes=[1],
        expected_line=981,
    ),
    DiagnosticJsonProbe(
        probe_id='02__probe_diagjson_lexer_line_directive_invalid_backtick_current_physical_line',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_invalid_backtick_current_physical_line.c',
        note="fixed baseline: diagnostics JSON reports remapped line for lexer invalid '`' token",
        expected_codes=[1],
        expected_line=981,
    ),
    DiagnosticJsonProbe(
        probe_id='02__probe_diagjson_lexer_line_directive_unterminated_char_location_reject',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_unterminated_char_location_reject.c',
        note='diagnostics JSON should honor #line remap for lexer unterminated character literal',
        expected_codes=[1],
        expected_line=991,
    ),
    DiagnosticJsonProbe(
        probe_id='02__probe_diagjson_lexer_line_directive_unterminated_char_current_physical_line',
        source=PROBE_DIR / 'diagnostics/02__probe_lexer_line_directive_unterminated_char_current_physical_line.c',
        note='fixed baseline: diagnostics JSON reports remapped line for lexer unterminated character literal',
        expected_codes=[1],
        expected_line=991,
    ),
]
