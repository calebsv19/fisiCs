from . import bucket_04_declarations
from . import bucket_07_types_conversions
from . import bucket_10_scopes_linkage
from . import bucket_11_functions_calls
from . import bucket_13_codegen_ir
from . import bucket_14_runtime_surface
from . import bucket_15_torture_differential
from . import bucket_05_expressions
from . import bucket_09_statements_controlflow
from . import bucket_01_translation_phases
from . import bucket_02_lexer
from . import bucket_03_preprocessor
from . import bucket_06_lvalues_rvalues
from . import bucket_08_initializers_layout
from . import bucket_12_diagnostics_recovery


RUNTIME_PROBES = (
    bucket_04_declarations.RUNTIME_PROBES +
    bucket_07_types_conversions.RUNTIME_PROBES +
    bucket_10_scopes_linkage.RUNTIME_PROBES +
    bucket_11_functions_calls.RUNTIME_PROBES +
    bucket_13_codegen_ir.RUNTIME_PROBES +
    bucket_14_runtime_surface.RUNTIME_PROBES +
    bucket_15_torture_differential.RUNTIME_PROBES +
    bucket_05_expressions.RUNTIME_PROBES +
    bucket_09_statements_controlflow.RUNTIME_PROBES
)

DIAG_PROBES = (
    bucket_01_translation_phases.DIAG_PROBES +
    bucket_02_lexer.DIAG_PROBES +
    bucket_03_preprocessor.DIAG_PROBES +
    bucket_04_declarations.DIAG_PROBES +
    bucket_05_expressions.DIAG_PROBES +
    bucket_06_lvalues_rvalues.DIAG_PROBES +
    bucket_07_types_conversions.DIAG_PROBES +
    bucket_08_initializers_layout.DIAG_PROBES +
    bucket_09_statements_controlflow.DIAG_PROBES +
    bucket_10_scopes_linkage.DIAG_PROBES +
    bucket_11_functions_calls.DIAG_PROBES +
    bucket_12_diagnostics_recovery.DIAG_PROBES +
    bucket_13_codegen_ir.DIAG_PROBES +
    bucket_14_runtime_surface.DIAG_PROBES +
    bucket_15_torture_differential.DIAG_PROBES
)

DIAG_JSON_PROBES = (
    bucket_01_translation_phases.DIAG_JSON_PROBES +
    bucket_02_lexer.DIAG_JSON_PROBES +
    bucket_03_preprocessor.DIAG_JSON_PROBES +
    bucket_05_expressions.DIAG_JSON_PROBES +
    bucket_06_lvalues_rvalues.DIAG_JSON_PROBES +
    bucket_07_types_conversions.DIAG_JSON_PROBES +
    bucket_08_initializers_layout.DIAG_JSON_PROBES +
    bucket_09_statements_controlflow.DIAG_JSON_PROBES +
    bucket_10_scopes_linkage.DIAG_JSON_PROBES +
    bucket_11_functions_calls.DIAG_JSON_PROBES +
    bucket_12_diagnostics_recovery.DIAG_JSON_PROBES +
    bucket_13_codegen_ir.DIAG_JSON_PROBES +
    bucket_14_runtime_surface.DIAG_JSON_PROBES +
    bucket_15_torture_differential.DIAG_JSON_PROBES
)
