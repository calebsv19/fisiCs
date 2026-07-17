from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="11__probe_wave88_indirect_no_proto_scalar_promotions_all_fisics",
        source=PROBE_DIR / "runtime/11__probe_wave88_indirect_no_proto_runtime_main.c",
        note="wave88 strict: scalar float and narrow integer arguments must receive default promotions through direct and factory-returned no-prototype function pointers",
        promoted_test_id="11__probe_wave88_indirect_no_proto_scalar_promotions_runtime",
        inputs=[
            PROBE_DIR / "runtime/11__probe_wave88_indirect_no_proto_runtime_main.c",
            PROBE_DIR / "runtime/11__probe_wave88_indirect_no_proto_runtime_lib.c",
        ],
    ),
    RuntimeProbe(
        probe_id="11__probe_wave88_indirect_no_proto_scalar_promotions_mixed_clang_callee",
        source=PROBE_DIR / "runtime/11__probe_wave88_indirect_no_proto_runtime_main.c",
        note="wave88 forward mixed oracle: a fisiCs caller must promote scalar arguments before invoking direct and factory-returned no-prototype pointers to clang-built double/int callees",
        promoted_test_id="11__probe_wave88_indirect_no_proto_scalar_promotions_runtime",
        inputs=[
            PROBE_DIR / "runtime/11__probe_wave88_indirect_no_proto_runtime_main.c",
        ],
        mixed_clang_inputs=[
            PROBE_DIR / "runtime/11__probe_wave88_indirect_no_proto_runtime_lib.c",
        ],
    ),
    RuntimeProbe(
        probe_id="11__probe_wave88_indirect_no_proto_scalar_promotions_reverse_clang_caller",
        source=PROBE_DIR / "runtime/11__probe_wave88_indirect_no_proto_runtime_lib.c",
        note="wave88 reverse mixed control: a clang caller must pass promoted scalars through no-prototype pointers returned by a fisiCs-built factory",
        promoted_test_id="11__probe_wave88_indirect_no_proto_scalar_promotions_runtime",
        inputs=[
            PROBE_DIR / "runtime/11__probe_wave88_indirect_no_proto_runtime_lib.c",
        ],
        mixed_clang_inputs=[
            PROBE_DIR / "runtime/11__probe_wave88_indirect_no_proto_runtime_main.c",
        ],
    ),
    RuntimeProbe(
        probe_id="11__probe_wave88_indirect_no_proto_double_int_control",
        source=PROBE_DIR / "runtime/11__probe_wave88_indirect_no_proto_control_runtime.c",
        note="wave88 current-threshold control: direct and factory-returned no-prototype dispatch with already-promoted double/int arguments remains operational",
        promoted_test_id="11__probe_wave88_indirect_no_proto_double_int_control_runtime",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
