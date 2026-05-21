from . import axis_runtime, diagnostics, diagjson, late_multitu_runtime, multitu_runtime, runtime_core


RUNTIME_PROBES = (
    runtime_core.RUNTIME_PROBES +
    multitu_runtime.RUNTIME_PROBES +
    late_multitu_runtime.RUNTIME_PROBES +
    axis_runtime.RUNTIME_PROBES
)

DIAG_PROBES = diagnostics.DIAG_PROBES

DIAG_JSON_PROBES = diagjson.DIAG_JSON_PROBES
