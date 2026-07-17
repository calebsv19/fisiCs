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
        promoted_test_id='04__declarator__factory_call_typedef_runtime',
    ),
    RuntimeProbe(
        probe_id='04__probe_deep_declarator_typedef_factory_assignment_runtime',
        source=PROBE_DIR / 'runtime/04__probe_deep_declarator_typedef_factory_assignment_runtime.c',
        note='reduced threshold: typedef-aliased factory assignment lane should compile/run and match clang',
        promoted_test_id='04__declarator__factory_assignment_typedef_runtime',
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
    RuntimeProbe(
        probe_id='04__probe_param_array_extent_adjust_runtime',
        source=PROBE_DIR / 'runtime/04__probe_param_array_extent_adjust_runtime.c',
        note='same-scope array parameter extents should adjust to one compatible pointer parameter type',
    ),
    RuntimeProbe(
        probe_id='04__probe_param_array_const_adjust_runtime',
        source=PROBE_DIR / 'runtime/04__probe_param_array_const_adjust_runtime.c',
        note='same-scope const-qualified array parameters should adjust cleanly to pointer parameters',
    ),
    RuntimeProbe(
        probe_id='04__probe_param_array_pointer_adjust_runtime',
        source=PROBE_DIR / 'runtime/04__probe_param_array_pointer_adjust_runtime.c',
        note='same-scope fixed-size array parameters should adjust cleanly to pointer definitions',
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_param_array_extent_adjust_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_param_array_extent_adjust_bridge_main.c',
        note='multi-TU array parameter extent drift should stay compatible after parameter adjustment',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_param_array_extent_adjust_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_param_array_extent_adjust_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_include_param_array_const_adjust_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_include_param_array_const_adjust_bridge_main.c',
        note='include-boundary const-qualified array parameters should stay compatible with pointer definitions across translation units',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_array_const_adjust_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_array_const_adjust_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_include_param_array_extent_adjust_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_include_param_array_extent_adjust_bridge_main.c',
        note='include-boundary fixed-size array parameters should stay compatible with pointer definitions across translation units',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_array_extent_adjust_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_array_extent_adjust_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_param_array_static_adjust_runtime',
        source=PROBE_DIR / 'runtime/04__probe_param_array_static_adjust_runtime.c',
        note='same-scope static array parameters should adjust cleanly to pointer definitions',
    ),
    RuntimeProbe(
        probe_id='04__probe_param_array_restrict_adjust_runtime',
        source=PROBE_DIR / 'runtime/04__probe_param_array_restrict_adjust_runtime.c',
        note='same-scope restrict-qualified array parameters should adjust cleanly to pointer definitions',
    ),
    RuntimeProbe(
        probe_id='04__probe_param_array_static_const_adjust_runtime',
        source=PROBE_DIR / 'runtime/04__probe_param_array_static_const_adjust_runtime.c',
        note='same-scope static+const array parameters should adjust cleanly to pointer definitions',
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_param_array_static_adjust_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_param_array_static_adjust_bridge_main.c',
        note='multi-TU static array parameters should stay compatible after adjustment',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_param_array_static_adjust_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_param_array_static_adjust_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_include_param_array_restrict_adjust_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_include_param_array_restrict_adjust_bridge_main.c',
        note='include-boundary restrict-qualified array parameters should stay compatible with pointer definitions across translation units',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_array_restrict_adjust_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_array_restrict_adjust_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_include_param_array_static_const_adjust_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_include_param_array_static_const_adjust_bridge_main.c',
        note='include-boundary static+const array parameters should stay compatible with pointer definitions across translation units',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_array_static_const_adjust_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_array_static_const_adjust_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_param_vla_pointer_adjust_runtime',
        source=PROBE_DIR / 'runtime/04__probe_param_vla_pointer_adjust_runtime.c',
        note='same-scope VLA parameters should adjust cleanly to pointer-to-VLA definitions',
    ),
    RuntimeProbe(
        probe_id='04__probe_param_vla_const_pointer_adjust_runtime',
        source=PROBE_DIR / 'runtime/04__probe_param_vla_const_pointer_adjust_runtime.c',
        note='same-scope const-qualified VLA parameters should adjust cleanly to pointer-to-VLA definitions',
    ),
    RuntimeProbe(
        probe_id='04__probe_param_vla_restrict_pointer_adjust_runtime',
        source=PROBE_DIR / 'runtime/04__probe_param_vla_restrict_pointer_adjust_runtime.c',
        note='same-scope restrict-qualified VLA parameters should adjust cleanly to pointer-to-VLA definitions',
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_param_vla_pointer_adjust_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_param_vla_pointer_adjust_bridge_main.c',
        note='multi-TU VLA parameters should stay compatible with pointer-to-VLA definitions',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_param_vla_pointer_adjust_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_param_vla_pointer_adjust_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_include_param_vla_const_pointer_adjust_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_const_pointer_adjust_bridge_main.c',
        note='include-boundary const-qualified VLA parameters should stay compatible with pointer-to-VLA definitions across translation units',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_const_pointer_adjust_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_const_pointer_adjust_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_include_param_vla_restrict_pointer_adjust_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_restrict_pointer_adjust_bridge_main.c',
        note='include-boundary restrict-qualified VLA parameters should stay compatible with pointer-to-VLA definitions across translation units',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_restrict_pointer_adjust_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_restrict_pointer_adjust_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_param_vla_pointer_chain_adjust_runtime',
        source=PROBE_DIR / 'runtime/04__probe_param_vla_pointer_chain_adjust_runtime.c',
        note='same-scope array-of-pointer-to-VLA parameters should adjust cleanly to pointer-to-pointer-to-VLA definitions',
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_include_param_vla_pointer_chain_adjust_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_pointer_chain_adjust_bridge_main.c',
        note='include-boundary array-of-pointer-to-VLA parameters should stay compatible with pointer-to-pointer-to-VLA definitions across translation units',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_pointer_chain_adjust_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_pointer_chain_adjust_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_param_vla_static_pointer_adjust_runtime',
        source=PROBE_DIR / 'runtime/04__probe_param_vla_static_pointer_adjust_runtime.c',
        note='same-scope static VLA parameters should adjust cleanly to pointer-to-VLA definitions',
    ),
    RuntimeProbe(
        probe_id='04__probe_param_vla_static_const_pointer_adjust_runtime',
        source=PROBE_DIR / 'runtime/04__probe_param_vla_static_const_pointer_adjust_runtime.c',
        note='same-scope static+const VLA parameters should adjust cleanly to pointer-to-VLA definitions',
    ),
    RuntimeProbe(
        probe_id='04__probe_param_vla_static_restrict_pointer_adjust_runtime',
        source=PROBE_DIR / 'runtime/04__probe_param_vla_static_restrict_pointer_adjust_runtime.c',
        note='same-scope static+restrict VLA parameters should adjust cleanly to pointer-to-VLA definitions',
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_param_vla_static_adjust_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_param_vla_static_adjust_bridge_main.c',
        note='multi-TU static VLA parameters should stay compatible with pointer-to-VLA definitions',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_param_vla_static_adjust_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_param_vla_static_adjust_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_include_param_vla_static_const_adjust_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_static_const_adjust_bridge_main.c',
        note='include-boundary static+const VLA parameters should stay compatible with pointer-to-VLA definitions across translation units',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_static_const_adjust_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_static_const_adjust_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_include_param_vla_static_restrict_adjust_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_static_restrict_adjust_bridge_main.c',
        note='include-boundary static+restrict VLA parameters should stay compatible with pointer-to-VLA definitions across translation units',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_static_restrict_adjust_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_include_param_vla_static_restrict_adjust_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_param_array_typedef_adjust_runtime',
        source=PROBE_DIR / 'runtime/04__probe_param_array_typedef_adjust_runtime.c',
        note='same-scope typedef-backed array parameters should adjust cleanly to pointer definitions',
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_param_array_typedef_adjust_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_param_array_typedef_adjust_bridge_main.c',
        note='multi-TU typedef-backed array parameters should adjust cleanly despite declaration-side array extent differences',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_param_array_typedef_adjust_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_param_array_typedef_adjust_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_fnptr_typedef_array_param_runtime',
        source=PROBE_DIR / 'runtime/04__probe_fnptr_typedef_array_param_runtime.c',
        note='function-pointer typedef arrays used as parameters should adjust cleanly to pointer-to-callback definitions',
    ),
    RuntimeProbe(
        probe_id='04__probe_multitu_fnptr_typedef_array_param_bridge',
        source=PROBE_DIR / 'runtime/04__probe_multitu_fnptr_typedef_array_param_bridge_main.c',
        note='multi-TU function-pointer typedef arrays used as parameters should stay compatible after adjustment',
        inputs=[
            PROBE_DIR / 'runtime/04__probe_multitu_fnptr_typedef_array_param_bridge_main.c',
            PROBE_DIR / 'runtime/04__probe_multitu_fnptr_typedef_array_param_bridge_lib.c',
        ],
    ),
    RuntimeProbe(
        probe_id='04__probe_local_vla_typedef_pointer_view_runtime',
        source=PROBE_DIR / 'runtime/04__probe_local_vla_typedef_pointer_view_runtime.c',
        note='block-scope VLA typedef pointer views should preserve runtime row stride metadata through nested indexing',
    ),
    RuntimeProbe(
        probe_id='04__probe_scoped_typedef_vla_sizeof_initializer_runtime',
        source=PROBE_DIR / 'runtime/04__probe_scoped_typedef_vla_sizeof_initializer_runtime.c',
        note='block-scope VLA typedefs should preserve runtime extent through sizeof and initializer expressions',
    ),
    RuntimeProbe(
        probe_id='04__probe_scoped_typedef_vla_initializer_current_runtime',
        source=PROBE_DIR / 'runtime/04__probe_scoped_typedef_vla_initializer_current_runtime.c',
        note='reduced threshold: block-scope VLA typedef pointer views should preserve runtime indexing through initializers without sizeof(row_t)',
    ),
    RuntimeProbe(
        probe_id='04__probe_vla_pointer_fnptr_chooser_nested_runtime',
        source=PROBE_DIR / 'runtime/04__probe_vla_pointer_fnptr_chooser_nested_runtime.c',
        note='strict frontier: nested function-pointer declarators accepting pointer-to-VLA typedefs should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='04__probe_vla_pointer_direct_chooser_current_runtime',
        source=PROBE_DIR / 'runtime/04__probe_vla_pointer_direct_chooser_current_runtime.c',
        note='reduced threshold: direct pointer-to-VLA helper calls should match clang runtime behavior before nested function-pointer typedefs',
    ),
    RuntimeProbe(
        probe_id='04__probe_fnptr_typedef_table_sizeof_initializer_runtime',
        source=PROBE_DIR / 'runtime/04__probe_fnptr_typedef_table_sizeof_initializer_runtime.c',
        note='function-pointer table typedefs should remain callable through pointer-to-array aliases used in sizeof-backed initializers',
    ),
    RuntimeProbe(
        probe_id='04__probe_fnptr_typedef_table_direct_initializer_current_runtime',
        source=PROBE_DIR / 'runtime/04__probe_fnptr_typedef_table_direct_initializer_current_runtime.c',
        note='reduced threshold: individual function-pointer typedef initializers should compile/run before array/table aliases',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave25_nested_typedef_vla_array_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave25_nested_typedef_vla_array_runtime.c',
        note='wave25 strict: nested typedef arrays over VLA extents should preserve indexing and runtime sizeof in initializers',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave25_nested_typedef_vla_array_current_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave25_nested_typedef_vla_array_current_runtime.c',
        note='wave25 current threshold: row typedef pointer views preserve VLA indexing and runtime sizeof before nested array typedef indirection',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave25_fnptr_vla_typedef_dispatch_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave25_fnptr_vla_typedef_dispatch_runtime.c',
        note='wave25 strict: function-pointer declarators with pointer-to-VLA typedef parameters should dispatch through typedef tables',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave25_scoped_decl_initializer_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave25_scoped_decl_initializer_runtime.c',
        note='wave25 strict: scoped typedef declarations should initialize pointer views and block-local arrays with fresh VLA extents',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave26_typedef_fnptr_initializer_sizeof_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave26_typedef_fnptr_initializer_sizeof_runtime.c',
        note='wave26: typedef-backed function-pointer table declarators should remain callable through initializer and sizeof surfaces',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave26_vla_param_typedef_adjust_sizeof_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave26_vla_param_typedef_adjust_sizeof_runtime.c',
        note='wave26: VLA parameter adjustment should preserve typedef row metadata through indexing and sizeof',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave26_typedef_declarator_boundary_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave26_typedef_declarator_boundary_runtime.c',
        note='wave26: nested typedef declarators should preserve pointer-to-VLA function-pointer dispatch through initializer surfaces',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave27_nested_typedef_declarator_vla_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave27_nested_typedef_declarator_vla_runtime.c',
        note='wave27 strict frontier: nested typedef declarators over VLA extents should allow pointer view initialization through typedef grid aliases',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave27_nested_typedef_declarator_fixed_current_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave27_nested_typedef_declarator_fixed_current_runtime.c',
        note='wave27 current threshold: fixed-extent nested typedef declarators preserve direct row view metadata before typedef-grid pointer indirection',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave27_fnptr_declarator_array_table_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave27_fnptr_declarator_array_table_runtime.c',
        note='wave27 strict frontier: function-pointer declarator arrays should initialize through nested typedef table aliases',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave27_fnptr_declarator_array_table_current_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave27_fnptr_declarator_array_table_current_runtime.c',
        note='wave27 current threshold: function-pointer declarator arrays remain callable without typedef array initializer nesting',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave27_vla_param_adjust_typedef_chain_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave27_vla_param_adjust_typedef_chain_runtime.c',
        note='wave27: VLA parameter adjustment should preserve typedef row metadata through chained declarations',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave27_scoped_typedef_shadow_declarations_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave27_scoped_typedef_shadow_declarations_runtime.c',
        note='wave27: scoped typedef shadowing in declarations should preserve block-local type identity and VLA sizeof',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave28_typedef_vla_fnptr_initializer_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave28_typedef_vla_fnptr_initializer_runtime.c',
        note='wave28 strict: block typedef pointer-to-VLA declarations should initialize function-pointer picker tables and preserve row sizeof',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave28_fnptr_selector_initializer_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave28_fnptr_selector_initializer_runtime.c',
        note='wave28 strict: typedef selector declarators returning function pointers should initialize through pointer-to-array aliases',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave28_fixed_typedef_initializer_current_runtime',
        source=PROBE_DIR / 'runtime/04__probe_wave28_fixed_typedef_initializer_current_runtime.c',
        note='wave28 current threshold: fixed-array typedef pointer initializers should preserve indexing and sizeof before VLA picker stress',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave29_nested_fnptr_vla_dispatch',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave29_nested_fnptr_vla_dispatch.c',
        note='wave29: nested function-pointer typedef tables should dispatch over pointer-to-VLA typedef views initialized in block scope',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave29_initializer_declarator_vla_views',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave29_initializer_declarator_vla_views.c',
        note='wave29: initializer declarators should preserve block-scope VLA typedef view metadata across multiple declarators',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave29_typedef_fnptr_initializer_chain',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave29_typedef_fnptr_initializer_chain.c',
        note='wave29: nested typedef function-pointer initializers should remain callable through selector aliases and initializer lists',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave30_typedef_vla_fnptr_initializer_matrix',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave30_typedef_vla_fnptr_initializer_matrix.c',
        note='wave30 strict: typedef pointer-to-VLA views should initialize nested function-pointer tables and preserve runtime row metadata',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave30_declarator_fnptr_vla_return_boundary',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave30_declarator_fnptr_vla_return_boundary.c',
        note='wave30 strict: declarator chains returning function pointers should preserve pointer-to-VLA parameter metadata through initializer surfaces',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave30_fixed_fnptr_initializer_current',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave30_fixed_fnptr_initializer_current.c',
        note='wave30 current threshold: fixed-array typedef function-pointer initializer path should remain callable before VLA return-boundary stress',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave31_scoped_typedef_initializer_shadow',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave31_scoped_typedef_initializer_shadow.c',
        note='wave31: scoped typedef shadowing inside initializer expressions should preserve block-local VLA row metadata',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave31_array_param_fnptr_return_adjust',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave31_array_param_fnptr_return_adjust.c',
        note='wave31: array parameter adjustment should remain callable through function-pointer return declarators',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave31_declarator_initializer_ordering',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave31_declarator_initializer_ordering.c',
        note='wave31: declarator initializer ordering should preserve typedef VLA views and function-pointer selections',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave31_nested_fnptr_typedef_vla_view',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave31_nested_fnptr_typedef_vla_view.c',
        note='wave31: nested function-pointer typedef tables should dispatch through pointer-to-VLA view aliases',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave32_typedef_vla_fnptr_rebind',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave32_typedef_vla_fnptr_rebind.c',
        note='wave32: typedef-backed pointer-to-VLA views should stay callable after rebinding through function-pointer tables',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave32_array_param_typedef_callback',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave32_array_param_typedef_callback.c',
        note='wave32: array parameter adjustment should survive typedef callback table dispatch over VLA row views',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave32_factory_returns_vla_reader',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave32_factory_returns_vla_reader.c',
        note='wave32: factories returning pointer-to-VLA reader function pointers should preserve row metadata through typedef views',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave32_scoped_shadow_vla_initializer',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave32_scoped_shadow_vla_initializer.c',
        note='wave32: scoped typedef shadowing should preserve VLA initializer metadata and function-pointer dispatch',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave33_fixed_array_prototype_adjustment',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave33_fixed_array_prototype_adjustment.c',
        note='wave33: fixed-array prototype parameters should adjust to pointer definitions through a function-pointer call',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave33_scoped_typedef_shadow_current',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave33_scoped_typedef_shadow_current.c',
        note='wave33 current threshold: block-local typedef shadowing should preserve the enclosing declaration identity',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave33_scoped_typedef_tag_shadow',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave33_scoped_typedef_tag_shadow.c',
        note='wave33 strict: scoped typedef/tag shadowing should preserve the inner aggregate layout',
    ),
    RuntimeProbe(
        probe_id='04__probe_runtime_wave33_direct_factory_declarator',
        source=PROBE_DIR / 'runtime/04__probe_runtime_wave33_direct_factory_declarator.c',
        note='wave33: direct function-pointer factory declarators should dispatch through a pointer to the factory',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave34_tag_lifecycle_forward_complete_typedef',
        source=PROBE_DIR / 'runtime/04__probe_wave34_tag_lifecycle_forward_complete_typedef.c',
        note='wave34 strict: a forward struct tag should retain identity through completion, later typedef binding, and pointer/member/sizeof use',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave34_tag_lifecycle_forward_typedef_complete',
        source=PROBE_DIR / 'runtime/04__probe_wave34_tag_lifecycle_forward_typedef_complete.c',
        note='wave34 strict: a typedef bound to an incomplete struct should observe the later tag completion during pointer/member/sizeof use',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave35_tag_only_shadow_same_name_identity',
        source=PROBE_DIR / 'runtime/04__probe_wave35_tag_only_shadow_same_name_identity.c',
        note='wave35 strict: a predeclared outer struct object and pointer retain their type identity during inner same-name tag-only shadowing',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave35_tag_only_shadow_unique_name_control',
        source=PROBE_DIR / 'runtime/04__probe_wave35_tag_only_shadow_unique_name_control.c',
        note='wave35 alpha control: the same outer object/member/sizeof path should match when the inner tag has a unique name',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave36_pointer_derived_tag_shadow_same_name',
        source=PROBE_DIR / 'runtime/04__probe_wave36_pointer_derived_tag_shadow_same_name.c',
        note='wave36 strict: a pointer declared before same-name tag shadowing should retain its aggregate element type for arrow, dereference, sizeof, and stride',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave36_pointer_derived_tag_shadow_unique_name_control',
        source=PROBE_DIR / 'runtime/04__probe_wave36_pointer_derived_tag_shadow_unique_name_control.c',
        note='wave36 alpha control: pointer-derived aggregate member and stride behavior should match when the inner tag has a unique name',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave37_relay_tag_shadow_same_name',
        source=PROBE_DIR / 'runtime/04__probe_wave37_relay_tag_shadow_same_name.c',
        note='wave37 strict: a pointer-to-pointer relay declared before same-name tag shadowing should retain aggregate identity through arrow, double dereference, sizeof, and stride',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave37_relay_tag_shadow_unique_name_control',
        source=PROBE_DIR / 'runtime/04__probe_wave37_relay_tag_shadow_unique_name_control.c',
        note='wave37 alpha control: relay-derived aggregate access and stride should match when the inner tag has a unique name',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave38_nested_oldstyle_fnptr_compatible',
        source=PROBE_DIR / 'runtime/04__probe_wave38_nested_oldstyle_fnptr_compatible.c',
        note='strict frontier: a nested old-style callback parameter should remain compatible with an int prototype because int survives default argument promotions',
        promoted_test_id='04__runtime__wave38_nested_oldstyle_fnptr_compatible',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave38_nested_oldstyle_double_fnptr_compatible',
        source=PROBE_DIR / 'runtime/04__probe_wave38_nested_oldstyle_double_fnptr_compatible.c',
        note='strict frontier: a nested old-style callback parameter should remain compatible with a double prototype because double survives default argument promotions',
        promoted_test_id='04__runtime__wave38_nested_oldstyle_double_fnptr_compatible',
    ),
    RuntimeProbe(
        probe_id='04__probe_wave38_nested_void_oldstyle_fnptr_compatible',
        source=PROBE_DIR / 'runtime/04__probe_wave38_nested_void_oldstyle_fnptr_compatible.c',
        note='strict frontier control: an explicit void callback prototype should remain compatible with an old-style empty parameter list',
        promoted_test_id='04__runtime__wave47_nested_void_oldstyle_fnptr_compatible',
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
        probe_id='04__probe_param_array_const_adjust_current_reject',
        source=PROBE_DIR / 'runtime/04__probe_param_array_const_adjust_runtime.c',
        note='fixed baseline: const-qualified array parameter adjustment now compiles cleanly',
        expect_any_diagnostic=False,
        required_substrings=[
            'Semantic analysis: no issues found.',
        ],
    ),
    DiagnosticProbe(
        probe_id='04__probe_multitu_include_param_array_const_adjust_current_reject',
        source=PROBE_DIR / 'runtime/04__probe_multitu_include_param_array_const_adjust_bridge_lib.c',
        note='fixed baseline: include-boundary const-qualified array parameter adjustment now compiles cleanly',
        expect_any_diagnostic=False,
        required_substrings=[
            'Semantic analysis: no issues found.',
        ],
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
        note='current semantics: multi-TU function-pointer parameter drift remains a no-diagnostic frontend lane under #line',
        expect_any_diagnostic=False,
        required_substrings=[
            'Semantic analysis: no issues found.',
        ],
        fisics_env={'DISABLE_CODEGEN': '1'},
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_fnptr_param_conflict_spelling_strict_lib.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_fnptr_param_conflict_spelling_strict_main.c',
        ],
    ),
    DiagnosticProbe(
        probe_id='04__probe_diag_line_directive_multitu_include_fnptr_param_conflict_spelling_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_fnptr_param_conflict_spelling_strict_main.c',
        note='include-boundary function-pointer parameter drift in the defining TU must be rejected with remapped spelling',
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
    DiagnosticProbe(
        probe_id='04__probe_wave29_typedef_fnptr_vla_current_no_diag',
        source=PROBE_DIR / 'diagnostics/04__probe_wave29_typedef_fnptr_vla_current_no_diag.c',
        note='wave29 current threshold: nested typedef function-pointer declarations over pointer-to-VLA forms should remain a no-diagnostic frontend lane',
        expect_any_diagnostic=False,
        required_substrings=[
            'Semantic analysis: no issues found.',
        ],
        fisics_env={'DISABLE_CODEGEN': '1'},
    ),
    DiagnosticProbe(
        probe_id='04__probe_wave38_fnptr_return_redeclaration_conflict',
        source=PROBE_DIR / 'diagnostics/04__probe_wave38_fnptr_return_redeclaration_conflict.c',
        note='wave38 strict: function redeclarations must reject incompatible function-pointer return signatures after typedef resolution',
        required_substrings=[
            'Conflicting types for function',
            'virtual_decl_wave38_fnptr_return_conflict.c:38005',
        ],
        promoted_test_id='04__diag__wave38_fnptr_return_redeclaration_conflict',
    ),
    DiagnosticProbe(
        probe_id='04__probe_wave38_nested_oldstyle_float_fnptr_conflict',
        source=PROBE_DIR / 'diagnostics/04__probe_wave38_nested_oldstyle_float_fnptr_conflict.c',
        note='strict frontier control: a nested old-style callback must conflict with a float prototype because default argument promotions change float to double',
        promoted_test_id='04__diag__wave47_nested_oldstyle_float_fnptr_conflict',
        required_substrings=[
            'Conflicting types for function',
            'virtual_decl_wave38_oldstyle_float_conflict.c:38021',
        ],
    ),
    DiagnosticProbe(
        probe_id='04__probe_wave38_nested_void_int_fnptr_conflict',
        source=PROBE_DIR / 'diagnostics/04__probe_wave38_nested_void_int_fnptr_conflict.c',
        note='strict frontier control: an explicit void callback prototype must remain incompatible with an int callback prototype',
        promoted_test_id='04__diag__wave47_nested_void_int_fnptr_conflict',
        required_substrings=[
            'Conflicting types for function',
            'virtual_decl_wave38_void_int_conflict.c:38031',
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
        note='current semantics: multi-TU function-pointer parameter drift exports diagnostics JSON without frontend diagnostics under #line',
        require_any_diagnostic=False,
        fisics_env={'DISABLE_CODEGEN': '1'},
        inputs=[
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_fnptr_param_conflict_spelling_strict_lib.c',
            PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_fnptr_param_conflict_spelling_strict_main.c',
        ],
    ),
    DiagnosticJsonProbe(
        probe_id='04__probe_diagjson_line_directive_multitu_include_fnptr_param_conflict_rich_strict',
        source=PROBE_DIR / 'diagnostics/04__probe_diag_line_directive_multitu_include_fnptr_param_conflict_spelling_strict_main.c',
        note='include-boundary function-pointer parameter drift must export a structured semantic diagnostic under #line',
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
