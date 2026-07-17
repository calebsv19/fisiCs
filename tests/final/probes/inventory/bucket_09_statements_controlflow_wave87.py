from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='09__probe_wave87_switch_nested_fixed_array_case_dispatch_control',
        source=PROBE_DIR / 'runtime/09__probe_wave87_switch_nested_fixed_array_case_dispatch_control.c',
        note='wave87 strict control: switch dispatch must preserve a case label nested after an ordinary fixed-array declaration',
        fisics_args=('-std=c99',),
        clang_args=('-pedantic-errors',),
        promoted_test_id='09__probe_wave87_switch_nested_fixed_array_case_dispatch_control',
    ),
    RuntimeProbe(
        probe_id='09__probe_wave87_switch_nested_enum_fixed_typedef_alias_dispatch_control',
        source=PROBE_DIR / 'runtime/09__probe_wave87_switch_nested_enum_fixed_typedef_alias_dispatch_control.c',
        note='wave87 adjacency control: enum-ICE fixed typedef aliases remain legal across nested case dispatch and retain their sizeof',
        fisics_args=('-std=c99',),
        clang_args=('-pedantic-errors',),
        promoted_test_id='09__probe_wave87_switch_nested_enum_fixed_typedef_alias_dispatch_control',
    ),
    RuntimeProbe(
        probe_id='09__probe_wave87_switch_nested_local_enum_case_value_control',
        source=PROBE_DIR / 'runtime/09__probe_wave87_switch_nested_local_enum_case_value_control.c',
        note='wave87 review regression: nested case dispatch must use the semantic value of an earlier block-local enum constant',
        fisics_args=('-std=c99',),
        clang_args=('-pedantic-errors',),
        promoted_test_id='09__probe_wave87_switch_nested_local_enum_case_value_control',
    ),
]


DIAG_PROBES = [
    DiagnosticProbe(
        probe_id='09__probe_wave87_switch_dispatch_into_nested_vla_scope_reject',
        source=PROBE_DIR / 'diagnostics/09__probe_wave87_switch_dispatch_into_nested_vla_scope_reject.c',
        note='wave87 strict: implicit switch dispatch must not enter a nested case past a VLA declaration',
        required_substrings=('switch dispatch jumps into scope of variably modified declaration',),
        fisics_args=('-std=c99',),
        fisics_env={'DISABLE_CODEGEN': '1'},
        allowed_exit_codes=(1,),
        promoted_test_id='09__probe_wave87_switch_dispatch_into_nested_vla_scope_reject',
    ),
    DiagnosticProbe(
        probe_id='09__probe_wave87_switch_dispatch_into_nested_vm_typedef_alias_scope_reject',
        source=PROBE_DIR / 'diagnostics/09__probe_wave87_switch_dispatch_into_nested_vm_typedef_alias_scope_reject.c',
        note='wave87 adjacency: implicit switch dispatch must not enter a nested case past a variably modified typedef alias chain',
        required_substrings=(
            'switch dispatch jumps into scope of variably modified declaration',
            'case 1 bypasses Wave87Alias',
        ),
        fisics_args=('-std=c99',),
        fisics_env={'DISABLE_CODEGEN': '1'},
        allowed_exit_codes=(1,),
        promoted_test_id='09__probe_wave87_switch_dispatch_into_nested_vm_typedef_alias_scope_reject',
    ),
]


DIAG_JSON_PROBES = []
