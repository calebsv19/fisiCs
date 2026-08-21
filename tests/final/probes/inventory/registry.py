from . import bucket_04_declarations
from . import bucket_04_declarations_wave39
from . import bucket_04_declarations_wave40
from . import bucket_04_declarations_wave41
from . import bucket_04_declarations_wave42
from . import bucket_04_declarations_wave43
from . import bucket_04_declarations_wave44
from . import bucket_04_declarations_wave45
from . import bucket_04_declarations_wave46
from . import bucket_04_declarations_wave48
from . import bucket_04_declarations_wave49
from . import bucket_04_declarations_wave50
from . import bucket_07_types_conversions
from . import bucket_07_types_conversions_wave87
from . import bucket_07_types_conversions_wave88
from . import bucket_07_types_conversions_wave89
from . import bucket_07_types_conversions_wave90
from . import bucket_10_scopes_linkage
from . import bucket_10_scopes_linkage_wave67
from . import bucket_10_scopes_linkage_wave69
from . import bucket_10_scopes_linkage_wave70
from . import bucket_11_functions_calls
from . import bucket_11_functions_calls_wave83
from . import bucket_11_functions_calls_wave84
from . import bucket_11_functions_calls_wave85
from . import bucket_11_functions_calls_wave86
from . import bucket_11_functions_calls_wave87
from . import bucket_11_functions_calls_wave88
from . import bucket_11_functions_calls_wave89
from . import bucket_11_functions_calls_wave90
from . import bucket_11_functions_calls_wave91
from . import bucket_11_functions_calls_wave92
from . import bucket_13_codegen_ir
from . import bucket_13_codegen_ir_wave57
from . import bucket_13_codegen_ir_wave58
from . import bucket_13_codegen_ir_wave59
from . import bucket_13_codegen_ir_wave61
from . import bucket_13_codegen_ir_wave62
from . import bucket_13_codegen_ir_wave63
from . import bucket_13_codegen_ir_wave64
from . import bucket_13_codegen_ir_wave65
from . import bucket_13_codegen_ir_wave66
from . import bucket_13_codegen_ir_wave67
from . import bucket_13_codegen_ir_wave68
from . import bucket_14_runtime_surface
from . import bucket_15_torture_differential
from .promotion_closure_owners import PROMOTION_CLOSURE_OWNERS
from . import bucket_05_expressions
from . import bucket_05_expressions_wave28
from . import bucket_05_expressions_wave29
from . import bucket_05_expressions_wave30
from . import bucket_05_expressions_wave31
from . import bucket_05_expressions_wave32
from . import bucket_05_expressions_wave34
from . import bucket_05_expressions_wave35
from . import bucket_09_statements_controlflow
from . import bucket_09_statements_controlflow_wave84
from . import bucket_09_statements_controlflow_wave85
from . import bucket_09_statements_controlflow_wave86
from . import bucket_09_statements_controlflow_wave87
from . import bucket_09_statements_controlflow_wave88
from . import bucket_01_translation_phases
from . import bucket_02_lexer
from . import bucket_03_preprocessor
from . import bucket_03_preprocessor_wave39
from . import bucket_03_preprocessor_wave41
from . import bucket_03_preprocessor_wave43
from . import bucket_06_lvalues_rvalues
from . import bucket_06_lvalues_rvalues_wave26
from . import bucket_06_lvalues_rvalues_wave27
from . import bucket_06_lvalues_rvalues_wave28
from . import bucket_08_initializers_layout
from . import bucket_08_initializers_layout_wave86
from . import bucket_08_initializers_layout_wave87
from . import bucket_08_initializers_layout_wave88
from . import bucket_08_initializers_layout_wave89
from . import bucket_08_initializers_layout_wave90
from . import bucket_08_initializers_layout_wave91
from . import bucket_07_types_conversions_wave86
from . import bucket_12_diagnostics_recovery
from . import bucket_12_diagnostics_recovery_wave39_identity
from . import bucket_12_diagnostics_recovery_wave42
from . import bucket_12_diagnostics_recovery_wave43
from . import bucket_12_diagnostics_recovery_wave44
from . import bucket_12_diagnostics_recovery_wave45


RUNTIME_PROBES = (
    bucket_01_translation_phases.RUNTIME_PROBES +
    bucket_02_lexer.RUNTIME_PROBES +
    bucket_03_preprocessor.RUNTIME_PROBES +
    bucket_03_preprocessor_wave39.RUNTIME_PROBES +
    bucket_03_preprocessor_wave41.RUNTIME_PROBES +
    bucket_03_preprocessor_wave43.RUNTIME_PROBES +
    bucket_04_declarations.RUNTIME_PROBES +
    bucket_04_declarations_wave39.RUNTIME_PROBES +
    bucket_04_declarations_wave40.RUNTIME_PROBES +
    bucket_04_declarations_wave41.RUNTIME_PROBES +
    bucket_04_declarations_wave42.RUNTIME_PROBES +
    bucket_04_declarations_wave43.RUNTIME_PROBES +
    bucket_04_declarations_wave44.RUNTIME_PROBES +
    bucket_04_declarations_wave45.RUNTIME_PROBES +
    bucket_04_declarations_wave46.RUNTIME_PROBES +
    bucket_04_declarations_wave48.RUNTIME_PROBES +
    bucket_04_declarations_wave49.RUNTIME_PROBES +
    bucket_04_declarations_wave50.RUNTIME_PROBES +
    bucket_06_lvalues_rvalues.RUNTIME_PROBES +
    bucket_06_lvalues_rvalues_wave26.RUNTIME_PROBES +
    bucket_06_lvalues_rvalues_wave27.RUNTIME_PROBES +
    bucket_06_lvalues_rvalues_wave28.RUNTIME_PROBES +
    bucket_07_types_conversions.RUNTIME_PROBES +
    bucket_07_types_conversions_wave87.RUNTIME_PROBES +
    bucket_07_types_conversions_wave86.RUNTIME_PROBES +
    bucket_07_types_conversions_wave89.RUNTIME_PROBES +
    bucket_07_types_conversions_wave90.RUNTIME_PROBES +
    bucket_08_initializers_layout.RUNTIME_PROBES +
    bucket_08_initializers_layout_wave86.RUNTIME_PROBES +
    bucket_08_initializers_layout_wave87.RUNTIME_PROBES +
    bucket_08_initializers_layout_wave88.RUNTIME_PROBES +
    bucket_08_initializers_layout_wave89.RUNTIME_PROBES +
    bucket_08_initializers_layout_wave90.RUNTIME_PROBES +
    bucket_08_initializers_layout_wave91.RUNTIME_PROBES +
    bucket_10_scopes_linkage.RUNTIME_PROBES +
    bucket_10_scopes_linkage_wave67.RUNTIME_PROBES +
    bucket_10_scopes_linkage_wave69.RUNTIME_PROBES +
    bucket_10_scopes_linkage_wave70.RUNTIME_PROBES +
    bucket_11_functions_calls.RUNTIME_PROBES +
    bucket_11_functions_calls_wave83.RUNTIME_PROBES +
    bucket_11_functions_calls_wave84.RUNTIME_PROBES +
    bucket_11_functions_calls_wave85.RUNTIME_PROBES +
    bucket_11_functions_calls_wave86.RUNTIME_PROBES +
    bucket_11_functions_calls_wave87.RUNTIME_PROBES +
    bucket_11_functions_calls_wave88.RUNTIME_PROBES +
    bucket_11_functions_calls_wave89.RUNTIME_PROBES +
    bucket_11_functions_calls_wave90.RUNTIME_PROBES +
    bucket_11_functions_calls_wave91.RUNTIME_PROBES +
    bucket_11_functions_calls_wave92.RUNTIME_PROBES +
    bucket_13_codegen_ir.RUNTIME_PROBES +
    bucket_13_codegen_ir_wave57.RUNTIME_PROBES +
    bucket_13_codegen_ir_wave58.RUNTIME_PROBES +
    bucket_13_codegen_ir_wave59.RUNTIME_PROBES +
    bucket_13_codegen_ir_wave61.RUNTIME_PROBES +
    bucket_13_codegen_ir_wave62.RUNTIME_PROBES +
    bucket_13_codegen_ir_wave63.RUNTIME_PROBES +
    bucket_13_codegen_ir_wave64.RUNTIME_PROBES +
    bucket_13_codegen_ir_wave66.RUNTIME_PROBES +
    bucket_13_codegen_ir_wave67.RUNTIME_PROBES +
    bucket_13_codegen_ir_wave68.RUNTIME_PROBES +
    bucket_14_runtime_surface.RUNTIME_PROBES +
    bucket_15_torture_differential.RUNTIME_PROBES +
    bucket_05_expressions.RUNTIME_PROBES +
    bucket_05_expressions_wave28.RUNTIME_PROBES +
    bucket_05_expressions_wave29.RUNTIME_PROBES +
    bucket_05_expressions_wave30.RUNTIME_PROBES +
    bucket_05_expressions_wave31.RUNTIME_PROBES +
    bucket_05_expressions_wave32.RUNTIME_PROBES +
    bucket_05_expressions_wave34.RUNTIME_PROBES +
    bucket_05_expressions_wave35.RUNTIME_PROBES +
    bucket_09_statements_controlflow.RUNTIME_PROBES +
    bucket_09_statements_controlflow_wave84.RUNTIME_PROBES +
    bucket_09_statements_controlflow_wave85.RUNTIME_PROBES +
    bucket_09_statements_controlflow_wave86.RUNTIME_PROBES +
    bucket_09_statements_controlflow_wave87.RUNTIME_PROBES +
    bucket_09_statements_controlflow_wave88.RUNTIME_PROBES +
    bucket_12_diagnostics_recovery.RUNTIME_PROBES +
    bucket_12_diagnostics_recovery_wave42.RUNTIME_PROBES
    + bucket_12_diagnostics_recovery_wave43.RUNTIME_PROBES
    + bucket_12_diagnostics_recovery_wave44.RUNTIME_PROBES
    + bucket_12_diagnostics_recovery_wave45.RUNTIME_PROBES
)

OBJECT_PROBES = bucket_15_torture_differential.OBJECT_PROBES

DIAG_PROBES = (
    bucket_01_translation_phases.DIAG_PROBES +
    bucket_02_lexer.DIAG_PROBES +
    bucket_03_preprocessor.DIAG_PROBES +
    bucket_03_preprocessor_wave39.DIAG_PROBES +
    bucket_03_preprocessor_wave41.DIAG_PROBES +
    bucket_04_declarations.DIAG_PROBES +
    bucket_04_declarations_wave40.DIAG_PROBES +
    bucket_04_declarations_wave41.DIAG_PROBES +
    bucket_04_declarations_wave42.DIAG_PROBES +
    bucket_04_declarations_wave43.DIAG_PROBES +
    bucket_04_declarations_wave44.DIAG_PROBES +
    bucket_04_declarations_wave45.DIAG_PROBES +
    bucket_04_declarations_wave46.DIAG_PROBES +
    bucket_04_declarations_wave48.DIAG_PROBES +
    bucket_04_declarations_wave49.DIAG_PROBES +
    bucket_04_declarations_wave50.DIAG_PROBES +
    bucket_05_expressions.DIAG_PROBES +
    bucket_05_expressions_wave28.DIAG_PROBES +
    bucket_05_expressions_wave29.DIAG_PROBES +
    bucket_05_expressions_wave30.DIAG_PROBES +
    bucket_05_expressions_wave31.DIAG_PROBES +
    bucket_05_expressions_wave32.DIAG_PROBES +
    bucket_05_expressions_wave34.DIAG_PROBES +
    bucket_05_expressions_wave35.DIAG_PROBES +
    bucket_06_lvalues_rvalues.DIAG_PROBES +
    bucket_06_lvalues_rvalues_wave26.DIAG_PROBES +
    bucket_06_lvalues_rvalues_wave27.DIAG_PROBES +
    bucket_06_lvalues_rvalues_wave28.DIAG_PROBES +
    bucket_07_types_conversions.DIAG_PROBES +
    bucket_07_types_conversions_wave87.DIAG_PROBES +
    bucket_07_types_conversions_wave88.DIAG_PROBES +
    bucket_07_types_conversions_wave86.DIAG_PROBES +
    bucket_07_types_conversions_wave89.DIAG_PROBES +
    bucket_07_types_conversions_wave90.DIAG_PROBES +
    bucket_08_initializers_layout.DIAG_PROBES +
    bucket_08_initializers_layout_wave86.DIAG_PROBES +
    bucket_08_initializers_layout_wave87.DIAG_PROBES +
    bucket_08_initializers_layout_wave88.DIAG_PROBES +
    bucket_08_initializers_layout_wave89.DIAG_PROBES +
    bucket_09_statements_controlflow.DIAG_PROBES +
    bucket_09_statements_controlflow_wave84.DIAG_PROBES +
    bucket_09_statements_controlflow_wave86.DIAG_PROBES +
    bucket_09_statements_controlflow_wave87.DIAG_PROBES +
    bucket_09_statements_controlflow_wave88.DIAG_PROBES +
    bucket_10_scopes_linkage.DIAG_PROBES +
    bucket_10_scopes_linkage_wave67.DIAG_PROBES +
    bucket_10_scopes_linkage_wave69.DIAG_PROBES +
    bucket_10_scopes_linkage_wave70.DIAG_PROBES +
    bucket_11_functions_calls.DIAG_PROBES +
    bucket_11_functions_calls_wave83.DIAG_PROBES +
    bucket_11_functions_calls_wave84.DIAG_PROBES +
    bucket_11_functions_calls_wave85.DIAG_PROBES +
    bucket_11_functions_calls_wave86.DIAG_PROBES +
    bucket_11_functions_calls_wave87.DIAG_PROBES +
    bucket_11_functions_calls_wave88.DIAG_PROBES +
    bucket_11_functions_calls_wave90.DIAG_PROBES +
    bucket_11_functions_calls_wave92.DIAG_PROBES +
    bucket_12_diagnostics_recovery.DIAG_PROBES +
    bucket_12_diagnostics_recovery_wave39_identity.DIAG_PROBES +
    bucket_12_diagnostics_recovery_wave42.DIAG_PROBES +
    bucket_12_diagnostics_recovery_wave43.DIAG_PROBES +
    bucket_12_diagnostics_recovery_wave44.DIAG_PROBES +
    bucket_12_diagnostics_recovery_wave45.DIAG_PROBES +
    bucket_13_codegen_ir.DIAG_PROBES +
    bucket_13_codegen_ir_wave57.DIAG_PROBES +
    bucket_13_codegen_ir_wave58.DIAG_PROBES +
    bucket_13_codegen_ir_wave59.DIAG_PROBES +
    bucket_13_codegen_ir_wave61.DIAG_PROBES +
    bucket_13_codegen_ir_wave62.DIAG_PROBES +
    bucket_13_codegen_ir_wave63.DIAG_PROBES +
    bucket_13_codegen_ir_wave64.DIAG_PROBES +
    bucket_13_codegen_ir_wave65.DIAG_PROBES +
    bucket_13_codegen_ir_wave66.DIAG_PROBES +
    bucket_13_codegen_ir_wave67.DIAG_PROBES +
    bucket_13_codegen_ir_wave68.DIAG_PROBES +
    bucket_14_runtime_surface.DIAG_PROBES +
    bucket_15_torture_differential.DIAG_PROBES
)

DIAG_JSON_PROBES = (
    bucket_01_translation_phases.DIAG_JSON_PROBES +
    bucket_02_lexer.DIAG_JSON_PROBES +
    bucket_03_preprocessor.DIAG_JSON_PROBES +
    bucket_03_preprocessor_wave39.DIAG_JSON_PROBES +
    bucket_03_preprocessor_wave41.DIAG_JSON_PROBES +
    bucket_04_declarations.DIAG_JSON_PROBES +
    bucket_04_declarations_wave40.DIAG_JSON_PROBES +
    bucket_04_declarations_wave41.DIAG_JSON_PROBES +
    bucket_04_declarations_wave42.DIAG_JSON_PROBES +
    bucket_04_declarations_wave43.DIAG_JSON_PROBES +
    bucket_04_declarations_wave44.DIAG_JSON_PROBES +
    bucket_04_declarations_wave45.DIAG_JSON_PROBES +
    bucket_04_declarations_wave46.DIAG_JSON_PROBES +
    bucket_04_declarations_wave48.DIAG_JSON_PROBES +
    bucket_04_declarations_wave49.DIAG_JSON_PROBES +
    bucket_05_expressions.DIAG_JSON_PROBES +
    bucket_05_expressions_wave28.DIAG_JSON_PROBES +
    bucket_05_expressions_wave29.DIAG_JSON_PROBES +
    bucket_05_expressions_wave30.DIAG_JSON_PROBES +
    bucket_05_expressions_wave31.DIAG_JSON_PROBES +
    bucket_05_expressions_wave32.DIAG_JSON_PROBES +
    bucket_05_expressions_wave34.DIAG_JSON_PROBES +
    bucket_05_expressions_wave35.DIAG_JSON_PROBES +
    bucket_06_lvalues_rvalues.DIAG_JSON_PROBES +
    bucket_06_lvalues_rvalues_wave26.DIAG_JSON_PROBES +
    bucket_07_types_conversions.DIAG_JSON_PROBES +
    bucket_07_types_conversions_wave87.DIAG_JSON_PROBES +
    bucket_07_types_conversions_wave88.DIAG_JSON_PROBES +
    bucket_07_types_conversions_wave86.DIAG_JSON_PROBES +
    bucket_07_types_conversions_wave89.DIAG_JSON_PROBES +
    bucket_08_initializers_layout.DIAG_JSON_PROBES +
    bucket_08_initializers_layout_wave86.DIAG_JSON_PROBES +
    bucket_08_initializers_layout_wave87.DIAG_JSON_PROBES +
    bucket_08_initializers_layout_wave88.DIAG_JSON_PROBES +
    bucket_08_initializers_layout_wave89.DIAG_JSON_PROBES +
    bucket_09_statements_controlflow.DIAG_JSON_PROBES +
    bucket_09_statements_controlflow_wave84.DIAG_JSON_PROBES +
    bucket_09_statements_controlflow_wave86.DIAG_JSON_PROBES +
    bucket_09_statements_controlflow_wave87.DIAG_JSON_PROBES +
    bucket_09_statements_controlflow_wave88.DIAG_JSON_PROBES +
    bucket_10_scopes_linkage.DIAG_JSON_PROBES +
    bucket_10_scopes_linkage_wave67.DIAG_JSON_PROBES +
    bucket_10_scopes_linkage_wave69.DIAG_JSON_PROBES +
    bucket_10_scopes_linkage_wave70.DIAG_JSON_PROBES +
    bucket_11_functions_calls.DIAG_JSON_PROBES +
    bucket_11_functions_calls_wave83.DIAG_JSON_PROBES +
    bucket_11_functions_calls_wave84.DIAG_JSON_PROBES +
    bucket_11_functions_calls_wave85.DIAG_JSON_PROBES +
    bucket_11_functions_calls_wave86.DIAG_JSON_PROBES +
    bucket_11_functions_calls_wave87.DIAG_JSON_PROBES +
    bucket_11_functions_calls_wave88.DIAG_JSON_PROBES +
    bucket_11_functions_calls_wave90.DIAG_JSON_PROBES +
    bucket_11_functions_calls_wave92.DIAG_JSON_PROBES +
    bucket_12_diagnostics_recovery.DIAG_JSON_PROBES +
    bucket_12_diagnostics_recovery_wave39_identity.DIAG_JSON_PROBES +
    bucket_12_diagnostics_recovery_wave42.DIAG_JSON_PROBES +
    bucket_12_diagnostics_recovery_wave43.DIAG_JSON_PROBES +
    bucket_12_diagnostics_recovery_wave44.DIAG_JSON_PROBES +
    bucket_12_diagnostics_recovery_wave45.DIAG_JSON_PROBES +
    bucket_13_codegen_ir.DIAG_JSON_PROBES +
    bucket_13_codegen_ir_wave57.DIAG_JSON_PROBES +
    bucket_13_codegen_ir_wave58.DIAG_JSON_PROBES +
    bucket_13_codegen_ir_wave59.DIAG_JSON_PROBES +
    bucket_13_codegen_ir_wave61.DIAG_JSON_PROBES +
    bucket_13_codegen_ir_wave62.DIAG_JSON_PROBES +
    bucket_13_codegen_ir_wave63.DIAG_JSON_PROBES +
    bucket_13_codegen_ir_wave64.DIAG_JSON_PROBES +
    bucket_13_codegen_ir_wave66.DIAG_JSON_PROBES +
    bucket_13_codegen_ir_wave67.DIAG_JSON_PROBES +
    bucket_13_codegen_ir_wave68.DIAG_JSON_PROBES +
    bucket_14_runtime_surface.DIAG_JSON_PROBES +
    bucket_15_torture_differential.DIAG_JSON_PROBES
)


def _apply_promotion_closure_owners():
    probes_by_id = {
        probe.probe_id: probe
        for probe in (*RUNTIME_PROBES, *DIAG_PROBES, *DIAG_JSON_PROBES)
    }
    missing = sorted(set(PROMOTION_CLOSURE_OWNERS) - set(probes_by_id))
    if missing:
        raise RuntimeError(
            "promotion closure owner references unknown probe IDs: "
            + ", ".join(missing)
        )
    for probe_id, test_id in PROMOTION_CLOSURE_OWNERS.items():
        probe = probes_by_id[probe_id]
        if probe.promoted_test_id not in (None, test_id):
            raise RuntimeError(
                f"conflicting promotion owners for {probe_id}: "
                f"{probe.promoted_test_id} vs {test_id}"
            )
        probe.promoted_test_id = test_id


_apply_promotion_closure_owners()
