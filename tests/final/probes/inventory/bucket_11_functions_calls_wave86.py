from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='11__probe_wave86_variadic_float_complex_runtime',
        source=PROBE_DIR / 'runtime/11__probe_wave86_variadic_float_complex_runtime.c',
        note='wave86: float _Complex must preserve both float lanes through ellipsis and va_arg while a neighboring real float undergoes default promotion to double',
        clang_args=['-pedantic-errors'],
    ),
]
DIAG_PROBES = []
DIAG_JSON_PROBES = []
