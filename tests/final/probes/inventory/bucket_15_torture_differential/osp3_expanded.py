from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent.parent

FAMILIES = (
    (
        "admission",
        "15__probe_osp3_admission_precedence_matrix.c",
        (2166136261, 1789959244, 1555462464, 3221100319),
    ),
    (
        "queue",
        "15__probe_osp3_queue_transition_matrix.c",
        (757602046, 3736021253, 2681491344, 1790429436),
    ),
    (
        "scheduler",
        "15__probe_osp3_scheduler_selection_matrix.c",
        (2738958700, 873821048, 3060969056, 17677920),
    ),
    (
        "sync-rank",
        "15__probe_osp3_sync_rank_matrix.c",
        (2135587861, 4024655387, 2985448610, 2610210675),
    ),
    (
        "extent",
        "15__probe_osp3_extent_overflow_matrix.c",
        (374761393, 1494821840, 1920088649, 724164709),
    ),
    (
        "token",
        "15__probe_osp3_generation_token_matrix.c",
        (1759714724, 2685014073, 1486820027, 3183947719),
    ),
    (
        "scalar-double",
        "15__probe_osp3_scalar_double_dyadic_matrix.c",
        (668265263, 3272499649, 2909063695, 1274090958),
    ),
    (
        "variadic-sret",
        "15__probe_osp3_variadic_struct_return_matrix.c",
        (2496678331, 652074410, 4197787876, 3762523821),
    ),
    (
        "callback",
        "15__probe_osp3_callback_dispatch_matrix.c",
        (3528531795, 2876664558, 3045072206, 3212312239),
    ),
    (
        "aggregate",
        "15__probe_osp3_aggregate_checkpoint_matrix.c",
        (3449720151, 4166968593, 1411522653, 1200677242),
    ),
)

CONFIGS = (
    ("zero", "6d2b79f5", 0, False),
    ("one", "00000001", 1, False),
    ("prime", "a5a5a5a5", 257, False),
    ("stress_sanitized", "ffffffff", 4096, True),
)


def expanded_probe(family, source_name, digest, config):
    config_name, seed, case_budget, sanitized = config
    defines = (
        f"-DOSP3_SEED=0x{seed}u",
        f"-DOSP3_CASE_BUDGET={case_budget}u",
    )
    sanitizer_args = (
        "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer",
    ) if sanitized else ()
    return RuntimeProbe(
        probe_id=f"15__probe_osp3_expand_{family.replace('-', '_')}_{config_name}",
        source=PROBE_DIR / f"runtime/{source_name}",
        note=(
            f"OS-P3 expanded matrix {family}: seed {seed}, "
            f"{case_budget}-case budget"
            + (
                ", with the Clang reference instrumented by ASan+UBSan"
                if sanitized
                else ""
            )
        ),
        fisics_args=defines,
        clang_args=(*defines, *sanitizer_args),
        expected_exit_code=0,
        expected_stdout=(
            f"OSP3 {family} seed={seed} cases={case_budget} digest={digest}\n"
        ),
        expected_stderr="",
    )


RUNTIME_PROBES = [
    expanded_probe(family, source_name, digests[index], config)
    for family, source_name, digests in FAMILIES
    for index, config in enumerate(CONFIGS)
] + [
    RuntimeProbe(
        probe_id="15__probe_osp3_long_double_abi_model",
        source=PROBE_DIR / "runtime/15__probe_osp3_long_double_abi_model.c",
        note=(
            "OS-P3 long-double portability model: accept only the common "
            "binary64, x87-extended, or IEEE-binary128 ABI descriptions"
        ),
        expected_exit_code=0,
        expected_stdout_variants=(
            "OSP3 long-double-abi mant=53 maxexp=1024 size=8 eval=0\n",
            "OSP3 long-double-abi mant=64 maxexp=16384 size=16 eval=0\n",
            "OSP3 long-double-abi mant=113 maxexp=16384 size=16 eval=0\n",
        ),
        expected_stderr="",
        extra_differential_compiler="gcc",
    ),
    RuntimeProbe(
        probe_id="15__probe_osp3_long_double_variadic_sret_oracle_variants",
        source=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_permute_edge_main.c"
        ),
        inputs=(
            PROBE_DIR
            / "runtime/15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_permute_edge_main.c",
            PROBE_DIR
            / "runtime/15__probe_axis3_wave2_runtime_clang_gcc_tri_diff_multitu_variadic_struct_return_permute_edge_lib.c",
        ),
        note=(
            "OS-P3 target-aware oracle reduction: the strict multi-TU "
            "variadic long-double struct-return lane must match all available "
            "compilers and one of the two observed target transcripts"
        ),
        expected_exit_code=0,
        expected_stdout_variants=(
            "2031059386 1405211949\n",
            "485603065 1406966467\n",
        ),
        expected_stderr="",
        extra_differential_compiler="gcc",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
