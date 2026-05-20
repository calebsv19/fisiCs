from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id='03__probe_include_collision_inactive_angle_active_quote_long_clean',
        source=PROBE_DIR.parent / 'cases/03__include_collision_inactive_angle_active_quote_long_clean.c',
        note='clean-path parity: dead angle-include collision branch should not perturb the active longer quote-chain local-first resolution',
        expect_any_diagnostic=False,
        fisics_args=[
            f'-I{PROBE_DIR.parent / "cases/pp_collide2_p1"}',
            f'-I{PROBE_DIR.parent / "cases/pp_collide2_p2"}',
            f'-I{PROBE_DIR.parent / "cases/pp_collide2_p3"}',
        ],
    ),
    DiagnosticProbe(
        probe_id='03__probe_include_cycle_inactive_reject_active_pragma_once_clean',
        source=PROBE_DIR.parent / 'cases/03__include_cycle_inactive_reject_active_pragma_once_clean.c',
        note='clean-path parity: a dead recursive include cycle should stay inert while the active pure pragma-once three-hop graph resolves cleanly',
        expect_any_diagnostic=False,
    ),
    DiagnosticProbe(
        probe_id='03__probe_defined_include_nested_post_inactive_noise_clean',
        source=PROBE_DIR.parent / 'cases/03__defined_include_nested_post_inactive_noise_clean.c',
        note='clean-path parity: post-include inactive macro-state noise should not perturb nested include defined/undef convergence',
        expect_any_diagnostic=False,
    ),
    DiagnosticProbe(
        probe_id='03__probe_include_cycle_inactive_reject_active_guard_clean',
        source=PROBE_DIR.parent / 'cases/03__include_cycle_inactive_reject_active_guard_clean.c',
        note='clean-path parity: a dead recursive include cycle should stay inert while the active include-guard three-hop graph resolves cleanly',
        expect_any_diagnostic=False,
    ),
    DiagnosticProbe(
        probe_id='03__probe_include_cycle_inactive_reject_active_mixed_guard_once_clean',
        source=PROBE_DIR.parent / 'cases/03__include_cycle_inactive_reject_active_mixed_guard_once_clean.c',
        note='clean-path parity: a dead recursive include cycle should stay inert while the active mixed include-guard/pragma-once graph resolves cleanly',
        expect_any_diagnostic=False,
    ),
    DiagnosticProbe(
        probe_id='03__probe_defined_include_nested_inactive_noise_clean',
        source=PROBE_DIR.parent / 'cases/03__defined_include_nested_inactive_noise_clean.c',
        note='clean-path parity: inactive macro-state noise should not perturb nested include defined/undef convergence',
        expect_any_diagnostic=False,
    ),
    DiagnosticProbe(
        probe_id='03__probe_include_collision_inactive_quote_active_angle_long_clean',
        source=PROBE_DIR.parent / 'cases/03__include_collision_inactive_quote_active_angle_long_clean.c',
        note='clean-path parity: dead quote-include collision branch should not perturb the active longer angle-chain search-order resolution',
        expect_any_diagnostic=False,
        fisics_args=[
            f'-I{PROBE_DIR.parent / "cases/pp_collide2_p1"}',
            f'-I{PROBE_DIR.parent / "cases/pp_collide2_p2"}',
            f'-I{PROBE_DIR.parent / "cases/pp_collide2_p3"}',
        ],
    ),
    DiagnosticProbe(
        probe_id='03__probe_include_collision_inactive_angle_active_mixed_quote_clean',
        source=PROBE_DIR.parent / 'cases/03__include_collision_inactive_angle_active_mixed_quote_clean.c',
        note='clean-path parity: dead angle-include collision branch should not perturb the active mixed-delimiter local-first resolution chain',
        expect_any_diagnostic=False,
        fisics_args=[
            f'-I{PROBE_DIR.parent / "cases/pp_mix2_p1"}',
            f'-I{PROBE_DIR.parent / "cases/pp_mix2_p2"}',
            f'-I{PROBE_DIR.parent / "cases/pp_mix2_p3"}',
            f'-I{PROBE_DIR.parent / "cases/pp_mix2_p4"}',
        ],
    ),
    DiagnosticProbe(
        probe_id='03__probe_defined_include_nested_undef_clean',
        source=PROBE_DIR.parent / 'cases/03__defined_include_nested_undef_clean.c',
        note='clean-path parity: nested include macro state should preserve defined visibility, ordered undefs, and final post-chain convergence',
        expect_any_diagnostic=False,
    ),
    DiagnosticProbe(
        probe_id='03__probe_include_collision_inactive_quote_active_angle_clean',
        source=PROBE_DIR.parent / 'cases/03__include_collision_inactive_quote_active_angle_clean.c',
        note='clean-path parity: dead quote-include collision branch should not perturb active angle-chain search-order resolution',
        expect_any_diagnostic=False,
        fisics_args=[
            f'-I{PROBE_DIR.parent / "cases/pp_collide_p1"}',
            f'-I{PROBE_DIR.parent / "cases/pp_collide_p2"}',
        ],
    ),
    DiagnosticProbe(
        probe_id='03__probe_include_collision_inactive_angle_active_quote_clean',
        source=PROBE_DIR.parent / 'cases/03__include_collision_inactive_angle_active_quote_clean.c',
        note='clean-path parity: dead angle-include collision branch should not perturb active quote-chain local-first resolution',
        expect_any_diagnostic=False,
        fisics_args=[
            f'-I{PROBE_DIR.parent / "cases/pp_collide_p1"}',
            f'-I{PROBE_DIR.parent / "cases/pp_collide_p2"}',
        ],
    ),
    DiagnosticProbe(
        probe_id='03__probe_defined_include_cross_header_undef_clean',
        source=PROBE_DIR.parent / 'cases/03__defined_include_cross_header_undef_clean.c',
        note='clean-path parity: macro definition visibility and #undef convergence should remain consistent across include boundaries',
        expect_any_diagnostic=False,
    ),
    DiagnosticProbe(
        probe_id='03__probe_include_next_inactive_missing_skip_order_chain_clean',
        source=PROBE_DIR.parent / 'cases/03__include_next_inactive_missing_skip_order_chain_clean.c',
        note='clean-path parity: dead #if branch should skip missing #include_next while active order chain still resolves cleanly',
        expect_any_diagnostic=False,
        fisics_args=[
            f'-I{PROBE_DIR.parent / "cases/pp_inext_ord_p1"}',
            f'-I{PROBE_DIR.parent / "cases/pp_inext_ord_p2"}',
            f'-I{PROBE_DIR.parent / "cases/pp_inext_ord_p3"}',
            f'-I{PROBE_DIR.parent / "cases/pp_inext_ord_p4"}',
        ],
    ),
    DiagnosticProbe(
        probe_id='03__probe_include_next_inactive_invalid_skip_mixed_chain_clean',
        source=PROBE_DIR.parent / 'cases/03__include_next_inactive_invalid_skip_mixed_chain_clean.c',
        note='clean-path parity: dead #if branch should skip invalid #include_next operand while active mixed-delimiter chain still resolves cleanly',
        expect_any_diagnostic=False,
        fisics_args=[
            f'-I{PROBE_DIR.parent / "cases/pp_inext_mix_p1"}',
            f'-I{PROBE_DIR.parent / "cases/pp_inext_mix_p2"}',
            f'-I{PROBE_DIR.parent / "cases/pp_inext_mix_p3"}',
            f'-I{PROBE_DIR.parent / "cases/pp_inext_mix_p4"}',
        ],
    ),
    DiagnosticProbe(
        probe_id='03__probe_include_next_nested_inactive_expr_once_chain_clean',
        source=PROBE_DIR.parent / 'cases/03__include_next_nested_inactive_expr_once_chain_clean.c',
        note='clean-path parity: nested inactive bad expression and missing #include_next should be skipped while active pragma-once include_next chain stays clean',
        expect_any_diagnostic=False,
        fisics_args=[
            f'-I{PROBE_DIR.parent / "cases/pp_inext_once_p1"}',
            f'-I{PROBE_DIR.parent / "cases/pp_inext_once_p2"}',
        ],
    ),
    DiagnosticProbe(
        probe_id='03__probe_include_next_extra_tokens_diag_current_text',
        source=PROBE_DIR.parent / 'cases/03__include_next_extra_tokens_reject.c',
        note='current threshold: invalid #include_next operand with trailing tokens still emits text diagnostics even without diagjson export',
        required_substrings=['invalid #include operand'],
    ),
    DiagnosticProbe(
        probe_id='03__probe_include_next_macro_trailing_diag_current_text',
        source=PROBE_DIR.parent / 'cases/03__include_next_macro_trailing_tokens_reject.c',
        note='current threshold: macro-expanded invalid #include_next operand still emits text diagnostics even without diagjson export',
        required_substrings=['invalid #include operand'],
    ),
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
        probe_id='03__probe_diagjson_include_next_missing_target_basic_strict',
        source=PROBE_DIR.parent / 'cases/03__include_next_missing.c',
        note='strict parity: missing first-hop #include_next target should export preprocessor diagnostics JSON with file, line, and column',
        expected_codes=[3000],
        expected_line=1,
        expected_column=15,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='03__probe_diagjson_include_next_missing_target_mid_chain_strict',
        source=PROBE_DIR.parent / 'cases/03__include_next_missing_mid_chain.c',
        note='strict parity: missing mid-chain #include_next target should export preprocessor diagnostics JSON with file, line, and column',
        expected_codes=[3000],
        expected_line=1,
        expected_column=10,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='03__probe_diagjson_include_next_extra_tokens_strict',
        source=PROBE_DIR.parent / 'cases/03__include_next_extra_tokens_reject.c',
        note='strict frontier: invalid #include_next operand with trailing tokens should still export diagnostics JSON',
        expected_codes=[3000],
        expected_line=1,
        expected_column=1,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='03__probe_diagjson_include_next_macro_trailing_strict',
        source=PROBE_DIR.parent / 'cases/03__include_next_macro_trailing_tokens_reject.c',
        note='strict frontier: macro-expanded invalid #include_next operand should still export diagnostics JSON',
        expected_codes=[3000],
        expected_line=2,
        expected_column=1,
        expected_has_file=True,
    ),
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
