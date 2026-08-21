from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
RUNTIME_DIR = PROBE_DIR / "runtime"
INCLUDE_DIRS = tuple(
    f"-I{RUNTIME_DIR / f'pp_inext_reentry_p{index}'}"
    for index in range(1, 6)
)
GUARDED_INCLUDE_DIRS = tuple(
    f"-I{RUNTIME_DIR / f'pp_inext_reentry_guarded_p{index}'}"
    for index in range(1, 6)
)

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="03__probe_wave43_include_next_once_reentry_runtime",
        source=RUNTIME_DIR / "03__probe_wave43_include_next_once_reentry_runtime.c",
        note=(
            "wave43 include-path frontier: a five-level #include_next chain must "
            "preserve quote/angle next-search order, skip inactive missing includes, "
            "honor two pragma-once barriers, and safely re-enter at the first include "
            "path without restarting the protected chain"
        ),
        fisics_args=INCLUDE_DIRS,
        clang_args=INCLUDE_DIRS,
        expected_exit_code=0,
        promoted_test_id="03__runtime_wave43_include_next_once_reentry",
    ),
    RuntimeProbe(
        probe_id="03__probe_wave43_include_next_once_reentry_guarded_runtime",
        source=RUNTIME_DIR / "03__probe_wave43_include_next_once_reentry_guarded_runtime.c",
        note=(
            "wave43 reduced control: the same five-level #include_next path is "
            "valid when the re-entered first header has a once barrier; quote/angle "
            "search order, inactive missing-include skipping, and protected restart "
            "must preserve the deterministic macro result"
        ),
        fisics_args=GUARDED_INCLUDE_DIRS,
        clang_args=GUARDED_INCLUDE_DIRS,
        expected_exit_code=0,
        promoted_test_id="03__runtime_wave43_include_next_once_reentry_guarded",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
