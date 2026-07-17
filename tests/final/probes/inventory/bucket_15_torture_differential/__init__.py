from . import abi_payload_runtime, axis1_runtime, axis3_runtime, axis5_late_runtime, axis5_runtime, axis6_runtime, axis7_metamorphic_runtime, corpus_runtime, diagnostics, diagjson, multitu_runtime, physics_units, runtime_core, wave138_realproj_multitu


RUNTIME_PROBES = (
    runtime_core.RUNTIME_PROBES +
    multitu_runtime.RUNTIME_PROBES +
    corpus_runtime.RUNTIME_PROBES +
    axis1_runtime.RUNTIME_PROBES +
    axis3_runtime.RUNTIME_PROBES +
    axis5_runtime.RUNTIME_PROBES +
    axis5_late_runtime.RUNTIME_PROBES +
    axis6_runtime.RUNTIME_PROBES +
    axis7_metamorphic_runtime.RUNTIME_PROBES +
    physics_units.RUNTIME_PROBES +
    abi_payload_runtime.RUNTIME_PROBES +
    wave138_realproj_multitu.RUNTIME_PROBES
)

DIAG_PROBES = diagnostics.DIAG_PROBES + physics_units.DIAG_PROBES

DIAG_JSON_PROBES = diagjson.DIAG_JSON_PROBES + physics_units.DIAG_JSON_PROBES
