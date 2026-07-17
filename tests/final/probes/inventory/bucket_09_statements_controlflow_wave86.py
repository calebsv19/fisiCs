from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='09__probe_wave86_goto_bypasses_enum_fixed_typedef_alias_control',
        source=PROBE_DIR / 'runtime/09__probe_wave86_goto_bypasses_enum_fixed_typedef_alias_control.c',
        note='wave86 control: enum-bound fixed array typedef aliases are not variably modified, so goto may cross their declarations',
        fisics_args=('-std=c99',),
        clang_args=('-pedantic-errors',),
        promoted_test_id='09__probe_wave86_goto_bypasses_enum_fixed_typedef_alias_control',
    ),
]


DIAG_PROBES = [
    DiagnosticProbe(
        probe_id='09__probe_wave86_goto_bypasses_vm_typedef_same_block_reject',
        source=PROBE_DIR / 'diagnostics/09__probe_wave86_goto_bypasses_vm_typedef_same_block_reject.c',
        note='wave86 strict negative: goto must not enter the scope of a directly declared variably modified typedef',
        required_substrings=('goto jumps into scope of initialized variable',),
        fisics_args=('-std=c99',),
        fisics_env={'DISABLE_CODEGEN': '1'},
        allowed_exit_codes=(1,),
        promoted_test_id='09__probe_wave86_goto_bypasses_vm_typedef_same_block_reject',
    ),
    DiagnosticProbe(
        probe_id='09__probe_wave86_goto_bypasses_vm_typedef_alias_object_descendant_reject',
        source=PROBE_DIR / 'diagnostics/09__probe_wave86_goto_bypasses_vm_typedef_alias_object_descendant_reject.c',
        note='wave86 strict negative: goto must not enter a descendant scope past a named VLA object resolved through a two-link typedef alias chain',
        required_substrings=('goto jumps into scope of initialized variable',),
        fisics_args=('-std=c99',),
        fisics_env={'DISABLE_CODEGEN': '1'},
        allowed_exit_codes=(1,),
        promoted_test_id='09__probe_wave86_goto_bypasses_vm_typedef_alias_object_descendant_reject',
    ),
    DiagnosticProbe(
        probe_id='09__probe_wave86_goto_bypasses_vm_typedef_indirect_label_reject',
        source=PROBE_DIR / 'diagnostics/09__probe_wave86_goto_bypasses_vm_typedef_indirect_label_reject.c',
        note='wave86 strict adjacency: an indirect same-block label does not bypass a variably modified typedef scope barrier',
        required_substrings=('goto jumps into scope of initialized variable',),
        fisics_args=('-std=c99',),
        fisics_env={'DISABLE_CODEGEN': '1'},
        allowed_exit_codes=(1,),
        promoted_test_id='09__probe_wave86_goto_bypasses_vm_typedef_indirect_label_reject',
    ),
    DiagnosticProbe(
        probe_id='09__probe_wave86_goto_bypasses_for_init_vla_reject',
        source=PROBE_DIR / 'diagnostics/09__probe_wave86_goto_bypasses_for_init_vla_reject.c',
        note='wave86 strict adjacency: a goto from outside a for statement cannot enter the scope of its VLA initializer',
        required_substrings=('goto jumps into scope of initialized variable',),
        fisics_args=('-std=c99',),
        fisics_env={'DISABLE_CODEGEN': '1'},
        allowed_exit_codes=(1,),
        promoted_test_id='09__probe_wave86_goto_bypasses_for_init_vla_reject',
    ),
]


DIAG_JSON_PROBES = []
