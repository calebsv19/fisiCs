from pathlib import Path

from lib.models import ObjectProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent
HARDWARE_BLIND_FORBIDDEN = (
    "cli",
    "sti",
    "hlt",
    "in",
    "ins",
    "out",
    "outs",
    "rdmsr",
    "wrmsr",
    "invlpg",
    "lidt",
    "lgdt",
)


def object_probe(
    probe_id,
    source_name,
    export,
    note,
    *,
    allowed_undefined=(),
    clang_allowed_undefined=None,
    allowed_relocations=("R_X86_64_PC32",),
    clang_allowed_relocations=None,
    scalar_sse2=False,
):
    return ObjectProbe(
        probe_id=probe_id,
        source=PROBE_DIR / f"runtime/{source_name}",
        note=note,
        required_exports=(export,),
        allowed_undefined=allowed_undefined,
        clang_allowed_undefined=clang_allowed_undefined,
        allowed_relocations=allowed_relocations,
        clang_allowed_relocations=clang_allowed_relocations,
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
        scalar_sse2=scalar_sse2,
    )


OBJECT_PROBES = [
    object_probe(
        "15__probe_osp3_object_admission_many_args",
        "15__probe_osp3_object_admission_many_args.c",
        "osp3_object_admission_many_args",
        "OS-P3 object wave: eighteen-argument admission policy must remain deterministic and helper-free",
    ),
    object_probe(
        "15__probe_osp3_object_small_zero_strict",
        "15__probe_osp3_object_small_zero_strict.c",
        "osp3_object_small_zero_strict",
        "OS-P3 object wave strict reduction: four-lane aggregate zero initialization must remain helper-free",
        clang_allowed_undefined=("memset",),
        clang_allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
    ),
    object_probe(
        "15__probe_osp3_object_large_zero_strict",
        "15__probe_osp3_object_large_zero_strict.c",
        "osp3_object_large_zero_strict",
        "OS-P3 object wave strict: 512-byte aggregate zero initialization must remain helper-free",
        clang_allowed_undefined=("memset",),
        clang_allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
    ),
    object_probe(
        "15__probe_osp3_object_one_word_zero_minimized",
        "15__probe_osp3_object_one_word_zero_minimized.c",
        "osp3_object_one_word_zero_minimized",
        "OS-P3 object wave minimized strict: one-word aggregate zero initializer must remain helper-free",
        clang_allowed_undefined=("memset",),
        clang_allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
    ),
    object_probe(
        "15__probe_osp3_object_small_copy_reduced",
        "15__probe_osp3_object_small_copy_reduced.c",
        "osp3_object_small_copy_reduced",
        "OS-P3 object wave reduced threshold: two-lane aggregate assignment must remain helper-free",
    ),
    object_probe(
        "15__probe_osp3_object_large_copy_strict",
        "15__probe_osp3_object_large_copy_strict.c",
        "osp3_object_large_copy_strict",
        "OS-P3 object wave strict: 512-byte aggregate assignment must remain helper-free",
        clang_allowed_undefined=("memcpy",),
        clang_allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
    ),
    object_probe(
        "15__probe_osp3_object_callback_external",
        "15__probe_osp3_object_callback_external.c",
        "osp3_object_callback_external",
        "OS-P3 object wave: indirect callback plus one declared external policy must expose only its explicit dependency",
        allowed_undefined=("osp3_external_policy",),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
    ),
    object_probe(
        "15__probe_osp3_object_switch_transition",
        "15__probe_osp3_object_switch_transition.c",
        "osp3_object_switch_transition",
        "OS-P3 object wave: sparse transition switch must remain deterministic and helper-free",
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_32S"),
    ),
    object_probe(
        "15__probe_osp3_object_scalar_double",
        "15__probe_osp3_object_scalar_double.c",
        "osp3_object_scalar_double",
        "OS-P3 object wave: scalar-double SysV surface must remain deterministic and helper-free with SSE2 enabled",
        scalar_sse2=True,
    ),
    object_probe(
        "15__probe_osp3_object_manual_zero_current",
        "15__probe_osp3_object_manual_zero_current.c",
        "osp3_object_manual_zero_current",
        "OS-P3 object wave current threshold: explicit field stores provide helper-free aggregate clearing",
    ),
    object_probe(
        "15__probe_osp3_object_nested_struct_return",
        "15__probe_osp3_object_nested_struct_return.c",
        "osp3_object_nested_struct_return",
        "OS-P3 object wave: nested aggregate return must preserve deterministic helper-free sret lowering",
    ),
    object_probe(
        "15__probe_osp3_object_pair_return_reduced",
        "15__probe_osp3_object_pair_return_reduced.c",
        "osp3_object_pair_return_reduced",
        "OS-P3 object wave reduced threshold: two-word aggregate return must remain helper-free",
    ),
    object_probe(
        "15__probe_osp3_object_pointer_window",
        "15__probe_osp3_object_pointer_window.c",
        "osp3_object_pointer_window",
        "OS-P3 object wave: bounded pointer-window reduction must remain deterministic and helper-free",
    ),
]

RUNTIME_PROBES = []
DIAG_PROBES = []
DIAG_JSON_PROBES = []
