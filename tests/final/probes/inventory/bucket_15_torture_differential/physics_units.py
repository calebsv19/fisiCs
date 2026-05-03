from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent
REPO_ROOT = PROBE_DIR.parent.parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='15__probe_units_runtime_conversion_chain',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_conversion_chain.c',
        note='physics-units legal conversion chain should match clang runtime behavior on a deterministic speed-length-force-charge-energy bundle',
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_formula_graph_call_boundary',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_formula_graph_call_boundary.c',
        note='physics-units legal multi-helper formula graph should match clang runtime behavior while stressing annotated parameter boundaries, explicit conversion helpers, and derived energy-power pressure',
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_conversion_reduction_ladder',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_conversion_reduction_ladder.c',
        note='physics-units legal reduction ladder should match clang runtime behavior across repeated distance, energy, voltage, charge, and pressure conversions with same-unit aggregation after normalization',
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_return_bridge_pipeline',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_return_bridge_pipeline.c',
        note='physics-units legal return-bridge pipeline should match clang runtime behavior while chaining helper-returned converted values through staged distance, work, and total-energy helpers',
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_loop_normalization_mesh',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_loop_normalization_mesh.c',
        note='physics-units legal loop-carried normalization mesh should match clang runtime behavior while repeatedly converting distance, energy, and voltage families inside accumulator loops',
        fisics_args=['--overlay=physics-units'],
    ),
    RuntimeProbe(
        probe_id='15__probe_units_runtime_branch_conversion_mesh',
        source=PROBE_DIR / 'runtime/15__probe_units_runtime_branch_conversion_mesh.c',
        note='physics-units legal branch-selected conversion mesh should match clang runtime behavior while mixing direct and explicitly converted force, distance, and charge lanes through helper-returned values',
        fisics_args=['--overlay=physics-units'],
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id='15__probe_units_ballistics_valid_dump_contract',
        source=REPO_ROOT / 'examples/physics_units/ballistics_valid.c',
        note='physics-units valid ballistics pilot should compile under overlay mode and expose stable sema markers for resolved units plus legal intermediate multiply steps',
        expect_any_diagnostic=False,
        required_substrings=[
            'line 4: m [resolved, unit=meter, unit-resolved]',
            'line 10: m^2*kg/s^2 [resolved, unit=joule, unit-resolved]',
            'binary-expression(*) [dim=m/s, resolved]',
            'binary-expression(*) [dim=m^2*kg/s^2, resolved]',
        ],
        forbidden_substrings=[
            'SemanticModel: 1 error(s)',
            'Skipping LLVM code generation due to semantic errors.',
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_units_alias_conversion_chain_dump_contract',
        source=PROBE_DIR / 'diagnostics/15__probe_units_alias_conversion_chain_dump_contract.c',
        note='alias spellings should remain accepted while dump output canonicalizes concrete-unit identity across speed, power, and voltage conversion assignments',
        expect_any_diagnostic=False,
        required_substrings=[
            'variable: launch_speed [units=m/s, resolved, unit=foot_per_second, unit-resolved, decl-index=0]',
            'variable: load_kw [units=m^2*kg/s^3, resolved, unit=kilowatt, unit-resolved, decl-index=0]',
            'variable: cell_drop [units=m^2*kg/s^3*A, resolved, unit=millivolt, unit-resolved, decl-index=0]',
            'assignment(=) [dim=m^2*kg/s^3, resolved, unit=watt]',
            'assignment(=) [dim=m^2*kg/s^3*A, resolved, unit=kilovolt]',
        ],
        forbidden_substrings=[
            'SemanticModel: 1 error(s)',
            'Skipping LLVM code generation due to semantic errors.',
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_units_formula_graph_call_boundary_dump_contract',
        source=PROBE_DIR / 'diagnostics/15__probe_units_formula_graph_call_boundary_dump_contract.c',
        note='multi-helper formula graph should keep annotated parameter boundaries, explicit conversion calls, and derived distance-energy stages visible in the semantic dump',
        expect_any_diagnostic=False,
        required_substrings=[
            'line 2: m/s [resolved, unit=foot_per_second, unit-resolved]',
            'line 11: m*kg/s^2 [resolved, unit=pound_force, unit-resolved]',
            'function-call(fisics_convert_unit) [dim=m/s, resolved, unit=meter_per_second]',
            'function-call(fisics_convert_unit) [dim=m*kg/s^2, resolved, unit=newton]',
            'binary-expression(*) [dim=m, resolved]',
            'binary-expression(*) [dim=m^2*kg/s^2, resolved]',
        ],
        forbidden_substrings=[
            'SemanticModel: 1 error(s)',
            'Skipping LLVM code generation due to semantic errors.',
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_units_conversion_ladder_dump_contract',
        source=PROBE_DIR / 'diagnostics/15__probe_units_conversion_ladder_dump_contract.c',
        note='conversion-heavy reduction ladder should keep repeated normalization calls and same-unit aggregation stages visible in the semantic dump across distance, energy, and voltage families',
        expect_any_diagnostic=False,
        required_substrings=[
            'line 2: m [resolved, unit=mile, unit-resolved]',
            'function-call(fisics_convert_unit) [dim=m, resolved, unit=meter]',
            'binary-expression(+) [dim=m, resolved, unit=meter]',
            'function-call(fisics_convert_unit) [dim=m^2*kg/s^2, resolved, unit=joule]',
            'binary-expression(+) [dim=m^2*kg/s^2, resolved, unit=joule]',
            'function-call(fisics_convert_unit) [dim=m^2*kg/s^3*A, resolved, unit=kilovolt]',
            'binary-expression(+) [dim=m^2*kg/s^3*A, resolved, unit=kilovolt]',
        ],
        forbidden_substrings=[
            'SemanticModel: 1 error(s)',
            'Skipping LLVM code generation due to semantic errors.',
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_units_return_bridge_pipeline_dump_contract',
        source=PROBE_DIR / 'diagnostics/15__probe_units_return_bridge_pipeline_dump_contract.c',
        note='helper-returned converted values should remain visible through staged distance and total-energy helpers so the semantic dump still exposes boundary conversions plus the final energy aggregation',
        expect_any_diagnostic=False,
        required_substrings=[
            'line 2: m/s [resolved, unit=foot_per_second, unit-resolved]',
            'line 11: m*kg/s^2 [resolved, unit=pound_force, unit-resolved]',
            'line 19: m^2*kg/s^2 [resolved, unit=watt_hour, unit-resolved]',
            'function-call(fisics_convert_unit) [dim=m/s, resolved, unit=meter_per_second]',
            'function-call(fisics_convert_unit) [dim=m*kg/s^2, resolved, unit=newton]',
            'function-call(fisics_convert_unit) [dim=m^2*kg/s^2, resolved, unit=joule]',
            'binary-expression(*) [dim=m, resolved]',
            'binary-expression(*) [dim=m^2*kg/s^2, resolved]',
            'binary-expression(+) [dim=m^2*kg/s^2, resolved, unit=joule]',
        ],
        forbidden_substrings=[
            'SemanticModel: 1 error(s)',
            'Skipping LLVM code generation due to semantic errors.',
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_units_loop_normalization_mesh_dump_contract',
        source=PROBE_DIR / 'diagnostics/15__probe_units_loop_normalization_mesh_dump_contract.c',
        note='loop-carried normalization mesh should keep repeated conversion calls and same-unit accumulator updates visible in the semantic dump across distance, energy, and voltage families',
        expect_any_diagnostic=False,
        required_substrings=[
            'function-call(fisics_convert_unit) [dim=m, resolved, unit=meter]',
            'function-call(fisics_convert_unit) [dim=m^2*kg/s^2, resolved, unit=joule]',
            'function-call(fisics_convert_unit) [dim=m^2*kg/s^3*A, resolved, unit=kilovolt]',
            'binary-expression(+) [dim=m, resolved, unit=meter]',
            'binary-expression(+) [dim=m^2*kg/s^2, resolved, unit=joule]',
            'binary-expression(+) [dim=m^2*kg/s^3*A, resolved, unit=kilovolt]',
        ],
        forbidden_substrings=[
            'SemanticModel: 1 error(s)',
            'Skipping LLVM code generation due to semantic errors.',
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_units_branch_conversion_mesh_dump_contract',
        source=PROBE_DIR / 'diagnostics/15__probe_units_branch_conversion_mesh_dump_contract.c',
        note='branch-selected conversion mesh should keep helper-internal conversion calls and same-unit accumulation stages visible in the semantic dump across force and distance families',
        expect_any_diagnostic=False,
        required_substrings=[
            'function-call(fisics_convert_unit) [dim=m*kg/s^2, resolved, unit=newton]',
            'function-call(fisics_convert_unit) [dim=m, resolved, unit=meter]',
            'binary-expression(+) [dim=m*kg/s^2, resolved, unit=newton]',
            'binary-expression(+) [dim=m, resolved, unit=meter]',
        ],
        forbidden_substrings=[
            'SemanticModel: 1 error(s)',
            'Skipping LLVM code generation due to semantic errors.',
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_implicit_compare_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_implicit_compare_requires_convert.c',
        note='same-dimension comparison across meter and foot storage should require an explicit conversion boundary under overlay semantics',
        required_substrings=[
            "implicit concrete unit conversion in comparison requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_implicit_ternary_result_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_implicit_ternary_result_requires_convert.c',
        note='same-dimension ternary result merge across meter-per-second and foot-per-second storage should require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in ternary result requires explicit conversion from 'foot_per_second' to 'meter_per_second'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_compound_assignment_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_compound_assignment_requires_convert.c',
        note='same-dimension compound assignment across ampere-hour and milliampere-hour storage should require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in compound assignment requires explicit conversion from 'milliampere_hour' to 'ampere_hour'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_implicit_add_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_implicit_add_requires_convert.c',
        note='same-dimension addition across meter and foot storage should require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in addition requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_convert_invalid_target',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_convert_invalid_target.c',
        note='explicit helper conversion should reject target strings that do not resolve through the seeded unit registry',
        required_substrings=[
            "invalid explicit units conversion target 'bogus_unit_name'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_convert_incompatible',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_convert_incompatible.c',
        note='explicit helper conversion should reject known target units from an incompatible family or dimension',
        required_substrings=[
            "explicit units conversion from 'meter' to 'second' is not allowed",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_convert_missing_source',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_convert_missing_source.c',
        note='explicit helper conversion should reject source expressions that have a dimension but no resolved concrete unit',
        required_substrings=[
            "explicit units conversion to 'foot' requires a resolved source concrete unit",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_convert_requires_floating',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_convert_requires_floating.c',
        note='explicit helper conversion should reject non-floating source expressions in the first helper lane',
        required_substrings=[
            "explicit units conversion helper 'fisics_convert_unit' currently requires a floating scalar source expression",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_call_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_call_argument_requires_convert.c',
        note='same-dimension unit changes across a typed helper parameter should require an explicit conversion boundary at the call edge',
        required_substrings=[
            "implicit concrete unit conversion in argument requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_return_pipeline_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_return_pipeline_argument_requires_convert.c',
        note='helper-returned same-dimension values should still require an explicit conversion boundary when they flow into a typed parameter with a different concrete unit',
        required_substrings=[
            "implicit concrete unit conversion in argument requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_initializer_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_initializer_requires_convert.c',
        note='same-dimension unit changes in an annotated local initializer should require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in initializer requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_fnptr_call_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_fnptr_call_argument_requires_convert.c',
        note='same-dimension unit changes passed through a typed function-pointer call should still require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in argument requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_fnptr_return_pipeline_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_fnptr_return_pipeline_argument_requires_convert.c',
        note='helper-returned same-dimension values routed through a typed function-pointer call should still require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in argument requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_struct_fnptr_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_struct_fnptr_argument_requires_convert.c',
        note='same-dimension unit changes passed through a struct-held typed function pointer should still require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in argument requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_callback_param_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_callback_param_argument_requires_convert.c',
        note='same-dimension unit changes passed through a typed callback parameter should still require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in argument requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_table_fnptr_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_table_fnptr_argument_requires_convert.c',
        note='same-dimension unit changes passed through a table-dispatched typed function pointer should still require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in argument requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_ternary_callee_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_ternary_callee_argument_requires_convert.c',
        note='same-dimension unit changes passed through a ternary-selected typed function pointer should still require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in argument requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_deref_callee_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_deref_callee_argument_requires_convert.c',
        note='same-dimension unit changes passed through an explicitly dereferenced typed function pointer should still require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in argument requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_returned_callee_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_returned_callee_argument_requires_convert.c',
        note='same-dimension unit changes passed through a helper-returned typed callee should still require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in argument requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_comma_callee_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_comma_callee_argument_requires_convert.c',
        note='same-dimension unit changes passed through a comma-selected typed callee should still require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in argument requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_nested_callback_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_nested_callback_argument_requires_convert.c',
        note='same-dimension unit changes passed through nested callback-forwarding helpers should still require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in argument requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
    DiagnosticProbe(
        probe_id='15__probe_diag_units_table_relay_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_table_relay_argument_requires_convert.c',
        note='same-dimension unit changes passed through a table relay helper should still require an explicit conversion boundary',
        required_substrings=[
            "implicit concrete unit conversion in argument requires explicit conversion from 'foot' to 'meter'",
        ],
        fisics_args=['--overlay=physics-units', '--dump-sema', '-c'],
    ),
]

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_implicit_compare_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_implicit_compare_requires_convert.c',
        note='structured diagnostics for implicit same-dimension comparison conversion should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_implicit_add_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_implicit_add_requires_convert.c',
        note='structured diagnostics for same-dimension addition conversion rejection should stay in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_convert_invalid_target',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_convert_invalid_target.c',
        note='structured diagnostics for invalid explicit target-unit text should stay in the dedicated conversion-invalid-target code lane',
        expected_codes=[4108],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_convert_incompatible',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_convert_incompatible.c',
        note='structured diagnostics for explicit conversion across incompatible unit families should stay in the dedicated incompatible-conversion code lane',
        expected_codes=[4109],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_convert_missing_source',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_convert_missing_source.c',
        note='structured diagnostics for explicit conversion without a resolved source concrete unit should stay in the dedicated missing-source code lane',
        expected_codes=[4110],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_convert_requires_floating',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_convert_requires_floating.c',
        note='structured diagnostics for explicit conversion on a non-floating source should stay in the dedicated floating-source code lane',
        expected_codes=[4111],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_call_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_call_argument_requires_convert.c',
        note='structured diagnostics for typed helper parameter same-dimension conversion rejection should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_return_pipeline_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_return_pipeline_argument_requires_convert.c',
        note='structured diagnostics for helper-returned same-dimension values routed into typed parameters should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_initializer_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_initializer_requires_convert.c',
        note='structured diagnostics for initializer same-dimension conversion rejection should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_fnptr_call_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_fnptr_call_argument_requires_convert.c',
        note='structured diagnostics for typed function-pointer same-dimension conversion rejection should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_fnptr_return_pipeline_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_fnptr_return_pipeline_argument_requires_convert.c',
        note='structured diagnostics for helper-returned same-dimension values routed through a typed function-pointer call should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_struct_fnptr_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_struct_fnptr_argument_requires_convert.c',
        note='structured diagnostics for struct-held typed function-pointer same-dimension conversion rejection should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_callback_param_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_callback_param_argument_requires_convert.c',
        note='structured diagnostics for typed callback-parameter same-dimension conversion rejection should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_table_fnptr_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_table_fnptr_argument_requires_convert.c',
        note='structured diagnostics for table-dispatched typed function-pointer same-dimension conversion rejection should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_ternary_callee_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_ternary_callee_argument_requires_convert.c',
        note='structured diagnostics for ternary-selected typed function-pointer same-dimension conversion rejection should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_deref_callee_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_deref_callee_argument_requires_convert.c',
        note='structured diagnostics for explicitly dereferenced typed function-pointer same-dimension conversion rejection should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_returned_callee_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_returned_callee_argument_requires_convert.c',
        note='structured diagnostics for helper-returned typed-callee same-dimension conversion rejection should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_comma_callee_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_comma_callee_argument_requires_convert.c',
        note='structured diagnostics for comma-selected typed-callee same-dimension conversion rejection should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_nested_callback_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_nested_callback_argument_requires_convert.c',
        note='structured diagnostics for nested callback-forwarding same-dimension conversion rejection should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
    DiagnosticJsonProbe(
        probe_id='15__probe_diagjson_units_table_relay_argument_requires_convert',
        source=PROBE_DIR / 'diagnostics/15__probe_diag_units_table_relay_argument_requires_convert.c',
        note='structured diagnostics for table-relay same-dimension conversion rejection should remain in the extension implicit-conversion code lane',
        expected_codes=[4107],
        fisics_args=['--overlay=physics-units'],
    ),
]
