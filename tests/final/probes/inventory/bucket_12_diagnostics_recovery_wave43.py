from pathlib import Path

from lib.models import DiagnosticExpectation, DiagnosticJsonProbe, DiagnosticProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
SOURCE = PROBE_DIR / "diagnostics/12__probe_wave43_compound_literal_designator_missing_equal_then_semantic_followup.c"

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="12__probe_wave43_compound_literal_designator_unclosed_paren_tail_strict",
        source=PROBE_DIR / "diagnostics/12__probe_wave43_compound_literal_designator_unclosed_paren_then_semantic_followup.c",
        note=(
            "wave43 strict recovery guard: an unmatched '(' in the discarded missing-'=' "
            "payload must not consume the owning brace or semantic tail"
        ),
        required_substrings=[
            "Expected '=' after initializer designator",
            "virtual_wave43_compound_unclosed_paren.c:14321",
            "Undeclared identifier",
            "wave43_paren_tail_missing",
            "virtual_wave43_compound_unclosed_paren.c:14322:12",
        ],
        forbidden_substrings=[
            "Empty initializer for struct variable",
            "Expected '}' to close compound literal initializer",
            "<unknown>:0",
            "Invalid initializer in variable declaration",
            "invalid statement inside block",
            "Control reaches end of non-void function",
        ],
        fisics_env={"DISABLE_CODEGEN": "1"},
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave43_compound_literal_designator_unclosed_paren_tail_strict",
    ),
    DiagnosticProbe(
        probe_id="12__probe_wave43_compound_literal_designator_unclosed_bracket_tail_strict",
        source=PROBE_DIR / "diagnostics/12__probe_wave43_compound_literal_designator_unclosed_bracket_then_semantic_followup.c",
        note=(
            "wave43 strict recovery guard: an unmatched '[' in the discarded missing-'=' "
            "payload must not consume the owning brace or semantic tail"
        ),
        required_substrings=[
            "Expected '=' after initializer designator",
            "virtual_wave43_compound_unclosed_bracket.c:14341",
            "Undeclared identifier",
            "wave43_bracket_tail_missing",
            "virtual_wave43_compound_unclosed_bracket.c:14342:12",
        ],
        forbidden_substrings=[
            "Empty initializer for struct variable",
            "Expected '}' to close compound literal initializer",
            "<unknown>:0",
            "Invalid initializer in variable declaration",
            "invalid statement inside block",
            "Control reaches end of non-void function",
        ],
        fisics_env={"DISABLE_CODEGEN": "1"},
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave43_compound_literal_designator_unclosed_bracket_tail_strict",
    ),
    DiagnosticProbe(
        probe_id="12__probe_wave43_compound_literal_designator_missing_equal_clang_parity_strict",
        source=SOURCE,
        note=(
            "wave43 strict: a valid compound-literal field designator missing '=' should "
            "produce one primary parser diagnostic, preserve the remapped semantic tail, "
            "and avoid delimiter cascades or a bogus physical-file end-of-function error"
        ),
        required_substrings=[
            "Expected '=' after initializer designator",
            "virtual_wave43_compound_missing_equal.c:14301",
            "Undeclared identifier",
            "wave43_tail_missing",
            "virtual_wave43_compound_missing_equal.c:14302:12",
        ],
        forbidden_substrings=[
            "Invalid initializer in variable declaration",
            "Invalid expression after ','",
            "Unexpected token at start of expression",
            "invalid statement inside block",
            "Control reaches end of non-void function",
        ],
        fisics_env={"DISABLE_CODEGEN": "1"},
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave43_compound_literal_designator_missing_equal_strict",
    ),
]

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="12__probe_wave43_compound_literal_designator_unclosed_paren_tail_diagjson_strict",
        source=PROBE_DIR / "diagnostics/12__probe_wave43_compound_literal_designator_unclosed_paren_then_semantic_followup.c",
        note=(
            "wave43 strict JSON guard: unmatched-parenthesis synchronization must "
            "retain the located primary and semantic-tail records"
        ),
        expected_diagnostics=(
            DiagnosticExpectation(code=1000, line=14321, column=70, has_file=True, severity="error", stage="parse"),
            DiagnosticExpectation(code=2000, line=14322, column=12, has_file=True, severity="error", stage="semantic"),
        ),
        fisics_env={"DISABLE_CODEGEN": "1"},
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave43_compound_literal_designator_unclosed_paren_tail_strict",
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_wave43_compound_literal_designator_unclosed_bracket_tail_diagjson_strict",
        source=PROBE_DIR / "diagnostics/12__probe_wave43_compound_literal_designator_unclosed_bracket_then_semantic_followup.c",
        note=(
            "wave43 strict JSON guard: unmatched-bracket synchronization must "
            "retain the located primary and semantic-tail records"
        ),
        expected_diagnostics=(
            DiagnosticExpectation(code=1000, line=14341, column=74, has_file=True, severity="error", stage="parse"),
            DiagnosticExpectation(code=2000, line=14342, column=12, has_file=True, severity="error", stage="semantic"),
        ),
        fisics_env={"DISABLE_CODEGEN": "1"},
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave43_compound_literal_designator_unclosed_bracket_tail_strict",
    ),
]
