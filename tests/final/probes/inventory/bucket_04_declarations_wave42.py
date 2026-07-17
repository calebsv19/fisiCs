from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="04__probe_wave42_function_typedef_declaration",
        source=PROBE_DIR / "runtime/04__probe_wave42_function_typedef_declaration_compile.c",
        note=(
            "wave42 strict: an unadorned function-type typedef declarator is a "
            "function declaration and must not lower as a global function-typed object"
        ),
        promoted_test_id="04__compile__wave42_function_typedef_declaration",
    ),
    RuntimeProbe(
        probe_id="04__probe_wave42_function_typedef_array_parameter_definition",
        source=PROBE_DIR / "runtime/04__probe_wave42_function_typedef_array_parameter_definition_runtime.c",
        note=(
            "wave42 strict: a function-type typedef declaration must preserve "
            "array-parameter adjustment when matched by an explicit pointer definition"
        ),
        promoted_test_id="04__runtime__wave42_function_typedef_array_parameter_definition",
    ),
    RuntimeProbe(
        probe_id="04__probe_wave42_function_typedef_function_parameter_definition",
        source=PROBE_DIR / "runtime/04__probe_wave42_function_typedef_function_parameter_definition_runtime.c",
        note=(
            "wave42 strict: a function-type typedef declaration must preserve nested "
            "function-parameter adjustment when matched by a function-pointer definition"
        ),
        promoted_test_id="04__runtime__wave42_function_typedef_function_parameter_definition",
    ),
    RuntimeProbe(
        probe_id="04__probe_wave42_function_typedef_block_scope_redeclaration",
        source=PROBE_DIR / "runtime/04__probe_wave42_function_typedef_block_scope_redeclaration_runtime.c",
        note=(
            "wave42 strict adjacency: a block-scope function-type typedef declaration "
            "reconciles with the visible internal-linkage definition and lowers as a function"
        ),
        promoted_test_id="04__runtime__wave42_function_typedef_block_scope_redeclaration",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="04__probe_wave42_function_typedef_incompatible_definition",
        source=PROBE_DIR / "diagnostics/04__probe_wave42_function_typedef_incompatible_definition.c",
        note=(
            "wave42 strict negative: after reconstructing a function-type typedef "
            "declaration, an incompatible callback return type must conflict"
        ),
        required_substrings=[
            "Conflicting types for function",
            "wave42_apply_conflict",
        ],
        promoted_test_id="04__diag__wave42_function_typedef_incompatible_definition",
    ),
    DiagnosticProbe(
        probe_id="04__probe_wave42_function_typedef_block_static_reject",
        source=PROBE_DIR / "diagnostics/04__probe_wave42_function_typedef_block_static_reject.c",
        note="wave42 strict adjacency: block-scope function declarations reject static storage",
        required_substrings=["Invalid storage class for function declaration", "wave42_block_static"],
        promoted_test_id="04__diag__wave42_function_typedef_block_static_reject",
    ),
    DiagnosticProbe(
        probe_id="04__probe_wave42_function_typedef_linkage_conflict_reject",
        source=PROBE_DIR / "diagnostics/04__probe_wave42_function_typedef_linkage_conflict_reject.c",
        note="wave42 strict adjacency: typedef-written function redeclarations preserve extern/static linkage conflicts",
        required_substrings=["Conflicting types for function", "wave42_linkage_target"],
        promoted_test_id="04__diag__wave42_function_typedef_linkage_conflict_reject",
    ),
]

DIAG_JSON_PROBES = []
