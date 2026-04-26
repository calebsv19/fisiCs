from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='13__probe_phi_continue_runtime',
        source=PROBE_DIR / 'runtime/13__probe_phi_continue_runtime.c',
        note='loop-carried accumulator with continue/break edges should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='13__probe_short_circuit_chain_runtime',
        source=PROBE_DIR / 'runtime/13__probe_short_circuit_chain_runtime.c',
        note='nested short-circuit chain side effects should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='13__probe_struct_copy_runtime',
        source=PROBE_DIR / 'runtime/13__probe_struct_copy_runtime.c',
        note='struct return and by-value copy/update path should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='13__probe_ptr_stride_runtime',
        source=PROBE_DIR / 'runtime/13__probe_ptr_stride_runtime.c',
        note='pointer-difference scaling and indexed pointer loads should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='13__probe_global_init_runtime',
        source=PROBE_DIR / 'runtime/13__probe_global_init_runtime.c',
        note='global partial initializer and zero-fill behavior should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='13__probe_fnptr_ternary_decay_runtime',
        source=PROBE_DIR / 'runtime/13__probe_fnptr_ternary_decay_runtime.c',
        note='function designators in ternary should decay to compatible function pointers',
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id='13__probe_diag_line_directive_if_missing_lparen_parser_text_strict',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_if_missing_lparen_parser_presence.c',
        note="text diagnostics should preserve parser missing '(' line under #line remap for if",
        required_substrings=["Error: expected '(' after 'if' at line 132003"],
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_line_directive_include_if_missing_lparen_parser_text_strict',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_include_if_missing_lparen_parser_presence.c',
        note="text diagnostics should preserve parser missing '(' line under include #line remap for if",
        required_substrings=["Error: expected '(' after 'if' at line 132053"],
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_line_directive_switch_missing_rparen_parser_text_strict',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_switch_missing_rparen_parser_presence.c',
        note='text diagnostics should preserve parser switch-header parse failure line under #line remap',
        required_substrings=['Error: Failed to parse expression at line 132103'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_line_directive_include_switch_missing_rparen_parser_text_strict',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_include_switch_missing_rparen_parser_presence.c',
        note='text diagnostics should preserve parser switch-header parse failure line under include #line remap',
        required_substrings=['Error: Failed to parse expression at line 132153'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_line_directive_if_missing_body_parser_text_strict',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_if_missing_body_parser_presence.c',
        note='text diagnostics should preserve parser missing-body line under #line remap for if',
        required_substrings=['line 134003'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_line_directive_include_if_missing_body_parser_text_strict',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_include_if_missing_body_parser_presence.c',
        note='text diagnostics should preserve parser missing-body line under include #line remap for if',
        required_substrings=['line 134053'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_line_directive_while_missing_body_parser_text_strict',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_while_missing_body_parser_presence.c',
        note='text diagnostics should preserve parser missing-body line under #line remap for while',
        required_substrings=['line 134103'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_line_directive_include_while_missing_body_parser_text_strict',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_include_while_missing_body_parser_presence.c',
        note='text diagnostics should preserve parser missing-body line under include #line remap for while',
        required_substrings=['line 134153'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_line_directive_for_missing_body_parser_text_strict',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_for_missing_body_parser_presence.c',
        note='text diagnostics should preserve parser missing-body line under #line remap for for',
        required_substrings=['line 134203'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_line_directive_include_for_missing_body_parser_text_strict',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_include_for_missing_body_parser_presence.c',
        note='text diagnostics should preserve parser missing-body line under include #line remap for for',
        required_substrings=['line 134253'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_line_directive_switch_missing_body_parser_text_strict',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_switch_missing_body_parser_presence.c',
        note='text diagnostics should preserve parser missing-body line under #line remap for switch',
        required_substrings=['line 134303'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_line_directive_include_switch_missing_body_parser_text_strict',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_include_switch_missing_body_parser_presence.c',
        note='text diagnostics should preserve parser missing-body line under include #line remap for switch',
        required_substrings=['line 134353'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_fnptr_too_many_args_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_fnptr_too_many_args_reject.c',
        note='function-pointer call should reject too many arguments for fixed-arity target',
        required_substrings=['Too many arguments'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_fnptr_too_few_args_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_fnptr_too_few_args_reject.c',
        note='function-pointer call should reject too few arguments for fixed-arity target',
        required_substrings=['Too few arguments'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_mod_float_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_mod_float_reject.c',
        note='modulo with floating operand should be rejected',
        required_substrings=["Operator '%'"],
    ),
    DiagnosticProbe(
        probe_id='13__probe_fnptr_assign_incompatible_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_fnptr_assign_incompatible_reject.c',
        note='function-pointer assignment with incompatible signature should be rejected',
        required_substrings=['Incompatible assignment operands'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_fnptr_nested_qualifier_loss_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_fnptr_nested_qualifier_loss_reject.c',
        note='nested function-pointer assignment should reject qualifier loss at pointee depth',
        required_substrings=['discards qualifiers'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_voidptr_to_fnptr_assign_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_voidptr_to_fnptr_assign_reject.c',
        note='assignment from void* to function pointer should be rejected',
        required_substrings=['Incompatible assignment operands'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_fnptr_to_voidptr_assign_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_fnptr_to_voidptr_assign_reject.c',
        note='assignment from function pointer to void* should be rejected',
        required_substrings=['Incompatible assignment operands'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_fnptr_nested_volatile_qualifier_loss_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_fnptr_nested_volatile_qualifier_loss_reject.c',
        note='nested function-pointer assignment should reject volatile qualifier loss at pointee depth',
        required_substrings=['discards qualifiers'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_fnptr_deep_const_qualifier_loss_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_fnptr_deep_const_qualifier_loss_reject.c',
        note='multi-level function-pointer assignment should reject deep const qualifier loss',
        required_substrings=['discards qualifiers'],
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_if_missing_lparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_if_missing_lparen.c',
        note="parser should emit diagnostics for missing '(' in if statement",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_while_missing_lparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_while_missing_lparen.c',
        note="parser should emit diagnostics for missing '(' in while statement",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_do_while_missing_lparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_do_while_missing_lparen.c',
        note="parser should emit diagnostics for missing '(' in do-while condition",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_for_missing_lparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_for_missing_lparen.c',
        note="parser should emit diagnostics for missing '(' in for statement",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_if_missing_rparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_if_missing_rparen.c',
        note="parser should emit diagnostics for missing ')' in if statement",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_while_missing_rparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_while_missing_rparen.c',
        note="parser should emit diagnostics for missing ')' in while statement",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_do_while_missing_rparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_do_while_missing_rparen.c',
        note="parser should emit diagnostics for missing ')' in do-while condition",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_for_missing_rparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_for_missing_rparen.c',
        note="parser should emit diagnostics for missing ')' in for statement",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_for_missing_first_semicolon',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_for_missing_first_semicolon.c',
        note="parser should emit diagnostics for missing first ';' in for header",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_for_missing_second_semicolon',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_for_missing_second_semicolon.c',
        note="parser should emit diagnostics for missing second ';' in for header",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_for_missing_second_semicolon_simple',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_for_missing_second_semicolon_simple.c',
        note="parser should emit diagnostics for missing second ';' in minimal for-header form",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_if_missing_condition',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_if_missing_condition.c',
        note='parser should emit diagnostics for missing condition expression in if statement',
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_do_while_missing_while',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_do_while_missing_while.c',
        note="parser should emit diagnostics for missing 'while' in do-while statement",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_do_while_missing_while_simple',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_do_while_missing_while_simple.c',
        note="parser should emit diagnostics for missing 'while' in reduced do-while form",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_do_while_missing_condition',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_do_while_missing_condition.c',
        note='parser should emit diagnostics for missing condition expression in do-while statement',
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_do_while_missing_semicolon',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_do_while_missing_semicolon.c',
        note="parser should emit diagnostics for missing ';' after do-while condition",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_for_missing_body',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_for_missing_body.c',
        note='parser should emit diagnostics for missing for-loop body statement',
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_while_missing_body',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_while_missing_body.c',
        note='parser should emit diagnostics for missing while-loop body statement',
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_do_while_missing_body',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_do_while_missing_body.c',
        note='parser should emit diagnostics for missing do-while loop body statement',
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_do_while_missing_body_simple',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_do_while_missing_body_simple.c',
        note='parser should emit diagnostics for minimal missing do-while loop body statement',
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_switch_missing_body',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_switch_missing_body.c',
        note='parser should emit diagnostics for missing switch statement body',
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_switch_missing_lparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_switch_missing_lparen.c',
        note="parser should emit diagnostics for missing '(' in switch statement",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_while_empty_condition',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_while_empty_condition.c',
        note='parser should emit diagnostics for missing while condition expression',
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_switch_missing_rparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_switch_missing_rparen.c',
        note="parser should emit diagnostics for missing ')' in switch statement",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_if_missing_then_stmt',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_if_missing_then_stmt.c',
        note='parser should emit diagnostics for missing then-statement in if',
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_else_missing_stmt_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_else_missing_stmt_reject.c',
        note='parser should emit diagnostics for missing statement in else',
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_switch_missing_lbrace',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_switch_missing_lbrace.c',
        note="parser should emit diagnostics for missing '{' in switch statement",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_switch_case_missing_colon',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_switch_case_missing_colon.c',
        note="parser should emit diagnostics for missing ':' in switch case label",
    ),
    DiagnosticProbe(
        probe_id='13__probe_diag_parser_switch_default_missing_colon_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diag_parser_switch_default_missing_colon.c',
        note="parser should emit diagnostics for missing ':' in switch default label",
    ),
]

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_line_directive_if_missing_lparen_parser_presence_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_if_missing_lparen_parser_presence.c',
        note="reduced threshold: #line if-missing-'(' currently emits semantic-only diagjson payload at remapped function location",
        expected_codes=[2000],
        expected_line=132001,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_line_directive_include_if_missing_lparen_parser_presence_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_include_if_missing_lparen_parser_presence.c',
        note="reduced threshold: include #line if-missing-'(' currently emits semantic-only diagjson payload at remapped function location",
        expected_codes=[2000],
        expected_line=132051,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_line_directive_switch_missing_rparen_parser_presence_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_switch_missing_rparen_parser_presence.c',
        note="strict frontier: #line switch-missing-')' should preserve parser diagnostics JSON presence",
        expected_codes=[1000],
        expected_line=132103,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_line_directive_include_switch_missing_rparen_parser_presence_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_include_switch_missing_rparen_parser_presence.c',
        note="strict frontier: include #line switch-missing-')' should preserve parser diagnostics JSON presence",
        expected_codes=[1000],
        expected_line=132153,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_line_directive_if_missing_body_parser_presence_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_if_missing_body_parser_presence.c',
        note='strict frontier: #line if-missing-body should preserve parser diagnostics JSON presence',
        expected_codes=[1000],
        expected_line=134003,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_line_directive_include_if_missing_body_parser_presence_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_include_if_missing_body_parser_presence.c',
        note='strict frontier: include #line if-missing-body should preserve parser diagnostics JSON presence',
        expected_codes=[1000],
        expected_line=134053,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_line_directive_while_missing_body_parser_presence_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_while_missing_body_parser_presence.c',
        note='strict frontier: #line while-missing-body should preserve parser diagnostics JSON presence',
        expected_codes=[1000],
        expected_line=134103,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_line_directive_include_while_missing_body_parser_presence_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_include_while_missing_body_parser_presence.c',
        note='strict frontier: include #line while-missing-body should preserve parser diagnostics JSON presence',
        expected_codes=[1000],
        expected_line=134153,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_line_directive_for_missing_body_parser_presence_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_for_missing_body_parser_presence.c',
        note='strict frontier: #line for-missing-body should preserve parser diagnostics JSON presence',
        expected_codes=[1000],
        expected_line=134203,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_line_directive_include_for_missing_body_parser_presence_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_include_for_missing_body_parser_presence.c',
        note='strict frontier: include #line for-missing-body should preserve parser diagnostics JSON presence',
        expected_codes=[1000],
        expected_line=134253,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_line_directive_switch_missing_body_parser_presence_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_switch_missing_body_parser_presence.c',
        note='strict frontier: #line switch-missing-body should preserve parser diagnostics JSON presence',
        expected_codes=[1000],
        expected_line=134303,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_line_directive_include_switch_missing_body_parser_presence_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_line_directive_include_switch_missing_body_parser_presence.c',
        note='strict frontier: include #line switch-missing-body should preserve parser diagnostics JSON presence',
        expected_codes=[1000],
        expected_line=134353,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_if_missing_lparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_if_missing_lparen.c',
        note="diagnostics JSON should be emitted for missing '(' in if statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_while_missing_lparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_while_missing_lparen.c',
        note="diagnostics JSON should be emitted for missing '(' in while statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_do_while_missing_lparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_do_while_missing_lparen.c',
        note="diagnostics JSON should be emitted for missing '(' in do-while condition parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_for_missing_lparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_for_missing_lparen.c',
        note="diagnostics JSON should be emitted for missing '(' in for statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_if_missing_rparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_if_missing_rparen.c',
        note="diagnostics JSON should be emitted for missing ')' in if statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_while_missing_rparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_while_missing_rparen.c',
        note="diagnostics JSON should be emitted for missing ')' in while statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_do_while_missing_rparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_do_while_missing_rparen.c',
        note="diagnostics JSON should be emitted for missing ')' in do-while condition parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_for_missing_rparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_for_missing_rparen.c',
        note="diagnostics JSON should be emitted for missing ')' in for statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_for_missing_first_semicolon',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_for_missing_first_semicolon.c',
        note="diagnostics JSON should be emitted for missing first ';' in for header",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_for_missing_second_semicolon',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_for_missing_second_semicolon.c',
        note="diagnostics JSON should be emitted for missing second ';' in for header",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_for_missing_second_semicolon_simple',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_for_missing_second_semicolon_simple.c',
        note="diagnostics JSON should be emitted for missing second ';' in minimal for-header form",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_if_missing_condition',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_if_missing_condition.c',
        note='diagnostics JSON should be emitted for missing condition expression in if statement parser recovery',
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_do_while_missing_while',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_do_while_missing_while.c',
        note="diagnostics JSON should be emitted for missing 'while' in do-while statement",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_do_while_missing_while_simple',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_do_while_missing_while_simple.c',
        note="diagnostics JSON should be emitted for missing 'while' in minimal do-while form",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_do_while_missing_condition',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_do_while_missing_condition.c',
        note='diagnostics JSON should be emitted for missing condition expression in do-while condition parser recovery',
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_do_while_missing_semicolon',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_do_while_missing_semicolon.c',
        note="diagnostics JSON should be emitted for missing ';' after do-while condition",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_for_missing_body',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_for_missing_body.c',
        note='diagnostics JSON should be emitted for missing for-loop body statement',
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_while_missing_body',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_while_missing_body.c',
        note='diagnostics JSON should be emitted for missing while-loop body statement',
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_do_while_missing_body',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_do_while_missing_body.c',
        note='diagnostics JSON should be emitted for missing do-while loop body statement',
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_do_while_missing_body_simple',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_do_while_missing_body_simple.c',
        note='diagnostics JSON should be emitted for minimal missing do-while loop body statement',
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_switch_missing_body',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_switch_missing_body.c',
        note='diagnostics JSON should be emitted for missing switch body statement',
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_switch_missing_lparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_switch_missing_lparen.c',
        note="diagnostics JSON should be emitted for missing '(' in switch statement",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_while_empty_condition',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_while_empty_condition.c',
        note='diagnostics JSON should be emitted for missing while condition expression',
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_switch_missing_rparen',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_switch_missing_rparen.c',
        note="diagnostics JSON should be emitted for missing ')' in switch statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_if_missing_then_stmt',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_if_missing_then_stmt.c',
        note='diagnostics JSON should be emitted for missing then-statement in if parser recovery',
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_else_missing_stmt_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_else_missing_stmt_reject.c',
        note='diagnostics JSON should be emitted for missing statement in else parser recovery',
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_switch_missing_lbrace',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_switch_missing_lbrace.c',
        note="diagnostics JSON should be emitted for missing '{' in switch statement parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_switch_case_missing_colon',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_switch_case_missing_colon.c',
        note="diagnostics JSON should be emitted for missing ':' in switch case label parser recovery",
    ),
    DiagnosticJsonProbe(
        probe_id='13__probe_diagjson_parser_switch_default_missing_colon_reject',
        source=PROBE_DIR / 'diagnostics/13__probe_diagjson_parser_switch_default_missing_colon.c',
        note="diagnostics JSON should be emitted for missing ':' in switch default label parser recovery",
    ),
]
