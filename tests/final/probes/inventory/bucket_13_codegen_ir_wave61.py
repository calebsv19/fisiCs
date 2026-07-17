from pathlib import Path

from lib.models import RuntimeProbe


PROBE_DIR = Path(__file__).resolve().parent.parent

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="13__probe_wave61_proto_cache_mixed_order_runtime",
        source=PROBE_DIR / "runtime/13__probe_wave61_proto_cache_mixed_order_runtime.c",
        note=(
            "wave61 prototype-cache seam: compatible () and (void) declarations "
            "in both orders must remain callable through direct and cross-assigned "
            "indirect consumers"
        ),
        promoted_test_id="13__ir_wave61_proto_cache_mixed_order",
    ),
    RuntimeProbe(
        probe_id="13__probe_wave61_proto_cache_alpha_order_control_runtime",
        source=PROBE_DIR / "runtime/13__probe_wave61_proto_cache_alpha_order_control_runtime.c",
        note=(
            "wave61 alpha/order control: renamed declarations, reversed typedef order, "
            "and reversed consumer order must preserve the mixed-prototype result"
        ),
        promoted_test_id="13__ir_wave61_proto_cache_alpha_order_control",
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
