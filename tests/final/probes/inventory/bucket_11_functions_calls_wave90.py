from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
STRICT_CLANG_ARGS = ["-pedantic-errors", "-Wall", "-Wextra"]

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="11__probe_wave90_function_type_typedef_factory_explicit_all_fisics",
        source=PROBE_DIR / "runtime/11__probe_wave90_function_type_typedef_factory_main.c",
        inputs=[
            PROBE_DIR / "runtime/11__probe_wave90_function_type_typedef_factory_main.c",
            PROBE_DIR / "runtime/11__probe_wave90_function_type_typedef_factory_explicit_lib.c",
        ],
        note="wave90 strict: a function-type typedef parameter whose factory returns another function-type typedef pointer must be compatible with its fully explicit cross-TU definition",
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="11__runtime_wave90_function_type_typedef_factory_explicit_all_fisics",
    ),
    RuntimeProbe(
        probe_id="11__probe_wave90_function_type_typedef_factory_mixed_clang_callee",
        source=PROBE_DIR / "runtime/11__probe_wave90_function_type_typedef_factory_main.c",
        inputs=[
            PROBE_DIR / "runtime/11__probe_wave90_function_type_typedef_factory_main.c",
        ],
        mixed_clang_inputs=[
            PROBE_DIR / "runtime/11__probe_wave90_function_type_typedef_factory_explicit_lib.c",
        ],
        note="wave90 mixed control: a fisiCs caller compiled from function-type typedef parameter declarations must interoperate with a clang-built fully explicit nested factory callee",
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="11__runtime_wave90_function_type_typedef_factory_mixed_clang_callee",
    ),
    RuntimeProbe(
        probe_id="11__probe_wave90_function_type_typedef_direct_aggregate_control",
        source=PROBE_DIR / "runtime/11__probe_wave90_function_type_typedef_direct_aggregate_main.c",
        inputs=[
            PROBE_DIR / "runtime/11__probe_wave90_function_type_typedef_direct_aggregate_main.c",
            PROBE_DIR / "runtime/11__probe_wave90_function_type_typedef_direct_aggregate_explicit_lib.c",
        ],
        note="wave90 direct control: non-nested function-type typedef parameter adjustment remains compatible with a fully explicit pointer definition across aggregate argument and large-return ABI paths",
        clang_args=STRICT_CLANG_ARGS,
        promoted_test_id="11__runtime_wave90_function_type_typedef_direct_aggregate_control",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
