from pathlib import Path

from lib.models import ObjectProbe

from .osp3_object import HARDWARE_BLIND_FORBIDDEN


PROBE_DIR = Path(__file__).resolve().parent.parent.parent


OBJECT_PROBES = [
    ObjectProbe(
        probe_id="15__probe_p4_object_policy_rodata_bss",
        source=PROBE_DIR / "runtime/15__probe_p4_object_policy_rodata_bss.c",
        note=(
            "P4 accepted-object corpus: read-only lookup data and zero-initialized "
            "writable storage remain separately mapped, self-contained, and "
            "PC-relative under the freestanding object policy"
        ),
        required_exports=("p4_object_policy_rodata_bss_entry",),
        allowed_undefined=(),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        clang_allowed_relocations=(
            "R_X86_64_PC32",
            "R_X86_64_PLT32",
            "R_X86_64_32S",
        ),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
        required_section_flags={
            ".text": ("SHF_ALLOC", "SHF_EXECINSTR"),
            ".bss": ("SHF_ALLOC", "SHF_WRITE"),
        },
        required_section_prefix_flags={
            ".rodata": ("SHF_ALLOC",),
        },
    ),
    ObjectProbe(
        probe_id="15__probe_p4_object_policy_internal_calls",
        source=PROBE_DIR / "runtime/15__probe_p4_object_policy_internal_calls.c",
        note=(
            "P4 accepted-object corpus: internal helper calls must remain "
            "self-contained and use only the policy-approved PC-relative "
            "relocation forms"
        ),
        required_exports=("p4_object_policy_internal_call_entry",),
        allowed_undefined=(),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
        required_section_flags={
            ".text": ("SHF_ALLOC", "SHF_EXECINSTR"),
        },
    ),
    ObjectProbe(
        probe_id="15__probe_p4_object_policy_undefined_external_rejected",
        source=PROBE_DIR / "runtime/15__probe_p4_object_policy_undefined_external.c",
        note=(
            "P4 rejection corpus: a compiler-produced object with an unresolved "
            "external call is a valid emission result but must be rejected by the "
            "self-contained EDU-50 object policy before admission"
        ),
        required_exports=("p4_object_policy_external_entry",),
        allowed_undefined=(),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
        required_section_flags={
            ".text": ("SHF_ALLOC", "SHF_EXECINSTR"),
        },
        expected_policy_rejection="undefined symbol contract mismatch",
    ),
    ObjectProbe(
        probe_id="15__probe_edu50_frozen_program_two_object_receipt",
        source=PROBE_DIR / "runtime/15__probe_edu50_frozen_simulation_kernel.c",
        note=(
            "EDU-50 receipt intake: byte-exact simulation_kernel.c extracted "
            "from signed tag edu-50-bounded-relocatable-program-load-plan at "
            "b95261ac24d19148ce0dce0dc713846c1ea48212; candidate objects must "
            "remain self-contained, freestanding, no-red-zone, W^X-separated, "
            "and limited to the frozen load-plan relocation kinds"
        ),
        required_exports=(
            "edu12_reduce_result",
            "edu12_simulate_partition",
            "edu32_workload_valid",
            "edu42_program_result",
            "edu42_program_valid",
            "edu42_simulate_damped_partition",
        ),
        allowed_undefined=(),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
        scalar_sse2=True,
        required_section_flags={
            ".text": ("SHF_ALLOC", "SHF_EXECINSTR"),
        },
        expected_source_sha256=(
            "f55ec92ffc2e7b87da945f8b919d94b3640d45b7336bd8c54719654e29e2af5d"
        ),
        expected_fisics_object_sha256=(
            "eddff2b80d15fd58895d85ddb6997f7b373b28e9dd4e69c8dc5e3ee9d9a1157a"
        ),
    ),
    ObjectProbe(
        probe_id="15__probe_p4_object_policy_load_plan",
        source=PROBE_DIR / "runtime/15__probe_p4_object_policy_load_plan.c",
        note=(
            "P4 intake synthetic load-plan policy object must be deterministic, "
            "self-contained, freestanding, no-red-zone, W^X-separated, and "
            "limited to PC-relative x86-64 relocations before any EDU-50 source "
            "snapshot is admitted"
        ),
        required_exports=("p4_load_plan_policy_admit",),
        allowed_undefined=(),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        clang_allowed_relocations=(
            "R_X86_64_PC32",
            "R_X86_64_PLT32",
            "R_X86_64_32S",
        ),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
        required_section_flags={
            ".text": ("SHF_ALLOC", "SHF_EXECINSTR"),
            ".data": ("SHF_ALLOC", "SHF_WRITE"),
        },
    ),
    ObjectProbe(
        probe_id="15__probe_p4_object_policy_load_plan_current_data_reloc",
        source=PROBE_DIR / "runtime/15__probe_p4_object_policy_load_plan.c",
        note=(
            "P4 current-threshold companion: the same synthetic policy object "
            "keeps its W^X section separation and self-contained symbol set when "
            "the compiler's local writable-data relocation is admitted"
        ),
        required_exports=("p4_load_plan_policy_admit",),
        allowed_undefined=(),
        allowed_relocations=(
            "R_X86_64_PC32",
            "R_X86_64_PLT32",
            "R_X86_64_32S",
        ),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
        required_section_flags={
            ".text": ("SHF_ALLOC", "SHF_EXECINSTR"),
            ".data": ("SHF_ALLOC", "SHF_WRITE"),
        },
    ),
]

RUNTIME_PROBES = []
DIAG_PROBES = []
DIAG_JSON_PROBES = []
