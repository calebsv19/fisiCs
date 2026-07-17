from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
WAVE58_MAIN = PROBE_DIR / 'runtime/13__probe_wave58_for_header_recovery_aggregate_main.c'
WAVE58_LIB = PROBE_DIR / 'runtime/13__probe_wave58_for_header_recovery_aggregate_lib.c'
WAVE58_INPUTS = [WAVE58_MAIN, WAVE58_LIB]

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id='13__probe_wave58_multitu_for_header_recovery_aggregate_runtime',
        source=WAVE58_MAIN,
        inputs=WAVE58_INPUTS,
        note='wave58 clean lowering control: the exported aggregate return/copy links across two TUs and matches clang at runtime',
        promoted_test_id='13__runtime_wave58_multitu_for_header_recovery_aggregate',
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id='13__probe_wave58_multitu_for_header_recovery_exact_marker',
        source=WAVE58_LIB,
        inputs=WAVE58_INPUTS,
        fisics_args=['-DFISICS_WAVE58_RECOVERY'],
        note='wave58 recovery-to-lowering: full multi-TU compile must retain the exact for-header recovery marker without leaking internal backend failures',
        required_substrings=["Error: expected ';' after for-loop initializer at line 6"],
        forbidden_substrings=[
            'NULL node in codegen',
            'Failed to generate',
            'Skipping LLVM code generation',
        ],
        allowed_exit_codes=(1,),
        promoted_test_id='13__diag_wave58_multitu_for_header_recovery_exact_exit',
    ),
]

DIAG_JSON_PROBES = []
