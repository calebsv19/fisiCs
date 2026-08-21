from . import abi_payload_runtime, axis1_runtime, axis3_runtime, axis5_late_runtime, axis5_runtime, axis6_runtime, axis7_metamorphic_runtime, axis8_compound_literal_runtime, axis9_complex_runtime, axis10_macro_linkage_runtime, axis11_uint64_runtime, axis12_static_inline_runtime, axis13_va_copy_runtime, axis14_vla_sizeof_runtime, axis15_flexible_array_runtime, axis16_volatile_runtime, axis17_restrict_runtime, axis18_signed_division_runtime, axis19_subobject_pointer_runtime, axis20_static_storage_runtime, corpus_runtime, diagnostics, diagjson, multitu_runtime, os_post_edu19, osp3_expanded, osp3_object, osp3_policy_runtime, osp3_raw_elf, osp3_raw_job, osp3_raw_storage, p4_object_policy, physics_units, runtime_core, wave138_realproj_multitu


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
    axis8_compound_literal_runtime.RUNTIME_PROBES +
    axis9_complex_runtime.RUNTIME_PROBES +
    axis10_macro_linkage_runtime.RUNTIME_PROBES +
    axis11_uint64_runtime.RUNTIME_PROBES +
    axis12_static_inline_runtime.RUNTIME_PROBES +
    axis13_va_copy_runtime.RUNTIME_PROBES +
    axis14_vla_sizeof_runtime.RUNTIME_PROBES +
    axis15_flexible_array_runtime.RUNTIME_PROBES +
    axis16_volatile_runtime.RUNTIME_PROBES +
    axis17_restrict_runtime.RUNTIME_PROBES +
    axis18_signed_division_runtime.RUNTIME_PROBES +
    axis19_subobject_pointer_runtime.RUNTIME_PROBES +
    axis20_static_storage_runtime.RUNTIME_PROBES +
    osp3_policy_runtime.RUNTIME_PROBES +
    osp3_expanded.RUNTIME_PROBES +
    osp3_raw_elf.RUNTIME_PROBES +
    osp3_raw_job.RUNTIME_PROBES +
    osp3_raw_storage.RUNTIME_PROBES +
    os_post_edu19.RUNTIME_PROBES +
    physics_units.RUNTIME_PROBES +
    abi_payload_runtime.RUNTIME_PROBES +
    wave138_realproj_multitu.RUNTIME_PROBES
)

OBJECT_PROBES = (
    osp3_object.OBJECT_PROBES +
    osp3_raw_elf.OBJECT_PROBES +
    osp3_raw_job.OBJECT_PROBES
    + osp3_raw_storage.OBJECT_PROBES
    + os_post_edu19.OBJECT_PROBES
    + p4_object_policy.OBJECT_PROBES
)

DIAG_PROBES = diagnostics.DIAG_PROBES + physics_units.DIAG_PROBES

DIAG_JSON_PROBES = diagjson.DIAG_JSON_PROBES + physics_units.DIAG_JSON_PROBES
