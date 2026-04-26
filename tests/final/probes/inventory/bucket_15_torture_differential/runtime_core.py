from pathlib import Path

from lib.models import DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='15__probe_switch_loop_lite',
        source=PROBE_DIR / 'runtime/15__probe_switch_loop_lite.c',
        note='loop+switch lowering should match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_switch_loop_mod5',
        source=PROBE_DIR / 'runtime/15__probe_switch_loop_mod5.c',
        note='loop+mod+switch lowering should match clang',
    ),
    RuntimeProbe(
        probe_id='15__probe_path_decl_nested_runtime',
        source=PROBE_DIR / 'runtime/15__probe_path_decl_nested_runtime.c',
        note='function-pointer runtime call path should not crash',
    ),
    RuntimeProbe(
        probe_id='15__probe_deep_switch_loop_state_machine',
        source=PROBE_DIR / 'runtime/15__probe_deep_switch_loop_state_machine.c',
        note='deep loop+switch state machine should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_switch_sparse_case_jump_table',
        source=PROBE_DIR / 'runtime/15__probe_switch_sparse_case_jump_table.c',
        note='sparse-case switch control lowering should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_declarator_depth_runtime_chain',
        source=PROBE_DIR / 'runtime/15__probe_declarator_depth_runtime_chain.c',
        note='declarator-depth function-pointer runtime chain should match clang behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_many_globals_crossref',
        source=PROBE_DIR / 'runtime/15__probe_multitu_many_globals_crossref_main.c',
        note='multi-TU many-globals cross-reference path should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_many_globals_crossref_main.c', PROBE_DIR / 'runtime/15__probe_multitu_many_globals_crossref_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_multitu_fnptr_dispatch_grid',
        source=PROBE_DIR / 'runtime/15__probe_multitu_fnptr_dispatch_grid_main.c',
        note='multi-TU function-pointer dispatch grid should match clang runtime behavior',
        inputs=[PROBE_DIR / 'runtime/15__probe_multitu_fnptr_dispatch_grid_main.c', PROBE_DIR / 'runtime/15__probe_multitu_fnptr_dispatch_grid_lib.c'],
    ),
    RuntimeProbe(
        probe_id='15__probe_deep_recursion_stack_pressure',
        source=PROBE_DIR / 'runtime/15__probe_deep_recursion_stack_pressure.c',
        note='deep recursion with per-frame stack payload should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_deep_recursion_stack_pressure_ii',
        source=PROBE_DIR / 'runtime/15__probe_deep_recursion_stack_pressure_ii.c',
        note='deeper recursion with larger per-frame payload should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_deep_recursion_stack_pressure_iii',
        source=PROBE_DIR / 'runtime/15__probe_deep_recursion_stack_pressure_iii.c',
        note='third-tier recursion pressure lane should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_large_vla_stride_pressure',
        source=PROBE_DIR / 'runtime/15__probe_large_vla_stride_pressure.c',
        note='large local VLA row-stride and pointer-delta stress path should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_many_args_regstack_pressure',
        source=PROBE_DIR / 'runtime/15__probe_many_args_regstack_pressure.c',
        note='high-arity mixed scalar+struct call pressure should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_seeded_expr_fuzz_smoke',
        source=PROBE_DIR / 'runtime/15__probe_seeded_expr_fuzz_smoke.c',
        note='deterministic seeded expression stress lane should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_seeded_stmt_fuzz_smoke',
        source=PROBE_DIR / 'runtime/15__probe_seeded_stmt_fuzz_smoke.c',
        note='deterministic seeded statement/control stress lane should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_corpus_micro_compile_smoke',
        source=PROBE_DIR / 'runtime/15__probe_corpus_micro_compile_smoke.c',
        note='deterministic corpus-style micro workload should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_control_rebind_dispatch_lattice',
        source=PROBE_DIR / 'runtime/15__probe_control_rebind_dispatch_lattice.c',
        note='deep control-flow with function-pointer rebinding lattice should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_large_struct_array_checksum_grid',
        source=PROBE_DIR / 'runtime/15__probe_large_struct_array_checksum_grid.c',
        note='large local struct-array checksum grid should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_many_decls_density_pressure_ii',
        source=PROBE_DIR / 'runtime/15__probe_many_decls_density_pressure_ii.c',
        note='dense declaration-matrix pressure lane should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_many_decls_density_pressure_iii',
        source=PROBE_DIR / 'runtime/15__probe_many_decls_density_pressure_iii.c',
        note='third-tier dense declaration-matrix pressure lane should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_large_struct_array_pressure_ii',
        source=PROBE_DIR / 'runtime/15__probe_large_struct_array_pressure_ii.c',
        note='larger local struct-array pressure lane should match clang runtime behavior',
    ),
    RuntimeProbe(
        probe_id='15__probe_large_struct_array_pressure_iii',
        source=PROBE_DIR / 'runtime/15__probe_large_struct_array_pressure_iii.c',
        note='third-tier larger local struct-array pressure lane should match clang runtime behavior',
    ),
]

DIAG_PROBES = []

DIAG_JSON_PROBES = []
