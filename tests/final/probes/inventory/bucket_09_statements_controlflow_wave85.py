from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent


RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='09__probe_wave85_goto_bypasses_scalar_initializer_same_block',
        source=PROBE_DIR / 'runtime/09__probe_wave85_goto_bypasses_scalar_initializer_same_block.c',
        note='wave85 strict C99: a same-block goto may bypass an ordinary initialized scalar when no variably modified identifier is entered',
        fisics_args=('-std=c99',),
        clang_args=('-pedantic-errors',),
        promoted_test_id='09__probe_wave85_goto_bypasses_scalar_initializer_same_block',
    ),
    RuntimeProbe(
        probe_id='09__probe_wave85_goto_bypasses_scalar_initializer_descendant_scope',
        source=PROBE_DIR / 'runtime/09__probe_wave85_goto_bypasses_scalar_initializer_descendant_scope.c',
        note='wave85 strict C99: a goto may enter a descendant block past an ordinary initialized scalar when no variably modified identifier is entered',
        fisics_args=('-std=c99',),
        clang_args=('-pedantic-errors',),
        promoted_test_id='09__probe_wave85_goto_bypasses_scalar_initializer_descendant_scope',
    ),
]


DIAG_PROBES = []
DIAG_JSON_PROBES = []
