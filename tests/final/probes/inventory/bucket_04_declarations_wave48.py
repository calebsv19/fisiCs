from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="04__probe_wave48_typedef_wrapped_nested_factory_progressive_runtime",
        source=PROBE_DIR / "runtime/04__probe_wave48_typedef_wrapped_nested_factory_progressive_runtime.c",
        note=(
            "wave48 typedef-wrapped progressive control: an old-style leaf and "
            "factory return type refine to nested int prototypes and remain callable"
        ),
        promoted_test_id="04__runtime__wave48_typedef_wrapped_nested_factory_progressive",
    ),
    RuntimeProbe(
        probe_id="04__probe_wave48_typedef_wrapped_nested_factory_reverse_runtime",
        source=PROBE_DIR / "runtime/04__probe_wave48_typedef_wrapped_nested_factory_reverse_runtime.c",
        note=(
            "wave48 reverse-order control: the typedef-wrapped nested prototype "
            "survives a later compatible old-style declaration and remains callable"
        ),
        promoted_test_id="04__runtime__wave48_typedef_wrapped_nested_factory_reverse",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="04__probe_wave48_typedef_wrapped_nested_factory_conflict",
        source=PROBE_DIR / "diagnostics/04__probe_wave48_typedef_wrapped_nested_factory_conflict.c",
        note=(
            "wave48 strict negative: after the old-style nested factory return "
            "is refined to int prototypes, a later double callback/factory conflicts"
        ),
        required_substrings=[
            "Conflicting types for function",
            "wave48_conflict_route",
        ],
        allowed_exit_codes=(1,),
        promoted_test_id="04__diag__wave48_typedef_wrapped_nested_factory_conflict",
    ),
]

DIAG_JSON_PROBES = []
