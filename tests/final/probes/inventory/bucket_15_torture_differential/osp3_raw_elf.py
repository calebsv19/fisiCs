from pathlib import Path

from lib.models import ObjectProbe, RuntimeProbe

from .osp3_object import HARDWARE_BLIND_FORBIDDEN


PROBE_DIR = Path(__file__).resolve().parent.parent.parent
POLICY_SOURCE = PROBE_DIR / "runtime/15__probe_osp3_raw_elf_policy.c"
MATRIX_SOURCE = PROBE_DIR / "runtime/15__probe_osp3_raw_elf_matrix.c"

MODE_OUTPUTS = (
    (
        "valid_topology",
        0,
        "OSP3 raw-elf mode=0 seed=41c6ce57 budget=256 cases=8 "
        "accept=8 reject=0 failures=0 digest=1062203414\n",
    ),
    (
        "header_truncation",
        1,
        "OSP3 raw-elf mode=1 seed=41c6ce57 budget=256 cases=616 "
        "accept=25 reject=591 failures=0 digest=3280981654\n",
    ),
    (
        "segment_overlap",
        2,
        "OSP3 raw-elf mode=2 seed=41c6ce57 budget=256 cases=6 "
        "accept=3 reject=3 failures=0 digest=4254225686\n",
    ),
    (
        "entry_membership",
        3,
        "OSP3 raw-elf mode=3 seed=41c6ce57 budget=256 cases=9 "
        "accept=3 reject=6 failures=0 digest=2002156252\n",
    ),
    (
        "state_reset",
        4,
        "OSP3 raw-elf mode=4 seed=41c6ce57 budget=256 cases=64 "
        "accept=32 reject=32 failures=0 digest=4119337039\n",
    ),
    (
        "overflow_geometry",
        5,
        "OSP3 raw-elf mode=5 seed=41c6ce57 budget=256 cases=10 "
        "accept=0 reject=10 failures=0 digest=3369564033\n",
    ),
    (
        "mutation_base",
        6,
        "OSP3 raw-elf mode=6 seed=41c6ce57 budget=256 cases=256 "
        "accept=43 reject=213 failures=0 digest=363567582\n",
    ),
)


def raw_runtime_probe(name, mode, expected_stdout):
    defines = (
        f"-DOSP3_RAW_MODE={mode}",
        "-DOSP3_RAW_SEED=0x41c6ce57u",
        "-DOSP3_RAW_CASE_BUDGET=256u",
    )
    return RuntimeProbe(
        probe_id=f"15__probe_osp3_raw_elf_{name}",
        source=MATRIX_SOURCE,
        inputs=(POLICY_SOURCE, MATRIX_SOURCE),
        note=(
            f"OS-P3 raw-image ELF mode {mode}: {name.replace('_', ' ')} "
            "must match the pinned fail-closed loader-policy transcript"
        ),
        fisics_args=defines,
        clang_args=defines,
        expected_exit_code=0,
        expected_stdout=expected_stdout,
        expected_stderr="",
    )


RUNTIME_PROBES = [
    raw_runtime_probe(name, mode, output)
    for name, mode, output in MODE_OUTPUTS
] + [
    RuntimeProbe(
        probe_id="15__probe_osp3_raw_elf_mutation_alt",
        source=MATRIX_SOURCE,
        inputs=(POLICY_SOURCE, MATRIX_SOURCE),
        note=(
            "OS-P3 raw-image ELF alternate mutation stream: 1,024 bounded "
            "raw-byte/header/program-header mutations"
        ),
        fisics_args=(
            "-DOSP3_RAW_MODE=6",
            "-DOSP3_RAW_SEED=0xa5a5a5a5u",
            "-DOSP3_RAW_CASE_BUDGET=1024u",
        ),
        clang_args=(
            "-DOSP3_RAW_MODE=6",
            "-DOSP3_RAW_SEED=0xa5a5a5a5u",
            "-DOSP3_RAW_CASE_BUDGET=1024u",
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OSP3 raw-elf mode=6 seed=a5a5a5a5 budget=1024 cases=1024 "
            "accept=143 reject=881 failures=0 digest=437599798\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_osp3_raw_elf_mutation_stress_sanitized",
        source=MATRIX_SOURCE,
        inputs=(POLICY_SOURCE, MATRIX_SOURCE),
        note=(
            "OS-P3 raw-image ELF stress stream: 4,096 bounded mutations with "
            "the Clang reference instrumented by ASan+UBSan"
        ),
        fisics_args=(
            "-DOSP3_RAW_MODE=6",
            "-DOSP3_RAW_SEED=0xffffffffu",
            "-DOSP3_RAW_CASE_BUDGET=4096u",
        ),
        clang_args=(
            "-DOSP3_RAW_MODE=6",
            "-DOSP3_RAW_SEED=0xffffffffu",
            "-DOSP3_RAW_CASE_BUDGET=4096u",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OSP3 raw-elf mode=6 seed=ffffffff budget=4096 cases=4096 "
            "accept=609 reject=3487 failures=0 digest=1829243853\n"
        ),
        expected_stderr="",
    ),
]

OBJECT_PROBES = [
    ObjectProbe(
        probe_id="15__probe_osp3_raw_elf_policy_object",
        source=POLICY_SOURCE,
        note=(
            "OS-P3 raw-image ELF policy object must be deterministic, "
            "freestanding, helper-free, hardware-blind, and no-red-zone"
        ),
        required_exports=("osp3_raw_elf_admit",),
        allowed_relocations=("R_X86_64_PC32",),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    )
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
