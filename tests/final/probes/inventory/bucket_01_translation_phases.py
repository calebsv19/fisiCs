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
    RuntimeProbe(
        probe_id='01__probe_runtime_wave13_adjacent_string_file_remap',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave13_adjacent_string_file_remap.c',
        note='include-header #line remap should preserve __FILE__ inside adjacent string literal concatenation at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave13_tokenpaste_line_stringize',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave13_tokenpaste_line_stringize.c',
        note='include-header token paste plus stringized __LINE__ should preserve mapped source provenance at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave13_spliced_macro_include_provenance',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave13_spliced_macro_include_provenance.c',
        note='include-header line-spliced macro argument and adjacent string comment bridge should preserve remapped provenance at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave14_nested_include_adjacent_source',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave14_nested_include_adjacent_source.c',
        note='nested include #line remap should preserve inner and outer virtual source coordinates through adjacent strings at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave14_tokenpaste_stringize_adjacent',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave14_tokenpaste_stringize_adjacent.c',
        note='token paste plus stringized __LINE__ should preserve remapped source coordinates beside adjacent __FILE__ strings at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave14_spliced_nested_macro_file_line',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave14_spliced_nested_macro_file_line.c',
        note='line-spliced token-formed macro call should preserve remapped file and line provenance through adjacent strings at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave15_comment_splice_file_boundary',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave15_comment_splice_file_boundary.c',
        note='comment replacement plus line-spliced adjacent strings should preserve remapped file and line provenance at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave15_nested_include_line_boundary',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave15_nested_include_line_boundary.c',
        note='nested include #line boundaries should preserve inner and outer virtual source coordinates at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave15_spliced_macro_arg_source',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave15_spliced_macro_arg_source.c',
        note='line-spliced macro argument tokens should preserve remapped file and line provenance at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave16_splice_stringize_tokenpaste',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave16_splice_stringize_tokenpaste.c',
        note='strict frontier: line-spliced adjacent strings beside comment deletion, token paste, and stringized __LINE__ should preserve remapped provenance at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave16_nested_include_resumption',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave16_nested_include_resumption.c',
        note='nested include #line mapping should preserve inner source coordinates and resume outer virtual coordinates at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave16_splice_stringize_tokenpaste_current',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave16_splice_stringize_tokenpaste_current.c',
        note='current-threshold companion without the splice/comment boundary should preserve token paste and stringized __LINE__ provenance at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave16_macro_line_filename',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave16_macro_line_filename.c',
        note='macro-expanded #line filename should propagate into adjacent-string __FILE__ and stringized __LINE__ runtime provenance',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave17_multi_include_resumption',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave17_multi_include_resumption.c',
        note='multi-level nested include #line mapping should preserve leaf coordinates and resume outer virtual coordinates at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave17_spliced_macro_comment_source',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave17_spliced_macro_comment_source.c',
        note='line-spliced adjacent source text plus comment replacement, token paste, and stringized __LINE__ should preserve remapped provenance at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave17_macro_file_paste_line',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave17_macro_file_paste_line.c',
        note='macro-expanded #line filename with token paste and stringized line values should preserve runtime source provenance',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave17_adjacent_string_include_line',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave17_adjacent_string_include_line.c',
        note='include-header adjacent string literal provenance should preserve remapped __FILE__ and __LINE__ values at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave18_spliced_line_directive_adjacent',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave18_spliced_line_directive_adjacent.c',
        note='line-spliced adjacent source text with comment replacement should preserve macro-expanded #line filename provenance at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave18_include_reentry_source_stack',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave18_include_reentry_source_stack.c',
        note='include re-entry and nested include source stack should preserve remapped file/line coordinates across include return boundaries at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave18_macro_file_line_paste',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave18_macro_file_line_paste.c',
        note='macro-expanded #line filename with token paste and stringized line values should preserve runtime source provenance',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave18_comment_adjacent_source_map',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave18_comment_adjacent_source_map.c',
        note='comment replacement inside adjacent strings should preserve remapped __FILE__ and __LINE__ source mapping at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave19_include_resumption_runtime_boundary',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave19_include_resumption_runtime_boundary.c',
        note='include return boundary should preserve caller virtual #line coordinates after a remapped header at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave19_splice_adjacent_source_provenance',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave19_splice_adjacent_source_provenance.c',
        note='line-spliced adjacent source text with comment deletion should preserve remapped runtime provenance',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave19_include_tokenpaste_adjacent_boundary',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave19_include_tokenpaste_adjacent_boundary.c',
        note='include-header token paste and adjacent string provenance should preserve remapped file and line values at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave20_trigraph_splice_comment_adjacency',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave20_trigraph_splice_comment_adjacency.c',
        note='trigraph backslash splicing across comment and adjacent string boundaries should preserve remapped source provenance at runtime',
        fisics_args=['--trigraphs'],
        clang_args=['-trigraphs'],
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave20_splice_comment_adjacency_current',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave20_splice_comment_adjacency_current.c',
        note='current-threshold ordinary backslash splice across comment and adjacent string boundaries should preserve remapped source provenance at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave20_nested_include_return_boundary',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave20_nested_include_return_boundary.c',
        note='nested include return should preserve inner source coordinates and resume caller virtual source coordinates at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave20_tokenpaste_stringize_source_provenance',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave20_tokenpaste_stringize_source_provenance.c',
        note='token paste and stringize source provenance should preserve macro filename and remapped line values at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave20_adjacent_string_line_boundary',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave20_adjacent_string_line_boundary.c',
        note='adjacent string literal source-line boundaries should preserve remapped file and line values at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave21_nested_include_source_stack',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave21_nested_include_source_stack.c',
        note='nested include source stack should preserve leaf coordinates and resume caller virtual source coordinates at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave21_splice_comment_adjacent_source',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave21_splice_comment_adjacent_source.c',
        note='ordinary backslash splice across comment replacement and adjacent strings should preserve remapped source provenance at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave21_tokenpaste_stringize_line_provenance',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave21_tokenpaste_stringize_line_provenance.c',
        note='token paste and stringize source provenance should preserve macro-expanded filename and remapped line values at runtime',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave21_trigraph_stringize_boundary',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave21_trigraph_stringize_boundary.c',
        note='explicit trigraph backslash splicing across comment and adjacent string boundaries should preserve remapped source provenance at runtime',
        fisics_args=['--trigraphs'],
        clang_args=['-trigraphs'],
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave22_layered_macro_line_checksum',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave22_layered_macro_line_checksum.c',
        note='layered macro forwarding should preserve direct #line file and line provenance through a runtime checksum',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave22_trigraph_splice_comment_checksum',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave22_trigraph_splice_comment_checksum.c',
        note='trigraph splicing and comment replacement should preserve virtual source provenance in a runtime checksum',
        fisics_args=['--trigraphs'],
        clang_args=['-trigraphs'],
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave22_macro_line_filename_checksum',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave22_macro_line_filename_checksum.c',
        note='macro-expanded #line filename and stringized mapped line should preserve runtime provenance through layered macros',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave23_object_rescan_stringize_control',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave23_object_rescan_stringize_control.c',
        note='control lane: a pasted object-like macro name should rescan to its value while expanded and raw stringization remain observably distinct',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave23_pasted_function_rescan_strict',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave23_pasted_function_rescan_strict.c',
        note='strict frontier: a pasted identifier followed by an argument list should rescan as a function-like macro invocation',
    ),
    RuntimeProbe(
        probe_id='01__probe_runtime_wave23_pasted_function_provenance_strict',
        source=PROBE_DIR / 'runtime/01__probe_runtime_wave23_pasted_function_provenance_strict.c',
        note='strict frontier: pasted function-like macro selection should preserve remapped file and line provenance identically to a direct invocation',
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
        required_substrings=['Error at (1814:'],
    ),
    DiagnosticProbe(
        probe_id='01__probe_line_directive_include_tokenpaste_undeclared_identifier_location_strict',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_include_tokenpaste_undeclared_identifier_location_strict.c',
        note='#line include-header token-paste should preserve remapped undeclared-identifier location',
        required_substrings=['Error at (1914:'],
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
        expected_line=1814,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='01__probe_diagjson_line_directive_include_tokenpaste_undeclared_identifier_location_strict',
        source=PROBE_DIR / 'diagnostics/01__probe_line_directive_include_tokenpaste_undeclared_identifier_location_strict.c',
        note='diagnostics JSON should preserve remapped include-header token-paste location',
        expected_codes=[2000],
        expected_line=1914,
        expected_has_file=True,
    ),
]
