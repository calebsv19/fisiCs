from pathlib import Path

from lib.models import DiagnosticProbe, RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="04__probe_wave41_nested_prototype_scope_unwind",
        source=PROBE_DIR / "runtime/04__probe_wave41_nested_prototype_scope_unwind.c",
        note=(
            "wave41 strict: a callback parameter's nested prototype scope must "
            "unwind before a later outer parameter reuses the typedef spelling"
        ),
        promoted_test_id="04__runtime__wave41_nested_prototype_scope_unwind",
    ),
    RuntimeProbe(
        probe_id="04__probe_wave41_own_fnptr_declarator_typedef_control",
        source=PROBE_DIR / "runtime/04__probe_wave41_own_fnptr_declarator_typedef_control.c",
        note=(
            "wave41 control: a function-pointer parameter name does not hide a "
            "typedef until after its own complete declarator"
        ),
        promoted_test_id="04__runtime__wave41_own_fnptr_declarator_typedef_control",
    ),
    RuntimeProbe(
        probe_id="04__probe_wave41_later_array_bound_parameter_shadow",
        source=PROBE_DIR / "runtime/04__probe_wave41_later_array_bound_parameter_shadow.c",
        note=(
            "wave41 strict: sizeof in a later array bound resolves a preceding "
            "same-spelled parameter as an expression rather than the hidden typedef"
        ),
        promoted_test_id="04__runtime__wave41_later_array_bound_parameter_shadow",
    ),
    RuntimeProbe(
        probe_id="04__probe_wave41_knr_multi_parameter_promotions",
        source=PROBE_DIR / "runtime/04__probe_wave41_knr_multi_parameter_promotions.c",
        note=(
            "wave41 strict: positional K&R parameters independently promote float "
            "to double and char to int against a multi-parameter prototype"
        ),
        promoted_test_id="04__runtime__wave41_knr_multi_parameter_promotions",
    ),
    RuntimeProbe(
        probe_id="04__probe_wave41_knr_multi_exact_control",
        source=PROBE_DIR / "runtime/04__probe_wave41_knr_multi_exact_control.c",
        note=(
            "wave41 current-threshold control: a multi-parameter K&R definition "
            "with already-promoted double and int types preserves positional ABI"
        ),
        promoted_test_id="04__runtime__wave41_knr_multi_exact_control",
    ),
    RuntimeProbe(
        probe_id="04__probe_wave41_knr_function_parameter_adjustment",
        source=PROBE_DIR / "runtime/04__probe_wave41_knr_function_parameter_adjustment.c",
        note=(
            "wave41 strict: an old-style function parameter declaration adjusts "
            "to a function pointer compatible with the preceding prototype"
        ),
        promoted_test_id="04__runtime__wave41_knr_function_parameter_adjustment",
    ),
    RuntimeProbe(
        probe_id="04__probe_wave41_function_pointer_typed_control",
        source=PROBE_DIR / "runtime/04__probe_wave41_function_pointer_typed_control.c",
        note=(
            "wave41 current-threshold control: a typed function-pointer definition "
            "remains compatible when the old-style adjustment boundary is removed"
        ),
        promoted_test_id="04__runtime__wave41_function_pointer_typed_control",
    ),
]

DIAG_PROBES = [
    DiagnosticProbe(
        probe_id="04__probe_wave41_own_fnptr_name_hides_typedef_later",
        source=PROBE_DIR / "diagnostics/04__probe_wave41_own_fnptr_name_hides_typedef_later.c",
        note=(
            "wave41 strict: after its own declarator completes, a function-pointer "
            "parameter name hides the same-spelled typedef from a later parameter"
        ),
        required_substrings=["wave41_own_later_t"],
        promoted_test_id="04__diag__wave41_own_fnptr_name_hides_typedef_later",
    ),
    DiagnosticProbe(
        probe_id="04__probe_wave41_knr_float_to_int_conflict",
        source=PROBE_DIR / "diagnostics/04__probe_wave41_knr_float_to_int_conflict.c",
        note=(
            "wave41 strict negative: a float K&R parameter promotes to double and "
            "must conflict with a preceding int prototype"
        ),
        required_substrings=["wave41_knr_conflict"],
        promoted_test_id="04__diag__wave41_knr_float_to_int_conflict",
    ),
]

DIAG_JSON_PROBES = []
