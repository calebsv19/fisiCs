from pathlib import Path
from lib.models import ObjectProbe, RuntimeProbe
from .osp3_object import HARDWARE_BLIND_FORBIDDEN

PROBE_DIR = Path(__file__).resolve().parent.parent.parent
POLICY = PROBE_DIR / "runtime/15__probe_osp3_raw_storage_policy.c"
MATRIX = PROBE_DIR / "runtime/15__probe_osp3_raw_storage_matrix.c"
MODES = (
    ("valid_images",0,16,16,0,726037349),
    ("truncation_geometry",1,129,1,128,1749755878),
    ("rejection_precedence",2,6,0,6,1358008951),
    ("sequence_replay",3,4,2,2,2949061641),
    ("extent_overlap",4,6,1,5,3912859602),
    ("late_reject_reset",5,64,32,32,1114492101),
    ("mutation_base",6,256,25,231,2780351482),
)

def probe(name, mode, cases, accept, reject, digest):
    defs=(f"-DOSP3_STORE_MODE={mode}","-DOSP3_STORE_SEED=0x93d765a1U","-DOSP3_STORE_BUDGET=256U")
    return RuntimeProbe(
        probe_id=f"15__probe_osp3_raw_storage_{name}", source=MATRIX,
        inputs=(POLICY,MATRIX),
        note=f"OS-P3 raw-storage mode {mode}: {name.replace('_',' ')} must match the pinned fail-closed transcript",
        fisics_args=defs, clang_args=defs, expected_exit_code=0,
        expected_stdout=f"OSP3 raw-storage mode={mode} seed=93d765a1 budget=256 cases={cases} accept={accept} reject={reject} failures=0 digest={digest}\n",
        expected_stderr="")

RUNTIME_PROBES=[probe(*row) for row in MODES]+[
    RuntimeProbe(
        probe_id="15__probe_osp3_raw_storage_mutation_alt",source=MATRIX,
        inputs=(POLICY,MATRIX),note="OS-P3 raw-storage alternate 1,024-case mutation stream",
        fisics_args=("-DOSP3_STORE_MODE=6","-DOSP3_STORE_SEED=0xa5a5a5a5U","-DOSP3_STORE_BUDGET=1024U"),
        clang_args=("-DOSP3_STORE_MODE=6","-DOSP3_STORE_SEED=0xa5a5a5a5U","-DOSP3_STORE_BUDGET=1024U"),
        expected_exit_code=0,expected_stdout="OSP3 raw-storage mode=6 seed=a5a5a5a5 budget=1024 cases=1024 accept=108 reject=916 failures=0 digest=3000300579\n",expected_stderr=""),
    RuntimeProbe(
        probe_id="15__probe_osp3_raw_storage_mutation_stress_sanitized",source=MATRIX,
        inputs=(POLICY,MATRIX),note="OS-P3 raw-storage 4,096-case mutation stream with Clang ASan+UBSan",
        fisics_args=("-DOSP3_STORE_MODE=6","-DOSP3_STORE_SEED=0xffffffffU","-DOSP3_STORE_BUDGET=4096U"),
        clang_args=("-DOSP3_STORE_MODE=6","-DOSP3_STORE_SEED=0xffffffffU","-DOSP3_STORE_BUDGET=4096U","-fsanitize=address,undefined","-fno-omit-frame-pointer"),
        expected_exit_code=0,expected_stdout="OSP3 raw-storage mode=6 seed=ffffffff budget=4096 cases=4096 accept=384 reject=3712 failures=0 digest=2197151647\n",expected_stderr="")
]
OBJECT_PROBES=[ObjectProbe(
    probe_id="15__probe_osp3_raw_storage_policy_object",source=POLICY,
    note="OS-P3 raw-storage object must be deterministic, freestanding, helper-free, hardware-blind, and no-red-zone",
    required_exports=("osp3_raw_storage_admit",),allowed_relocations=("R_X86_64_PC32",),
    forbidden_instructions=HARDWARE_BLIND_FORBIDDEN)]
DIAG_PROBES=[]
DIAG_JSON_PROBES=[]
