from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='05__probe_typedef_shadow_parenthesized_expr',
        source=PROBE_DIR / 'runtime/05__probe_typedef_shadow_parenthesized_expr.c',
        note='parenthesized identifier should win over cast when typedef is shadowed',
    ),
    RuntimeProbe(
        probe_id='05__probe_precedence_runtime',
        source=PROBE_DIR / 'runtime/05__probe_precedence_runtime.c',
        note='operator precedence runtime result should match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_unsigned_arith_conv_runtime',
        source=PROBE_DIR / 'runtime/05__probe_unsigned_arith_conv_runtime.c',
        note='usual arithmetic conversions for signed/unsigned mix should match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_nested_ternary_runtime',
        source=PROBE_DIR / 'runtime/05__probe_nested_ternary_runtime.c',
        note='nested ternary associativity/runtime behavior should match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_nested_ternary_outer_true_runtime',
        source=PROBE_DIR / 'runtime/05__probe_nested_ternary_outer_true_runtime.c',
        note='nested ternary outer-true path should match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_nested_ternary_false_chain_runtime',
        source=PROBE_DIR / 'runtime/05__probe_nested_ternary_false_chain_runtime.c',
        note='nested ternary false-chain path should match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_short_circuit_and_runtime',
        source=PROBE_DIR / 'runtime/05__probe_short_circuit_and_runtime.c',
        note='logical AND short-circuit side effects should match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_short_circuit_or_runtime',
        source=PROBE_DIR / 'runtime/05__probe_short_circuit_or_runtime.c',
        note='logical OR short-circuit side effects should match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_comma_eval_runtime',
        source=PROBE_DIR / 'runtime/05__probe_comma_eval_runtime.c',
        note='comma operator evaluation/value behavior should match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_vla_sizeof_side_effect_runtime',
        source=PROBE_DIR / 'runtime/05__probe_vla_sizeof_side_effect_runtime.c',
        note='sizeof(VLA) should evaluate bound side effects and match clang runtime',
    ),
    RuntimeProbe(
        probe_id='05__probe_ternary_side_effect_runtime',
        source=PROBE_DIR / 'runtime/05__probe_ternary_side_effect_runtime.c',
        note='conditional operator branch side effects should match clang runtime',
    ),
    RuntimeProbe(
        probe_id='05__probe_nested_ternary_deep_false_chain_runtime',
        source=PROBE_DIR / 'runtime/05__probe_nested_ternary_deep_false_chain_runtime.c',
        note='deep nested ternary false-chain path should match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_compound_literal_array_runtime',
        source=PROBE_DIR / 'runtime/05__probe_compound_literal_array_runtime.c',
        note='compound-literal array value/lifetime-in-block behavior should match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_sizeof_nonvla_unevaluated_runtime',
        source=PROBE_DIR / 'runtime/05__probe_sizeof_nonvla_unevaluated_runtime.c',
        note='sizeof on non-VLA expression should not evaluate assignment side effects',
    ),
    RuntimeProbe(
        probe_id='05__probe_sizeof_null_deref_unevaluated_runtime',
        source=PROBE_DIR / 'runtime/05__probe_sizeof_null_deref_unevaluated_runtime.c',
        note='sizeof on dereferenced null pointer expression should remain unevaluated and match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_ternary_inactive_comma_runtime',
        source=PROBE_DIR / 'runtime/05__probe_ternary_inactive_comma_runtime.c',
        note='inactive ternary branch with comma side effects should remain unevaluated and match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_nested_ternary_short_circuit_runtime',
        source=PROBE_DIR / 'runtime/05__probe_nested_ternary_short_circuit_runtime.c',
        note='nested ternary sequencing on the taken branch should match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_logical_and_comma_chain_runtime',
        source=PROBE_DIR / 'runtime/05__probe_logical_and_comma_chain_runtime.c',
        note='logical AND with comma-chain rhs should preserve defined sequencing and match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_logical_or_nested_comma_runtime',
        source=PROBE_DIR / 'runtime/05__probe_logical_or_nested_comma_runtime.c',
        note='logical OR should skip nested comma side effects on truthy lhs and match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_logical_and_false_skips_nested_comma_runtime',
        source=PROBE_DIR / 'runtime/05__probe_logical_and_false_skips_nested_comma_runtime.c',
        note='logical AND with false lhs should skip nested comma side effects and match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_logical_or_false_executes_nested_comma_runtime',
        source=PROBE_DIR / 'runtime/05__probe_logical_or_false_executes_nested_comma_runtime.c',
        note='logical OR with false lhs should execute nested comma rhs and match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_ternary_condition_comma_gate_runtime',
        source=PROBE_DIR / 'runtime/05__probe_ternary_condition_comma_gate_runtime.c',
        note='comma sequencing inside ternary condition and chosen branch should match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_nested_ternary_postinc_path_runtime',
        source=PROBE_DIR / 'runtime/05__probe_nested_ternary_postinc_path_runtime.c',
        note='nested ternary post-increment chosen-path sequencing should match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_logical_and_vla_sizeof_eval_runtime',
        source=PROBE_DIR / 'runtime/05__probe_logical_and_vla_sizeof_eval_runtime.c',
        note='logical AND should evaluate VLA sizeof rhs only when lhs is truthy and match clang',
    ),
    RuntimeProbe(
        probe_id='05__probe_logical_or_vla_sizeof_skip_runtime',
        source=PROBE_DIR / 'runtime/05__probe_logical_or_vla_sizeof_skip_runtime.c',
        note='logical OR should skip VLA sizeof rhs side effects on truthy lhs and match clang',
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id='05__probe_conditional_void_condition_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_conditional_void_condition_reject.c',
        note='conditional operator first operand must be scalar (void expression should reject)',
    ),
    DiagnosticProbe(
        probe_id='05__probe_conditional_struct_result_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_conditional_struct_result_reject.c',
        note='conditional expression with struct result should be rejected in int return context',
    ),
    DiagnosticProbe(
        probe_id='05__probe_logical_and_struct_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_logical_and_struct_reject.c',
        note='logical && operands must be scalar (struct operand should reject)',
    ),
    DiagnosticProbe(
        probe_id='05__probe_unary_not_struct_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_unary_not_struct_reject.c',
        note='unary ! operand must be scalar (struct operand should reject)',
    ),
    DiagnosticProbe(
        probe_id='05__probe_address_of_rvalue_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_address_of_rvalue_reject.c',
        note='address-of operator should reject non-lvalue operand',
    ),
    DiagnosticProbe(
        probe_id='05__probe_deref_non_pointer_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_deref_non_pointer_reject.c',
        note='dereference operator should reject non-pointer operand',
    ),
    DiagnosticProbe(
        probe_id='05__probe_sizeof_incomplete_type_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_sizeof_incomplete_type_reject.c',
        note='sizeof should reject incomplete object types',
    ),
    DiagnosticProbe(
        probe_id='05__probe_sizeof_function_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_sizeof_function_reject.c',
        note='sizeof should reject function types',
    ),
    DiagnosticProbe(
        probe_id='05__probe_logical_void_operand_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_logical_void_operand_reject.c',
        note='logical operators should reject void operands',
    ),
    DiagnosticProbe(
        probe_id='05__probe_relational_void_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_relational_void_reject.c',
        note='relational operators should reject void operands',
    ),
    DiagnosticProbe(
        probe_id='05__probe_ternary_struct_condition_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_ternary_struct_condition_reject.c',
        note='conditional operator first operand must be scalar (struct should reject)',
    ),
    DiagnosticProbe(
        probe_id='05__probe_cast_int_to_struct_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_cast_int_to_struct_reject.c',
        note='casts between scalar and non-scalar types should reject',
    ),
    DiagnosticProbe(
        probe_id='05__probe_alignof_void_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_alignof_void_reject.c',
        note='_Alignof should reject void type operand',
    ),
    DiagnosticProbe(
        probe_id='05__probe_alignof_incomplete_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_alignof_incomplete_reject.c',
        note='_Alignof should reject incomplete type operand',
    ),
    DiagnosticProbe(
        probe_id='05__probe_shift_width_large_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_shift_width_large_reject.c',
        note='left/right shift should reject widths >= bit width of promoted lhs',
    ),
    DiagnosticProbe(
        probe_id='05__probe_bitwise_float_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_bitwise_float_reject.c',
        note='bitwise operators should reject floating operands',
    ),
    DiagnosticProbe(
        probe_id='05__probe_relational_struct_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_relational_struct_reject.c',
        note='relational operators should reject struct operands',
    ),
    DiagnosticProbe(
        probe_id='05__probe_equality_struct_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_equality_struct_reject.c',
        note='equality operators should reject struct operands',
    ),
    DiagnosticProbe(
        probe_id='05__probe_add_void_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_add_void_reject.c',
        note='arithmetic operators should reject void operands',
    ),
    DiagnosticProbe(
        probe_id='05__probe_unary_bitnot_float_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_unary_bitnot_float_reject.c',
        note='unary bitwise-not should reject floating operands',
    ),
    DiagnosticProbe(
        probe_id='05__probe_unary_plus_struct_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_unary_plus_struct_reject.c',
        note='unary plus should reject struct operands',
    ),
    DiagnosticProbe(
        probe_id='05__probe_unary_minus_pointer_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_unary_minus_pointer_reject.c',
        note='unary minus should reject pointer operands',
    ),
    DiagnosticProbe(
        probe_id='05__probe_sizeof_void_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_sizeof_void_reject.c',
        note='sizeof should reject void type operand',
    ),
    DiagnosticProbe(
        probe_id='05__probe_alignof_expr_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_alignof_expr_reject.c',
        note='_Alignof should reject expression operand in strict C mode',
    ),
    DiagnosticProbe(
        probe_id='05__probe_line_directive_add_void_location_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_line_directive_add_void_location_reject.c',
        note='#line remapped virtual line should propagate into add-void expression diagnostic location',
        required_substrings=['Error at (831:'],
    ),
    DiagnosticProbe(
        probe_id='05__probe_line_directive_add_void_filename_location_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_line_directive_add_void_filename_location_reject.c',
        note='#line remapped virtual filename should propagate into add-void expression diagnostic spelling location',
        required_substrings=['Spelling: virtual_expr_add_void_filename_probe.c:832:'],
    ),
    DiagnosticProbe(
        probe_id='05__probe_line_directive_unary_minus_ptr_location_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_line_directive_unary_minus_ptr_location_reject.c',
        note='fixed baseline: multiline unary-minus-pointer lane reports remapped return-statement line under #line',
        required_substrings=['Error at (844:'],
    ),
    DiagnosticProbe(
        probe_id='05__probe_line_directive_unary_minus_ptr_reduced_location_pass',
        source=PROBE_DIR / 'diagnostics/05__probe_line_directive_unary_minus_ptr_reduced_location_pass.c',
        note='reduced threshold: single-line unary-minus-pointer lane preserves remapped #line boundary',
        required_substrings=['Error at (841:74):'],
    ),
]

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id='05__probe_diagjson_line_directive_shift_width_location_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_diagjson_line_directive_shift_width_location_reject.c',
        note='diagnostics JSON should honor #line remap for shift-width expression diagnostic',
        expected_codes=[2000],
        expected_line=851,
    ),
    DiagnosticJsonProbe(
        probe_id='05__probe_diagjson_line_directive_alignof_expr_location_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_diagjson_line_directive_alignof_expr_location_reject.c',
        note='fixed baseline: diagnostics JSON reports remapped return-statement line for _Alignof-expression diagnostic',
        expected_codes=[2000],
        expected_line=863,
    ),
    DiagnosticJsonProbe(
        probe_id='05__probe_diagjson_line_directive_alignof_expr_reduced_location_pass',
        source=PROBE_DIR / 'diagnostics/05__probe_diagjson_line_directive_alignof_expr_reduced_location_pass.c',
        note='reduced threshold: single-line _Alignof-expression lane preserves remapped #line boundary in diagnostics JSON',
        expected_codes=[2000],
        expected_line=861,
    ),
    DiagnosticJsonProbe(
        probe_id='05__probe_diagjson_line_directive_shift_width_file_presence_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_diagjson_line_directive_shift_width_file_presence_reject.c',
        note='strict frontier: diagnostics JSON should include has_file for shift-width diagnostic under #line remap',
        expected_codes=[2000],
        expected_line=871,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='05__probe_diagjson_line_directive_shift_width_current_sparse_pass',
        source=PROBE_DIR / 'diagnostics/05__probe_diagjson_line_directive_shift_width_current_sparse_pass.c',
        note='fixed baseline: shift-width diagnostics JSON under #line remap carries file presence',
        expected_codes=[2000],
        expected_line=871,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='05__probe_diagjson_line_directive_alignof_expr_file_presence_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_diagjson_line_directive_alignof_expr_file_presence_reject.c',
        note='strict frontier: diagnostics JSON should include has_file for _Alignof-expression diagnostic under #line remap',
        expected_codes=[2000],
        expected_line=881,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='05__probe_diagjson_line_directive_alignof_expr_current_sparse_pass',
        source=PROBE_DIR / 'diagnostics/05__probe_diagjson_line_directive_alignof_expr_current_sparse_pass.c',
        note='fixed baseline: _Alignof-expression diagnostics JSON under #line remap carries file presence',
        expected_codes=[2000],
        expected_line=881,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='05__probe_diagjson_line_directive_macro_add_rich_presence_strict',
        source=PROBE_DIR / 'diagnostics/05__probe_diagjson_line_directive_macro_add_rich_presence_strict.c',
        note='control lane: macro-expanded add-void diagnostics JSON carries rich location (line/column/file)',
        expected_codes=[2000],
        expected_line=901,
        expected_column=37,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='05__probe_diagjson_line_directive_include_shift_width_file_presence_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_diagjson_line_directive_include_shift_width_file_presence_reject.c',
        note='strict frontier: include-header shift-width diagnostics JSON should include has_file under #line remap',
        expected_codes=[2000],
        expected_line=1361,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='05__probe_diagjson_line_directive_include_shift_width_current_sparse_pass',
        source=PROBE_DIR / 'diagnostics/05__probe_diagjson_line_directive_include_shift_width_current_sparse_pass.c',
        note='fixed baseline: include-header shift-width diagnostics JSON carries file presence under #line remap',
        expected_codes=[2000],
        expected_line=1361,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='05__probe_diagjson_line_directive_include_alignof_expr_file_presence_reject',
        source=PROBE_DIR / 'diagnostics/05__probe_diagjson_line_directive_include_alignof_expr_file_presence_reject.c',
        note='strict frontier: include-header _Alignof-expression diagnostics JSON should include has_file under #line remap',
        expected_codes=[2000],
        expected_line=1381,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='05__probe_diagjson_line_directive_include_alignof_expr_current_sparse_pass',
        source=PROBE_DIR / 'diagnostics/05__probe_diagjson_line_directive_include_alignof_expr_current_sparse_pass.c',
        note='fixed baseline: include-header _Alignof-expression diagnostics JSON carries file presence under #line remap',
        expected_codes=[2000],
        expected_line=1381,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='05__probe_diagjson_line_directive_include_macro_add_rich_presence_strict',
        source=PROBE_DIR / 'diagnostics/05__probe_diagjson_line_directive_include_macro_add_rich_presence_strict.c',
        note='control lane: include-header macro-expanded add-void diagnostics JSON carries rich location (line/column/file)',
        expected_codes=[2000],
        expected_line=1411,
        expected_column=47,
        expected_has_file=True,
    ),
]
