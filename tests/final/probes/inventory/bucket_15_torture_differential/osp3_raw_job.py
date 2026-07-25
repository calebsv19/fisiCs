from pathlib import Path

from lib.models import ObjectProbe, RuntimeProbe

from .osp3_object import HARDWARE_BLIND_FORBIDDEN


PROBE_DIR = Path(__file__).resolve().parent.parent.parent
POLICY_SOURCE = PROBE_DIR / "runtime/15__probe_osp3_raw_job_policy.c"
MATRIX_SOURCE = PROBE_DIR / "runtime/15__probe_osp3_raw_job_matrix.c"

MODE_OUTPUTS = (
    ("valid_packages", 0, 16, 16, 0, 1062896933),
    ("truncation_geometry", 1, 257, 1, 256, 3323744702),
    ("rejection_precedence", 2, 6, 0, 6, 1358008951),
    ("authority_replay", 3, 6, 1, 5, 2277227818),
    ("step_ranges_overlap", 4, 6, 0, 6, 1565904323),
    ("late_reject_reset", 5, 64, 32, 32, 1506419909),
    ("mutation_base", 6, 256, 44, 212, 2460613671),
)


def raw_job_probe(name, mode, cases, accepted, rejected, digest):
    defines = (
        f"-DOSP3_JOB_MODE={mode}",
        "-DOSP3_JOB_SEED=0x7f4a7c15U",
        "-DOSP3_JOB_CASE_BUDGET=256U",
    )
    return RuntimeProbe(
        probe_id=f"15__probe_osp3_raw_job_{name}",
        source=MATRIX_SOURCE,
        inputs=(POLICY_SOURCE, MATRIX_SOURCE),
        note=(
            f"OS-P3 raw-job mode {mode}: {name.replace('_', ' ')} must match "
            "the pinned fail-closed package-admission transcript"
        ),
        fisics_args=defines,
        clang_args=defines,
        expected_exit_code=0,
        expected_stdout=(
            f"OSP3 raw-job mode={mode} seed=7f4a7c15 budget=256 cases={cases} "
            f"accept={accepted} reject={rejected} failures=0 digest={digest}\n"
        ),
        expected_stderr="",
    )


RUNTIME_PROBES = [
    raw_job_probe(name, mode, cases, accepted, rejected, digest)
    for name, mode, cases, accepted, rejected, digest in MODE_OUTPUTS
] + [
    RuntimeProbe(
        probe_id="15__probe_osp3_raw_job_mutation_alt",
        source=MATRIX_SOURCE,
        inputs=(POLICY_SOURCE, MATRIX_SOURCE),
        note=(
            "OS-P3 raw-job alternate mutation stream: 1,024 bounded "
            "header/table/payload mutations"
        ),
        fisics_args=(
            "-DOSP3_JOB_MODE=6",
            "-DOSP3_JOB_SEED=0xa5a5a5a5U",
            "-DOSP3_JOB_CASE_BUDGET=1024U",
        ),
        clang_args=(
            "-DOSP3_JOB_MODE=6",
            "-DOSP3_JOB_SEED=0xa5a5a5a5U",
            "-DOSP3_JOB_CASE_BUDGET=1024U",
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OSP3 raw-job mode=6 seed=a5a5a5a5 budget=1024 cases=1024 "
            "accept=186 reject=838 failures=0 digest=3408012459\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_osp3_raw_job_mutation_stress_sanitized",
        source=MATRIX_SOURCE,
        inputs=(POLICY_SOURCE, MATRIX_SOURCE),
        note=(
            "OS-P3 raw-job stress stream: 4,096 bounded mutations with the "
            "Clang reference instrumented by ASan+UBSan"
        ),
        fisics_args=(
            "-DOSP3_JOB_MODE=6",
            "-DOSP3_JOB_SEED=0xffffffffU",
            "-DOSP3_JOB_CASE_BUDGET=4096U",
        ),
        clang_args=(
            "-DOSP3_JOB_MODE=6",
            "-DOSP3_JOB_SEED=0xffffffffU",
            "-DOSP3_JOB_CASE_BUDGET=4096U",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OSP3 raw-job mode=6 seed=ffffffff budget=4096 cases=4096 "
            "accept=720 reject=3376 failures=0 digest=3372341051\n"
        ),
        expected_stderr="",
    ),
]

OBJECT_PROBES = [
    ObjectProbe(
        probe_id="15__probe_osp3_raw_job_policy_object",
        source=POLICY_SOURCE,
        note=(
            "OS-P3 raw-job policy object must be deterministic, freestanding, "
            "helper-free, hardware-blind, and no-red-zone"
        ),
        required_exports=("osp3_raw_job_admit",),
        allowed_relocations=("R_X86_64_PC32",),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    )
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
