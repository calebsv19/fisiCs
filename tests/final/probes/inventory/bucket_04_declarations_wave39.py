from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="04__probe_wave39_knr_definition_char_to_int_strict",
        source=PROBE_DIR / "runtime/04__probe_wave39_knr_definition_char_to_int_strict.c",
        note=(
            "wave39 strict: an identifier-list definition whose char parameter "
            "promotes to int must remain compatible with a preceding int prototype"
        ),
        promoted_test_id="04__runtime__wave39_knr_definition_char_to_int_strict",
    ),
    RuntimeProbe(
        probe_id="04__probe_wave39_knr_definition_int_exact_control",
        source=PROBE_DIR / "runtime/04__probe_wave39_knr_definition_int_exact_control.c",
        note=(
            "wave39 current-threshold control: an identifier-list definition with "
            "an exact int parameter remains compatible with a preceding int prototype"
        ),
        promoted_test_id="04__runtime__wave39_knr_definition_int_exact_control",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
