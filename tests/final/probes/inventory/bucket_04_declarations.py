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
    RuntimeProbe(
        probe_id='04__probe_tag_typedef_parallel_namespace_ok',
        source=PROBE_DIR / 'runtime/04__probe_tag_typedef_parallel_namespace_ok.c',
        note='tag namespace and typedef namespace should coexist when names match',
    ),
    RuntimeProbe(
        probe_id='04__probe_tag_object_parallel_namespace_ok',
        source=PROBE_DIR / 'runtime/04__probe_tag_object_parallel_namespace_ok.c',
        note='tag namespace and ordinary identifier namespace should coexist when names match',
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_typedef_fnptr_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_typedef_fnptr_bridge_main.c',
        note='multi-TU typedef-backed function-pointer declarations should stay compatible on the clean path',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_typedef_fnptr_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_typedef_fnptr_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_pointer_array_compat_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_pointer_array_compat_bridge_main.c',
        note='multi-TU pointer-to-array declarations with matching extents should compile and run cleanly',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_pointer_array_compat_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_pointer_array_compat_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_tag_function_parallel_namespace',
        source=PROBE_DIR / 'runtime/04__probe_multitu_tag_function_parallel_namespace_main.c',
        note='struct tags and ordinary function identifiers should coexist across multi-TU include boundaries',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_tag_function_parallel_namespace_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_tag_function_parallel_namespace_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_include_tag_typedef_parallel_guard',
        source=PROBE_DIR / 'runtime/04__probe_multitu_include_tag_typedef_parallel_guard_main.c',
        note='include-shared tag and typedef namespaces should remain compatible across multiple translation units',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_include_tag_typedef_parallel_guard_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_include_tag_typedef_parallel_guard_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_typedef_object_alias_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_typedef_object_alias_bridge_main.c',
        note='multi-TU typedef-backed object aliases should stay compatible on the clean path',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_typedef_object_alias_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_typedef_object_alias_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_include_return_alias_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_include_return_alias_bridge_main.c',
        note='include-shared typedef return aliases should remain compatible across multiple translation units',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_include_return_alias_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_include_return_alias_bridge_lib.c',
        ],
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
        expect_any_diagnostic=False,
        required_substrings=['Semantic analysis: no issues found.'],
    ),
    DiagnosticProbe(
        probe_id='04__probe_deep_declarator_factory_assignment_current_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_deep_declarator_factory_assignment_current_reject.c',
        note='fixed baseline: factory() assignment path now compiles cleanly',
        expect_any_diagnostic=False,
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
    DiagnosticProbe(
        probe_id='04__probe_storage_typedef_static_conflict_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_storage_typedef_static_conflict_reject.c',
        note='typedef combined with static in one declaration should be rejected',
    ),
    DiagnosticProbe(
        probe_id='04__probe_storage_typedef_extern_conflict_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_storage_typedef_extern_conflict_reject.c',
        note='typedef combined with extern in one declaration should be rejected',
    ),
    DiagnosticProbe(
        probe_id='04__probe_tag_struct_union_conflict_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_tag_struct_union_conflict_reject.c',
        note='struct and union tags sharing a name in one scope should be rejected',
    ),
    DiagnosticProbe(
        probe_id='04__probe_tag_union_enum_conflict_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_tag_union_enum_conflict_reject.c',
        note='union and enum tags sharing a name in one scope should be rejected',
    ),
    DiagnosticProbe(
        probe_id='04__probe_redecl_array_extent_conflict_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_redecl_array_extent_conflict_reject.c',
        note='same-scope array redeclaration with conflicting extents should be rejected',
    ),
    DiagnosticProbe(
        probe_id='04__probe_redecl_function_param_conflict_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_redecl_function_param_conflict_reject.c',
        note='same-scope function redeclaration with incompatible parameter types should be rejected',
    ),
    DiagnosticProbe(
        probe_id='04__probe_redecl_object_function_conflict_reject',
        source=PROBE_DIR / 'diagnostics/04__probe_redecl_object_function_conflict_reject.c',
        note='same-scope object and function declarations sharing one name should be rejected',
    ),
    DiagnosticProbe(
        probe_id='04__probe_diag_line_directive_array_extent_conflict_spelling_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_array_extent_conflict_spelling_strict.c',
        note='text diagnostics should preserve remapped spelling for array-extent conflicts under #line',
        required_substrings=['Spelling: virtual_decl_array_extent_conflict_probe_diag_text.c:11002:5'],
    ),
    DiagnosticProbe(
        probe_id='04__probe_diag_line_directive_include_array_extent_conflict_spelling_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_include_array_extent_conflict_spelling_strict.c',
        note='text diagnostics should preserve remapped spelling for include-header array-extent conflicts under #line',
        required_substrings=['Spelling: virtual_decl_include_array_extent_conflict_probe_diag_text.h:11102:5'],
    ),
    DiagnosticProbe(
        probe_id='04__probe_diag_line_directive_tag_cross_kind_conflict_spelling_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_tag_cross_kind_conflict_spelling_strict.c',
        note='text diagnostics should preserve remapped spelling for tag cross-kind conflicts under #line',
        required_substrings=['Spelling: virtual_decl_tag_cross_kind_probe_diag_text.c:11202:7'],
    ),
    DiagnosticProbe(
        probe_id='04__probe_diag_line_directive_include_tag_cross_kind_conflict_spelling_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_include_tag_cross_kind_conflict_spelling_strict.c',
        note='text diagnostics should preserve remapped spelling for include-header tag cross-kind conflicts under #line',
        required_substrings=['Spelling: virtual_decl_include_tag_cross_kind_probe_diag_text.h:11302:6'],
    ),
    DiagnosticProbe(
        probe_id='04__probe_diag_line_directive_multitu_fnptr_param_conflict_spelling_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_fnptr_param_conflict_spelling_strict_main.c',
        note='multi-TU function-pointer parameter drift should preserve remapped spelling under #line',
        required_substrings=[
            'Conflicting types for function',
            'virtual_decl_multitu_fnptr_param_conflict_probe_diag_text_lib.c:16002',
        ],
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_fnptr_param_conflict_spelling_strict_main.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_fnptr_param_conflict_spelling_strict_lib.c',
        ],
    ),
    DiagnosticProbe(
        probe_id='04__probe_diag_line_directive_multitu_include_fnptr_param_conflict_spelling_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_fnptr_param_conflict_spelling_strict_main.c',
        note='multi-TU include-boundary function-pointer parameter drift should preserve remapped spelling under #line',
        required_substrings=[
            'Conflicting types for function',
            'virtual_decl_multitu_include_fnptr_param_conflict_probe_diag_text_lib.c:16102',
        ],
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_fnptr_param_conflict_spelling_strict_main.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_fnptr_param_conflict_spelling_strict_lib.c',
        ],
    ),
    DiagnosticProbe(
        probe_id='04__probe_diag_line_directive_multitu_ptr_array_conflict_spelling_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_ptr_array_conflict_spelling_strict_main.c',
        note='multi-TU pointer-to-array declarator drift should preserve remapped spelling under #line',
        required_substrings=[
            'Conflicting types for variable',
            'virtual_decl_multitu_ptr_array_conflict_probe_diag_text_lib.c:16202',
        ],
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_ptr_array_conflict_spelling_strict_main.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_ptr_array_conflict_spelling_strict_lib.c',
        ],
    ),
    DiagnosticProbe(
        probe_id='04__probe_diag_line_directive_multitu_typedef_object_conflict_spelling_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_typedef_object_conflict_spelling_strict_main.c',
        note='multi-TU typedef-backed object drift should preserve remapped spelling under #line',
        required_substrings=[
            'Conflicting types for variable',
            'virtual_decl_multitu_typedef_object_conflict_probe_diag_text_lib.c:17003',
        ],
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_typedef_object_conflict_spelling_strict_main.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_typedef_object_conflict_spelling_strict_lib.c',
        ],
    ),
    DiagnosticProbe(
        probe_id='04__probe_diag_line_directive_multitu_typedef_return_conflict_spelling_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_typedef_return_conflict_spelling_strict_main.c',
        note='multi-TU typedef-backed function return drift should preserve remapped spelling under #line',
        required_substrings=[
            'Conflicting types for function',
            'virtual_decl_multitu_typedef_return_conflict_probe_diag_text_lib.c:17103',
        ],
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_typedef_return_conflict_spelling_strict_main.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_typedef_return_conflict_spelling_strict_lib.c',
        ],
    ),
    DiagnosticProbe(
        probe_id='04__probe_diag_line_directive_multitu_include_object_qualifier_conflict_spelling_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_object_qualifier_conflict_spelling_strict_main.c',
        note='multi-TU include-boundary object qualifier drift should preserve remapped spelling under #line',
        required_substrings=[
            'Conflicting types for variable',
            'virtual_decl_multitu_include_object_qualifier_conflict_probe_diag_text_lib.c:17202',
        ],
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_object_qualifier_conflict_spelling_strict_main.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_object_qualifier_conflict_spelling_strict_lib.c',
        ],
    ),
    DiagnosticProbe(
        probe_id='04__probe_diag_line_directive_multitu_include_return_ptr_conflict_spelling_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_return_ptr_conflict_spelling_strict_main.c',
        note='multi-TU include-boundary function return pointer drift should preserve remapped spelling under #line',
        required_substrings=[
            'Conflicting types for function',
            'virtual_decl_multitu_include_return_ptr_conflict_probe_diag_text_lib.c:17302',
        ],
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_return_ptr_conflict_spelling_strict_main.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_return_ptr_conflict_spelling_strict_lib.c',
        ],
    ),
]

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id='04__probe_diagjson_line_directive_array_extent_conflict_rich_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diagjson_line_directive_array_extent_conflict_rich_strict.c',
        note='diagnostics JSON should carry remapped line/column/file for array-extent conflicts under #line',
        expected_codes=[2000],
        expected_line=11002,
        expected_column=5,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='04__probe_diagjson_line_directive_include_array_extent_conflict_rich_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diagjson_line_directive_include_array_extent_conflict_rich_strict.c',
        note='diagnostics JSON should carry remapped line/column/file for include-header array-extent conflicts under #line',
        expected_codes=[2000],
        expected_line=11102,
        expected_column=5,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='04__probe_diagjson_line_directive_tag_cross_kind_conflict_rich_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diagjson_line_directive_tag_cross_kind_conflict_rich_strict.c',
        note='diagnostics JSON should carry remapped line/column/file for tag cross-kind conflicts under #line',
        expected_codes=[2000],
        expected_line=11202,
        expected_column=7,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='04__probe_diagjson_line_directive_include_tag_cross_kind_conflict_rich_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diagjson_line_directive_include_tag_cross_kind_conflict_rich_strict.c',
        note='diagnostics JSON should carry remapped line/column/file for include-header tag cross-kind conflicts under #line',
        expected_codes=[2000],
        expected_line=11302,
        expected_column=6,
        expected_has_file=True,
    ),
    DiagnosticJsonProbe(
        probe_id='04__probe_diagjson_line_directive_multitu_fnptr_param_conflict_rich_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_fnptr_param_conflict_spelling_strict_main.c',
        note='multi-TU function-pointer parameter drift diagnostics JSON should preserve remapped line/file under #line',
        expected_codes=[2000],
        expected_line=16002,
        expected_has_file=True,
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_fnptr_param_conflict_spelling_strict_main.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_fnptr_param_conflict_spelling_strict_lib.c',
        ],
    ),
    DiagnosticJsonProbe(
        probe_id='04__probe_diagjson_line_directive_multitu_include_fnptr_param_conflict_rich_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_fnptr_param_conflict_spelling_strict_main.c',
        note='multi-TU include-boundary function-pointer parameter drift diagnostics JSON should preserve remapped line/file under #line',
        expected_codes=[2000],
        expected_line=16102,
        expected_has_file=True,
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_fnptr_param_conflict_spelling_strict_main.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_fnptr_param_conflict_spelling_strict_lib.c',
        ],
    ),
    DiagnosticJsonProbe(
        probe_id='04__probe_diagjson_line_directive_multitu_ptr_array_conflict_rich_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_ptr_array_conflict_spelling_strict_main.c',
        note='multi-TU pointer-to-array declarator drift diagnostics JSON should preserve remapped line/file under #line',
        expected_codes=[2000],
        expected_line=16202,
        expected_has_file=True,
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_ptr_array_conflict_spelling_strict_main.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_ptr_array_conflict_spelling_strict_lib.c',
        ],
    ),
    DiagnosticJsonProbe(
        probe_id='04__probe_diagjson_line_directive_multitu_typedef_object_conflict_rich_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_typedef_object_conflict_spelling_strict_main.c',
        note='multi-TU typedef-backed object drift diagnostics JSON should preserve remapped line/file under #line',
        expected_codes=[2000],
        expected_line=17003,
        expected_has_file=True,
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_typedef_object_conflict_spelling_strict_main.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_typedef_object_conflict_spelling_strict_lib.c',
        ],
    ),
    DiagnosticJsonProbe(
        probe_id='04__probe_diagjson_line_directive_multitu_typedef_return_conflict_rich_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_typedef_return_conflict_spelling_strict_main.c',
        note='multi-TU typedef-backed function return drift diagnostics JSON should preserve remapped line/file under #line',
        expected_codes=[2000],
        expected_line=17103,
        expected_has_file=True,
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_typedef_return_conflict_spelling_strict_main.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_typedef_return_conflict_spelling_strict_lib.c',
        ],
    ),
    DiagnosticJsonProbe(
        probe_id='04__probe_diagjson_line_directive_multitu_include_object_qualifier_conflict_rich_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_object_qualifier_conflict_spelling_strict_main.c',
        note='multi-TU include-boundary object qualifier drift diagnostics JSON should preserve remapped line/file under #line',
        expected_codes=[2000],
        expected_line=17202,
        expected_has_file=True,
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_object_qualifier_conflict_spelling_strict_main.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_object_qualifier_conflict_spelling_strict_lib.c',
        ],
    ),
    DiagnosticJsonProbe(
        probe_id='04__probe_diagjson_line_directive_multitu_include_return_ptr_conflict_rich_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_return_ptr_conflict_spelling_strict_main.c',
        note='multi-TU include-boundary return-pointer drift diagnostics JSON should preserve remapped line/file under #line',
        expected_codes=[2000],
        expected_line=17302,
        expected_has_file=True,
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_return_ptr_conflict_spelling_strict_main.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_return_ptr_conflict_spelling_strict_lib.c',
        ],
    ),
]
