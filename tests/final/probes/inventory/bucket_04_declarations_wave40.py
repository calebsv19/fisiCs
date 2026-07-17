from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="04__probe_wave40_prototype_scope_pop_typedef_control",
        source=PROBE_DIR / "runtime/04__probe_wave40_prototype_scope_pop_typedef_control.c",
        note=(
            "wave40 alpha control: a prototype parameter name must stop hiding a "
            "typedef when the prototype scope closes"
        ),
        promoted_test_id="04__runtime__wave40_prototype_scope_pop_typedef_control",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="04__probe_wave40_prototype_param_hides_typedef_later",
        source=PROBE_DIR / "diagnostics/04__probe_wave40_prototype_param_hides_typedef_later.c",
        note=(
            "wave40 strict: a named parameter must hide the same-spelled typedef "
            "for each later parameter in its prototype scope"
        ),
        required_substrings=["wave40_direct_t"],
        promoted_test_id="04__diag__wave40_prototype_param_hides_typedef_later",
    ),
    DiagnosticProbe(
        probe_id="04__probe_wave40_fnptr_param_name_hides_typedef_later",
        source=PROBE_DIR / "diagnostics/04__probe_wave40_fnptr_param_name_hides_typedef_later.c",
        note=(
            "wave40 strict: a function-pointer parameter declarator name must hide "
            "the same-spelled typedef for later parameters"
        ),
        required_substrings=["wave40_fnptr_name_t"],
        promoted_test_id="04__diag__wave40_fnptr_param_name_hides_typedef_later",
    ),
    DiagnosticProbe(
        probe_id="04__probe_wave40_prototype_param_hides_typedef_nested_fnptr",
        source=PROBE_DIR / "diagnostics/04__probe_wave40_prototype_param_hides_typedef_nested_fnptr.c",
        note=(
            "wave40 strict: an outer prototype parameter name must hide the "
            "same-spelled typedef inside a later nested callback prototype"
        ),
        required_substrings=["wave40_nested_t"],
        promoted_test_id="04__diag__wave40_prototype_param_hides_typedef_nested_fnptr",
    ),
    DiagnosticProbe(
        probe_id="04__probe_wave40_definition_param_hides_typedef_body",
        source=PROBE_DIR / "diagnostics/04__probe_wave40_definition_param_hides_typedef_body.c",
        note=(
            "wave40 strict: a function-definition parameter name must hide the "
            "same-spelled typedef throughout the function body"
        ),
        required_substrings=["wave40_body_t"],
        promoted_test_id="04__diag__wave40_definition_param_hides_typedef_body",
    ),
]

DIAG_JSON_PROBES = []
