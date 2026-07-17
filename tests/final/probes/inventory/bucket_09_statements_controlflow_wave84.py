from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='09__probe_wave84_goto_bypasses_enum_fixed_array_control',
        source=PROBE_DIR / 'runtime/09__probe_wave84_goto_bypasses_enum_fixed_array_control.c',
        note='wave84 control: an enum integer constant expression produces a fixed array, so goto may bypass its declaration',
        fisics_args=('-std=c99',),
        clang_args=('-pedantic-errors',),
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id='09__probe_wave84_goto_bypasses_const_object_vla_reject',
        source=PROBE_DIR / 'diagnostics/09__probe_wave84_goto_bypasses_const_object_vla_reject.c',
        note='wave84 strict negative: a block-scope const int is not an integer constant expression, so the array is variably modified and goto must not enter its scope',
        required_substrings=('goto jumps into scope of initialized variable',),
        fisics_args=('-std=c99',),
        fisics_env={'DISABLE_CODEGEN': '1'},
        allowed_exit_codes=(1,),
    ),
]

DIAG_JSON_PROBES = []
