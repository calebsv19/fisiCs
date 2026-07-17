from pathlib import Path

from lib.models import DiagnosticExpectation, DiagnosticJsonProbe, DiagnosticProbe


PROBE_DIR = Path(__file__).resolve().parent.parent
MACRO_SOURCE = PROBE_DIR / "diagnostics/12__probe_wave44_macro_compound_designator_missing_equal_then_semantic_followup.c"
INCLUDE_SOURCE = PROBE_DIR / "diagnostics/12__probe_wave44_include_compound_designator_unclosed_bracket_then_semantic_followup.c"
NESTED_MACRO_SOURCE = PROBE_DIR / "diagnostics/12__probe_wave44_nested_macro_forward_compound_designator_missing_equal_then_semantic_followup.c"

RUNTIME_PROBES = []

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="12__probe_wave44_macro_compound_designator_missing_equal_provenance_strict",
        source=MACRO_SOURCE,
        note=(
            "wave44 strict macro provenance: a missing '=' emitted by a compound-literal "
            "macro must remain anchored at the remapped call site and preserve the semantic tail"
        ),
        required_substrings=[
            "Expected '=' after initializer designator",
            "virtual_wave44_macro_compound_callsite.c:14401",
            "Undeclared identifier",
            "wave44_macro_tail_missing",
            "virtual_wave44_macro_compound_callsite.c:14402:12",
        ],
        forbidden_substrings=[
            "Empty initializer for struct variable",
            "Expected '}' to close compound literal initializer",
            "Operator '+' requires arithmetic operands",
            "<unknown>:0",
            "Control reaches end of non-void function",
        ],
        fisics_env={"DISABLE_CODEGEN": "1"},
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave44_macro_compound_designator_missing_equal_provenance_strict",
    ),
    DiagnosticProbe(
        probe_id="12__probe_wave44_include_compound_designator_unclosed_bracket_provenance_strict",
        source=INCLUDE_SOURCE,
        note=(
            "wave44 strict include provenance: unmatched-bracket recovery in a remapped header "
            "must retain the header primary and the separately remapped source tail"
        ),
        required_substrings=[
            "Expected '=' after initializer designator",
            "virtual_wave44_compound_designator_include.h:14442",
            "Undeclared identifier",
            "wave44_include_tail_missing",
            "virtual_wave44_include_compound_tail.c:14464:12",
        ],
        forbidden_substrings=[
            "Empty initializer for struct variable",
            "Expected '}' to close compound literal initializer",
            "Operator '+' requires arithmetic operands",
            "<unknown>:0",
            "Control reaches end of non-void function",
        ],
        fisics_env={"DISABLE_CODEGEN": "1"},
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave44_include_compound_designator_unclosed_bracket_provenance_strict",
    ),
    DiagnosticProbe(
        probe_id="12__probe_wave44_nested_macro_forward_compound_designator_missing_equal_provenance_strict",
        source=NESTED_MACRO_SOURCE,
        note=(
            "wave44 strict nested-macro provenance: a consumed argument forwarded through two "
            "function-like macros must remain anchored at the outer remapped call site"
        ),
        required_substrings=[
            "Expected '=' after initializer designator",
            "virtual_wave44_nested_macro_forward_callsite.c:14481",
            "Undeclared identifier",
            "wave44_nested_macro_tail_missing",
            "virtual_wave44_nested_macro_forward_callsite.c:14482:12",
        ],
        forbidden_substrings=[
            "virtual_wave44_nested_macro_forward_callsite.c:14477",
            "Empty initializer for struct variable",
            "Expected '}' to close compound literal initializer",
            "<unknown>:0",
            "Control reaches end of non-void function",
        ],
        fisics_env={"DISABLE_CODEGEN": "1"},
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave44_nested_macro_forward_compound_designator_missing_equal_provenance_strict",
    ),
]

DIAG_JSON_PROBES = [
    DiagnosticJsonProbe(
        probe_id="12__probe_wave44_macro_compound_designator_missing_equal_provenance_diagjson_strict",
        source=MACRO_SOURCE,
        note="wave44 strict JSON: macro call-site primary precedes the independent semantic tail",
        expected_diagnostics=(
            DiagnosticExpectation(code=1000, line=14401, column=46, has_file=True, severity="error", stage="parse"),
            DiagnosticExpectation(code=2000, line=14402, column=12, has_file=True, severity="error", stage="semantic"),
        ),
        fisics_env={"DISABLE_CODEGEN": "1"},
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave44_macro_compound_designator_missing_equal_provenance_strict",
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_wave44_include_compound_designator_unclosed_bracket_provenance_diagjson_strict",
        source=INCLUDE_SOURCE,
        note="wave44 strict JSON: remapped include primary precedes the remapped source semantic tail",
        expected_diagnostics=(
            DiagnosticExpectation(code=1000, line=14442, column=74, has_file=True, severity="error", stage="parse"),
            DiagnosticExpectation(code=2000, line=14464, column=12, has_file=True, severity="error", stage="semantic"),
        ),
        fisics_env={"DISABLE_CODEGEN": "1"},
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave44_include_compound_designator_unclosed_bracket_provenance_strict",
    ),
    DiagnosticJsonProbe(
        probe_id="12__probe_wave44_nested_macro_forward_compound_designator_missing_equal_provenance_diagjson_strict",
        source=NESTED_MACRO_SOURCE,
        note="wave44 strict JSON: nested forwarding retains the outer argument location before the semantic tail",
        expected_diagnostics=(
            DiagnosticExpectation(code=1000, line=14481, column=57, has_file=True, severity="error", stage="parse"),
            DiagnosticExpectation(code=2000, line=14482, column=12, has_file=True, severity="error", stage="semantic"),
        ),
        fisics_env={"DISABLE_CODEGEN": "1"},
        allowed_exit_codes=(1,),
        promoted_test_id="12__wave44_nested_macro_forward_compound_designator_missing_equal_provenance_strict",
    ),
]
