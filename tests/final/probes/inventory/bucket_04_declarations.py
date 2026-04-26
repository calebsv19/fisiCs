from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='04__probe_fnptr_array_call_runtime',
        source=PROBE_DIR / 'runtime/04__probe_fnptr_array_call_runtime.c',
        note='function-pointer array call path should match clang runtime result',
    ),
    RuntimeProbe(
        probe_id='04__probe_tag_block_shadow_ok',
        source=PROBE_DIR / 'runtime/04__probe_tag_block_shadow_ok.c',
        note='inner-scope struct tag shadowing should compile/run cleanly',
    ),
    RuntimeProbe(
        probe_id='04__probe_deep_declarator_call_only',
        source=PROBE_DIR / 'runtime/04__probe_deep_declarator_call_only.c',
        note='deep declarator call-only path should compile/run cleanly',
    ),
    RuntimeProbe(
        probe_id='04__probe_deep_declarator_codegen_hang',
        source=PROBE_DIR / 'runtime/04__probe_deep_declarator_codegen_hang.c',
        note='deep declarator runtime path should compile and run (no codegen hang)',
    ),
    RuntimeProbe(
        probe_id='04__probe_deep_declarator_typedef_factory_runtime',
        source=PROBE_DIR / 'runtime/04__probe_deep_declarator_typedef_factory_runtime.c',
        note='reduced threshold: typedef-aliased factory-call function pointer lane should compile/run and match clang',
    ),
    RuntimeProbe(
        probe_id='04__probe_deep_declarator_typedef_factory_assignment_runtime',
        source=PROBE_DIR / 'runtime/04__probe_deep_declarator_typedef_factory_assignment_runtime.c',
        note='reduced threshold: typedef-aliased factory assignment lane should compile/run and match clang',
    ),
    RuntimeProbe(
        probe_id='04__probe_union_overlap_runtime',
        source=PROBE_DIR / 'runtime/04__probe_union_overlap_runtime.c',
        note='union members should overlap storage base address and satisfy size floor checks',
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id='04__probe_block_extern_initializer_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_block_extern_initializer_reject.c',
        note='block-scope extern declaration with initializer should be rejected',
    ),
    DiagnosticProbe(
        probe_id='04__probe_param_extern_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_param_extern_reject.c',
        note='parameter declaration should reject extern storage class',
    ),
    DiagnosticProbe(
        probe_id='04__probe_param_static_nonarray_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_param_static_nonarray_reject.c',
        note='parameter declaration should reject static when not used in array parameter form',
    ),
    DiagnosticProbe(
        probe_id='04__probe_param_void_named_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_param_void_named_reject.c',
        note='named parameter with type void should be rejected',
    ),
    DiagnosticProbe(
        probe_id='04__probe_enum_const_typedef_conflict_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_enum_const_typedef_conflict_reject.c',
        note='enumerator should not reuse an existing typedef name in same scope',
    ),
    DiagnosticProbe(
        probe_id='04__probe_enum_const_var_conflict_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_enum_const_var_conflict_reject.c',
        note='enumerator should not reuse an existing variable name in same scope',
    ),
    DiagnosticProbe(
        probe_id='04__probe_tag_cross_kind_conflict_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_tag_cross_kind_conflict_reject.c',
        note='struct/enum tags sharing one name in same scope should be rejected',
    ),
    DiagnosticProbe(
        probe_id='04__probe_struct_member_missing_type_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_struct_member_missing_type_reject.c',
        note='struct member declaration missing a type specifier should be rejected',
    ),
    DiagnosticProbe(
        probe_id='04__probe_bitfield_missing_colon_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_bitfield_missing_colon_reject.c',
        note="bitfield declaration missing ':' before width should be rejected",
    ),
    DiagnosticProbe(
        probe_id='04__probe_enum_missing_rbrace_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_enum_missing_rbrace_reject.c',
        note="enum body missing closing '}' should be rejected",
    ),
    DiagnosticProbe(
        probe_id='04__probe_typedef_missing_identifier_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_typedef_missing_identifier_reject.c',
        note='typedef declaration missing declarator identifier should be rejected',
    ),
    DiagnosticProbe(
        probe_id='04__probe_declarator_unbalanced_parens_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_declarator_unbalanced_parens_reject.c',
        note='declarator with unbalanced parentheses should be rejected',
    ),
    DiagnosticProbe(
        probe_id='04__probe_deep_declarator_factory_initializer_current_reject',
        source=PROBE_DIR / 'runtime/04__probe_deep_declarator_call_only.c',
        note='fixed baseline: factory() initializer path now compiles cleanly',
        required_substrings=['Semantic analysis: no issues found.'],
    ),
    DiagnosticProbe(
        probe_id='04__probe_deep_declarator_factory_assignment_current_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_deep_declarator_factory_assignment_current_reject.c',
        note='fixed baseline: factory() assignment path now compiles cleanly',
        required_substrings=['Semantic analysis: no issues found.'],
    ),
    DiagnosticProbe(
        probe_id='04__probe_complex_int_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_complex_int_reject.c',
        note='_Complex int should be rejected (complex base type must be floating)',
    ),
    DiagnosticProbe(
        probe_id='04__probe_complex_unsigned_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_complex_unsigned_reject.c',
        note='unsigned _Complex double should be rejected',
    ),
    DiagnosticProbe(
        probe_id='04__probe_complex_missing_base_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_complex_missing_base_reject.c',
        note='_Complex without floating base type should be rejected',
    ),
]

DIAG_JSON_PROBES = []
