from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id='14__probe_diag_fnptr_table_incompatible_assign_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_fnptr_table_incompatible_assign_reject.c',
        note='runtime fnptr table lane should reject too-few-arguments call through function pointer',
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_fnptr_table_too_many_args_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_fnptr_table_too_many_args_reject.c',
        note='runtime fnptr table lane should reject too-many-arguments call through function pointer',
        required_substrings=['Too many arguments'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_fnptr_table_incompatible_signature_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_fnptr_table_incompatible_signature_reject.c',
        note='runtime fnptr table lane should reject incompatible function-pointer initializer signatures',
        required_substrings=['Incompatible initializer type'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_fnptr_struct_arg_incompatible_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_fnptr_struct_arg_incompatible_reject.c',
        note='runtime fnptr lane should reject struct argument passed where function pointer is required',
        required_substrings=['has incompatible type'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_fnptr_param_noncallable_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_fnptr_param_noncallable_reject.c',
        note='runtime fnptr lane should reject non-callable argument passed as function-pointer parameter',
        required_substrings=['has incompatible type'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_offsetof_bitfield_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_offsetof_bitfield_reject.c',
        note='layout lane should reject offsetof on bitfield member',
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_vla_non_integer_bound_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_vla_non_integer_bound_reject.c',
        note='layout lane should reject file-scope VLA with non-constant bound',
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_ternary_struct_condition_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_ternary_struct_condition_reject.c',
        note='control lane should reject ternary struct condition operand',
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_switch_struct_condition_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_switch_struct_condition_reject.c',
        note='control lane should reject switch with non-integer struct condition',
        required_substrings=['switch controlling expression must be integer'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_if_struct_condition_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_if_struct_condition_reject.c',
        note='control lane should reject if with non-scalar struct condition',
        required_substrings=['if controlling expression must be scalar'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_while_struct_condition_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_while_struct_condition_reject.c',
        note='control lane should reject while with non-scalar struct condition',
        required_substrings=['while controlling expression must be scalar'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_return_struct_to_int_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_return_struct_to_int_reject.c',
        note='semantic lane should reject returning struct from int function',
        required_substrings=['Incompatible return type'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_continue_outside_loop_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_continue_outside_loop_reject.c',
        note='control lane should reject continue outside iterative statements',
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_break_outside_loop_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_break_outside_loop_reject.c',
        note='control lane should reject break outside loop/switch statements',
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_case_outside_switch_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_case_outside_switch_reject.c',
        note='control lane should reject case labels outside switch statements',
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_default_outside_switch_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_default_outside_switch_reject.c',
        note='control lane should reject default labels outside switch statements',
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_switch_duplicate_default_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_switch_duplicate_default_reject.c',
        note='control lane should reject duplicate default labels in switch statements',
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_switch_duplicate_case_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_switch_duplicate_case_reject.c',
        note='control lane should reject duplicate case labels in switch statements',
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_switch_nonconst_case_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_switch_nonconst_case_reject.c',
        note='control lane should reject non-constant case labels',
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_continue_in_switch_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_continue_in_switch_reject.c',
        note='control lane should reject continue inside switch without an enclosing loop',
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_complex_lt_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_complex_lt_reject.c',
        note="complex relational '<' should be rejected with a semantic diagnostic",
        required_substrings=['real (non-complex) comparable operands'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_complex_le_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_complex_le_reject.c',
        note="complex relational '<=' should be rejected with a semantic diagnostic",
        required_substrings=['real (non-complex) comparable operands'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_complex_gt_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_complex_gt_reject.c',
        note="complex relational '>' should be rejected with a semantic diagnostic",
        required_substrings=['real (non-complex) comparable operands'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_complex_ge_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_complex_ge_reject.c',
        note="complex relational '>=' should be rejected with a semantic diagnostic",
        required_substrings=['real (non-complex) comparable operands'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_multitu_duplicate_external_definition_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_multitu_duplicate_external_definition_reject_main.c',
        note='multi-TU lane should reject duplicate external definitions at link stage',
        required_substrings=['duplicate symbol'],
        inputs=[PROBE_DIR / 'diagnostics/14__probe_diag_multitu_duplicate_external_definition_reject_main.c', PROBE_DIR / 'diagnostics/14__probe_diag_multitu_duplicate_external_definition_reject_lib.c'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_multitu_extern_type_mismatch_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_multitu_extern_type_mismatch_reject_main.c',
        note='multi-TU lane should reject conflicting extern types that collide at link stage',
        required_substrings=['linker command failed'],
        inputs=[PROBE_DIR / 'diagnostics/14__probe_diag_multitu_extern_type_mismatch_reject_main.c', PROBE_DIR / 'diagnostics/14__probe_diag_multitu_extern_type_mismatch_reject_lib.c'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_multitu_duplicate_tentative_type_conflict_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_multitu_duplicate_tentative_type_conflict_reject_main.c',
        note='multi-TU lane should reject conflicting external definitions with mismatched types',
        required_substrings=['duplicate symbol'],
        inputs=[PROBE_DIR / 'diagnostics/14__probe_diag_multitu_duplicate_tentative_type_conflict_reject_main.c', PROBE_DIR / 'diagnostics/14__probe_diag_multitu_duplicate_tentative_type_conflict_reject_lib.c'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_multitu_duplicate_function_definition_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_multitu_duplicate_function_definition_reject_main.c',
        note='multi-TU lane should reject duplicate function definitions at link stage (text diagnostics only)',
        required_substrings=['duplicate symbol'],
        inputs=[PROBE_DIR / 'diagnostics/14__probe_diag_multitu_duplicate_function_definition_reject_main.c', PROBE_DIR / 'diagnostics/14__probe_diag_multitu_duplicate_function_definition_reject_lib.c'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_axis2_wave1_line_map_runtime_guard_conflict_bridge',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_axis2_wave1_line_map_runtime_guard_conflict_bridge.c',
        note='axis2 wave1: #line remap through nested include+macro guard conflict should preserve virtual spelling location',
        required_substrings=['axis2_wave1_guard_layer_a.h'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_axis2_wave2_line_map_nested_include_macro_guard_reentry_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_axis2_wave2_line_map_nested_include_macro_guard_reentry_reject.c',
        note='axis2 wave2: #line remap through nested include+macro guard reentry should preserve deep virtual spelling location',
        required_substrings=['axis2_wave2_guard_layer_c.h'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_axis2_wave3_line_map_nested_include_macro_type_bridge_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_axis2_wave3_line_map_nested_include_macro_type_bridge_reject.c',
        note='axis2 wave3: #line remap through four-layer include+macro type-bridge lane should preserve deep virtual spelling location',
        required_substrings=['axis2_wave3_guard_layer_d.h'],
    ),
    DiagnosticProbe(
        probe_id='14__probe_diag_axis2_wave4_line_map_nested_include_macro_arity_overflow_reject',
        source=PROBE_DIR / 'diagnostics/14__probe_diag_axis2_wave4_line_map_nested_include_macro_arity_overflow_reject.c',
        note='axis2 wave4: #line remap through five-layer include+macro arity overflow lane should preserve deep virtual spelling location',
        required_substrings=['axis2_wave4_arity_layer_e.h'],
    ),
]

DIAG_JSON_PROBES = []
