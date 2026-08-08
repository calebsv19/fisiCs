from pathlib import Path

from lib.models import ObjectProbe, RuntimeProbe

from .osp3_object import HARDWARE_BLIND_FORBIDDEN


PROBE_DIR = Path(__file__).resolve().parent.parent.parent
ROOT_DIR = PROBE_DIR.parents[2]
POLICY_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_workload_v1_policy.c"
)
MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_workload_v1_matrix.c"
)
RESULT_POLICY_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_result_v1_policy.c"
)
RESULT_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_result_v1_matrix.c"
)
EDU21_CONTROL_POLICY_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu21_control_v1_policy.c"
)
EDU21_CONTROL_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu21_control_v1_matrix.c"
)
EDU22_QUEUE_POLICY_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu22_queue_v2_policy.c"
)
EDU22_QUEUE_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu22_queue_v2_matrix.c"
)
TWO_OWNER_COMPOSITION_MODEL_SOURCE = (
    PROBE_DIR
    / "runtime/15__probe_os_post_edu19_two_owner_fault_composition_model.c"
)
TWO_OWNER_COMPOSITION_MATRIX_SOURCE = (
    PROBE_DIR
    / "runtime/15__probe_os_post_edu19_two_owner_fault_composition_matrix.c"
)
EDU23_PARALLELISM_POLICY_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu23_parallelism_policy.c"
)
EDU23_PARALLELISM_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu23_parallelism_matrix.c"
)
EDU24_ARTIFACT_MODEL_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu24_artifact_exchange_model.c"
)
EDU24_ARTIFACT_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu24_artifact_exchange_matrix.c"
)
EDU24_31_WIRE_POLICY_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu24_31_wire_history_policy.c"
)
EDU24_31_WIRE_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu24_31_wire_history_matrix.c"
)
EDU25_LOADER_MODEL_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu25_loader_geometry_model.c"
)
EDU25_LOADER_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu25_loader_geometry_matrix.c"
)
EDU26_REUSE_POLICY_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu26_generation_reuse_policy.c"
)
EDU26_REUSE_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu26_generation_reuse_matrix.c"
)
EDU27_PHASE_MODEL_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu27_phase_execution_model.c"
)
EDU27_PHASE_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu27_phase_execution_matrix.c"
)
EDU28_ARTIFACT_POLICY_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu28_artifact_meta_policy.c"
)
EDU28_ARTIFACT_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu28_artifact_meta_matrix.c"
)
EDU29_30_ASYNC_STOP_MODEL_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu29_30_async_stop_model.c"
)
EDU29_30_ASYNC_STOP_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu29_30_async_stop_matrix.c"
)
EDU31_TIME_MODEL_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu31_time_arithmetic_model.c"
)
EDU31_TIME_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu31_time_arithmetic_matrix.c"
)
EDU34_POLICY_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu34_deadline_entry_policy.c"
)
EDU34_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu34_deadline_entry_matrix.c"
)
EDU35_POLICY_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu35_checkpoint_snapshot_policy.c"
)
EDU35_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu35_checkpoint_snapshot_matrix.c"
)
EDU36_POLICY_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu36_resume_entry_policy.c"
)
EDU36_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu36_resume_entry_matrix.c"
)
EDU37_MODEL_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu37_two_owner_store_model.c"
)
EDU37_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu37_two_owner_store_matrix.c"
)
EDU38_CONTROL_POLICY_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu38_control_v13_policy.c"
)
EDU38_CONTROL_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu38_control_v13_matrix.c"
)
EDU38_CONTEXT_MODEL_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu38_runner_context_model.c"
)
EDU38_CONTEXT_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu38_runner_context_matrix.c"
)
EDU39_PHASE_OWNER_MODEL_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu39_phase_owner_model.c"
)
EDU39_PHASE_OWNER_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu39_phase_owner_matrix.c"
)
EDU40_MAILBOX_OWNER_MODEL_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu40_mailbox_owner_model.c"
)
EDU40_MAILBOX_OWNER_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu40_mailbox_owner_matrix.c"
)
EDU41_TWO_ACTIVE_MODEL_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu41_two_active_runner_model.c"
)
EDU41_TWO_ACTIVE_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu41_two_active_runner_matrix.c"
)
DURABLE_OWNER_CHAIN_MODEL_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_durable_owner_chain_model.c"
)
DURABLE_OWNER_CHAIN_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_durable_owner_chain_matrix.c"
)
TEMPORAL_FAULT_SEQUENCE_MODEL_SOURCE = (
    PROBE_DIR
    / "runtime/15__probe_os_post_edu19_temporal_fault_sequence_model.c"
)
TEMPORAL_FAULT_SEQUENCE_MATRIX_SOURCE = (
    PROBE_DIR
    / "runtime/15__probe_os_post_edu19_temporal_fault_sequence_matrix.c"
)
TEMPORAL_PAIR_FAULT_MODEL_SOURCE = (
    PROBE_DIR
    / "runtime/15__probe_os_post_edu19_temporal_pair_fault_model.c"
)
TEMPORAL_PAIR_FAULT_MATRIX_SOURCE = (
    PROBE_DIR
    / "runtime/15__probe_os_post_edu19_temporal_pair_fault_matrix.c"
)
CROSS_MODEL_TEMPORAL_COMPOSITION_MODEL_SOURCE = (
    PROBE_DIR
    / "runtime/15__probe_os_post_edu19_cross_model_temporal_composition_model.c"
)
CROSS_MODEL_TEMPORAL_COMPOSITION_MATRIX_SOURCE = (
    PROBE_DIR
    / "runtime/15__probe_os_post_edu19_cross_model_temporal_composition_matrix.c"
)
EDU48_BUNDLE_SELECTION_MODEL_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu48_bundle_selection_model.c"
)
EDU48_BUNDLE_SELECTION_MATRIX_SOURCE = (
    PROBE_DIR / "runtime/15__probe_os_post_edu19_edu48_bundle_selection_matrix.c"
)
OSP2_SCHEDULER_MODEL_SOURCE = (
    ROOT_DIR / "tests/os_policy/cases/osp2_scheduler_transition.c"
)

RUNTIME_PROBES = [
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu21_control_v1_matrix",
        source=EDU21_CONTROL_MATRIX_SOURCE,
        inputs=(EDU21_CONTROL_POLICY_SOURCE, EDU21_CONTROL_MATRIX_SOURCE),
        note=(
            "Exact immutable EDU-21 control_kernel.c: original Wire-v1 "
            "operations one through six, operation-three payload geometry, "
            "reserved fields, checksum/error precedence, unsupported "
            "operation rejection, maximum request identity, and unaligned "
            "frames"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu21-control-v1 snapshot=d1544b0 vectors=33 "
            "digest=2910115348 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu22_queue_v2_matrix",
        source=EDU22_QUEUE_MATRIX_SOURCE,
        inputs=(EDU22_QUEUE_POLICY_SOURCE, EDU22_QUEUE_MATRIX_SOURCE),
        note=(
            "Exact immutable EDU-22 queue_kernel.c: Queue metadata v3, "
            "Entry-v2 and Trace-v1 structure, event ordering, state-specific "
            "terminal and resource evidence, unused/padding zeroing, "
            "unaligned sectors, action/resource precedence, and the exact "
            "caller-owned requirement to validate before action selection"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu22-queue-v2 snapshot=37900ba vectors=65 "
            "digest=45884357 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_two_owner_fault_composition_matrix",
        source=TWO_OWNER_COMPOSITION_MATRIX_SOURCE,
        inputs=(
            EDU40_MAILBOX_OWNER_MODEL_SOURCE,
            EDU41_TWO_ACTIVE_MODEL_SOURCE,
            TWO_OWNER_COMPOSITION_MODEL_SOURCE,
            TWO_OWNER_COMPOSITION_MATRIX_SOURCE,
        ),
        note=(
            "Hardware-blind composition of immutable EDU-40/41 contracts: "
            "two exact active identities, singleton-mailbox owner pinning, "
            "wrong-owner dispatch/completion rejection, independent "
            "cancel/deadline/budget/corruption outcomes, exact owner-local "
            "phase release, peer-preserving retirement, and unaligned "
            "records; authoritative execution remains OS-owned"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 two-owner-composition "
            "snapshots=1efb1ac+695ec66 vectors=36 digest=3296091944 "
            "result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_durable_owner_chain_matrix",
        source=DURABLE_OWNER_CHAIN_MATRIX_SOURCE,
        inputs=(
            POLICY_SOURCE,
            EDU26_REUSE_POLICY_SOURCE,
            EDU35_POLICY_SOURCE,
            EDU37_MODEL_SOURCE,
            EDU39_PHASE_OWNER_MODEL_SOURCE,
            EDU40_MAILBOX_OWNER_MODEL_SOURCE,
            EDU41_TWO_ACTIVE_MODEL_SOURCE,
            DURABLE_OWNER_CHAIN_MODEL_SOURCE,
            DURABLE_OWNER_CHAIN_MATRIX_SOURCE,
        ),
        note=(
            "Hardware-blind composition of immutable EDU-26/35/37/39/40/41 "
            "contracts: generation reuse after exact acknowledged retirement "
            "with peer preservation, exact checkpoint-lane and running-owner "
            "correlation, phase-owner/mailbox/runner correlation, and a full "
            "metadata-to-dispatch durable owner chain. The EDU-26 generation "
            "action alone binds request identity only for a prepared ACK; the "
            "full chain supplies request binding through checkpoint, phase, "
            "runner, and mailbox correlation. Authoritative execution and "
            "persistence remain OS-owned"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 durable-owner-chain "
            "snapshots=8359429+5b39037+30e34df+6dd5cd2+1efb1ac+695ec66 "
            "vectors=46 digest=1751309697 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_temporal_fault_sequence_matrix",
        source=TEMPORAL_FAULT_SEQUENCE_MATRIX_SOURCE,
        inputs=(
            POLICY_SOURCE,
            EDU26_REUSE_POLICY_SOURCE,
            EDU35_POLICY_SOURCE,
            EDU37_MODEL_SOURCE,
            EDU39_PHASE_OWNER_MODEL_SOURCE,
            EDU40_MAILBOX_OWNER_MODEL_SOURCE,
            EDU41_TWO_ACTIVE_MODEL_SOURCE,
            DURABLE_OWNER_CHAIN_MODEL_SOURCE,
            TEMPORAL_FAULT_SEQUENCE_MODEL_SOURCE,
            TEMPORAL_FAULT_SEQUENCE_MATRIX_SOURCE,
        ),
        note=(
            "Frontier lane: hardware-blind temporal composition over the frozen "
            "EDU-26/35/37/39/40/41 contracts and durable-owner chain: "
            "pre-ACK interruption blocks generation reuse, post-checkpoint "
            "restart distinguishes an overwrite destination from restored "
            "record identity, mid-phase owner loss preserves the peer, and "
            "post-completion retirement blocks stale redispatch. This is "
            "compiler-probe evidence, not an EDU-44 OS implementation claim"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 temporal-fault-sequence "
            "basis=durable-owner-chain-v1 vectors=20 digest=3207911626 "
            "result=PASS\n"
        ),
        expected_stderr="",
        promotion_disposition="probe-only",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_temporal_pair_fault_matrix",
        source=TEMPORAL_PAIR_FAULT_MATRIX_SOURCE,
        inputs=(
            POLICY_SOURCE,
            EDU26_REUSE_POLICY_SOURCE,
            EDU35_POLICY_SOURCE,
            EDU37_MODEL_SOURCE,
            EDU39_PHASE_OWNER_MODEL_SOURCE,
            EDU40_MAILBOX_OWNER_MODEL_SOURCE,
            EDU41_TWO_ACTIVE_MODEL_SOURCE,
            DURABLE_OWNER_CHAIN_MODEL_SOURCE,
            TEMPORAL_FAULT_SEQUENCE_MODEL_SOURCE,
            TEMPORAL_PAIR_FAULT_MODEL_SOURCE,
            TEMPORAL_PAIR_FAULT_MATRIX_SOURCE,
        ),
        note=(
            "Frontier lane: paired temporal contradictions over the first "
            "EDU-44 compiler model: checkpoint interruption plus generation "
            "reuse, completion/retirement reordering, owner loss plus peer "
            "corruption, stale ACK/mailbox/snapshot evidence crossing "
            "generations, and exact rejection of a checksum-valid recovery "
            "record with stale embedded identity. This is compiler-probe "
            "evidence, not an EDU-45 OS implementation claim"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 temporal-pair-fault "
            "basis=temporal-fault-sequence-v1 vectors=25 digest=2914858269 "
            "result=PASS\n"
        ),
        expected_stderr="",
        promotion_disposition="probe-only",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_cross_model_temporal_composition_matrix",
        source=CROSS_MODEL_TEMPORAL_COMPOSITION_MATRIX_SOURCE,
        inputs=(
            POLICY_SOURCE,
            EDU26_REUSE_POLICY_SOURCE,
            EDU35_POLICY_SOURCE,
            EDU37_MODEL_SOURCE,
            EDU39_PHASE_OWNER_MODEL_SOURCE,
            EDU40_MAILBOX_OWNER_MODEL_SOURCE,
            EDU41_TWO_ACTIVE_MODEL_SOURCE,
            DURABLE_OWNER_CHAIN_MODEL_SOURCE,
            TEMPORAL_FAULT_SEQUENCE_MODEL_SOURCE,
            TEMPORAL_PAIR_FAULT_MODEL_SOURCE,
            EDU22_QUEUE_POLICY_SOURCE,
            EDU24_31_WIRE_POLICY_SOURCE,
            OSP2_SCHEDULER_MODEL_SOURCE,
            CROSS_MODEL_TEMPORAL_COMPOSITION_MODEL_SOURCE,
            CROSS_MODEL_TEMPORAL_COMPOSITION_MATRIX_SOURCE,
        ),
        note=(
            "Frontier lane: hardware-blind cross-model temporal composition "
            "requires independent Queue-v2 admission, Wire-v7 admission, "
            "a deterministic scheduler handoff, and the established "
            "cross-generation temporal rejection to agree. Any stale ACK, "
            "mailbox, lane, queue, or wire evidence fails closed; this is "
            "compiler-probe evidence, not OS-P4 metadata or OS execution"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 cross-model-temporal "
            "basis=queue-v2+wire-v7+scheduler vectors=8 digest=762569745 "
            "result=PASS\n"
        ),
        expected_stderr="",
        promotion_disposition="probe-only",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu48_bundle_selection_matrix",
        source=EDU48_BUNDLE_SELECTION_MATRIX_SOURCE,
        inputs=(
            POLICY_SOURCE,
            EDU48_BUNDLE_SELECTION_MODEL_SOURCE,
            EDU48_BUNDLE_SELECTION_MATRIX_SOURCE,
        ),
        note=(
            "Source-derived immutable EDU-48 bundle-selection C boundary: "
            "only frozen program identifiers one and two may consume a "
            "self-consistent Workload-v1 record; program one returns the "
            "published result while program two recomputes the damped result. "
            "Signing, artifact persistence, and guest loading remain host- "
            "or OS-owned and are not modeled here"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu48-bundle-selection vectors=12 result=PASS\n"
        ),
        expected_stderr="",
        promoted_test_id="15__runtime_edu48_frozen_program_selection",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu23_parallelism_matrix",
        source=EDU23_PARALLELISM_MATRIX_SOURCE,
        inputs=(
            EDU23_PARALLELISM_POLICY_SOURCE,
            EDU23_PARALLELISM_MATRIX_SOURCE,
        ),
        note=(
            "Immutable EDU-23 generated-C policy mirror: bounded worker "
            "admission, grants, deterministic partition values, path "
            "evidence, terminal state, and fail-closed identity conflicts"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu23-parallelism snapshot=cf375ea vectors=40 "
            "digest=2666730100 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu24_artifact_exchange_matrix",
        source=EDU24_ARTIFACT_MATRIX_SOURCE,
        inputs=(EDU24_ARTIFACT_MODEL_SOURCE, EDU24_ARTIFACT_MATRIX_SOURCE),
        note=(
            "Assembly-derived immutable EDU-24 result-artifact contract: "
            "complete-entry and exact-identity admission, bounded cursor, "
            "512-byte chunk geometry, final markers, and fail-closed "
            "misrepresented request and response shapes"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu24-artifact snapshot=06979f3 vectors=38 "
            "digest=2676546390 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu24_31_wire_history_matrix",
        source=EDU24_31_WIRE_MATRIX_SOURCE,
        inputs=(EDU24_31_WIRE_POLICY_SOURCE, EDU24_31_WIRE_MATRIX_SOURCE),
        note=(
            "Exact immutable generated-C Wire-v2 through Wire-v7 history "
            "for EDU-24..31: version boundaries, operation payloads, "
            "reserved bytes, checksum/error precedence, and fail-closed "
            "cross-version or malformed frames"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu24-31-wire-history snapshots=06979f3..0d10b3d "
            "vectors=146 digest=804736364 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu25_loader_geometry_matrix",
        source=EDU25_LOADER_MATRIX_SOURCE,
        inputs=(EDU25_LOADER_MODEL_SOURCE, EDU25_LOADER_MATRIX_SOURCE),
        note=(
            "Assembly-derived immutable EDU-25 bounded-loader contract: "
            "disk geometry, kernel capacity, EDD availability, bounded "
            "64-sector transfer decomposition, destination/LBA progress, "
            "and fail-closed malformed plans"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu25-loader snapshot=ebed910 vectors=47 "
            "digest=2506815014 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu26_generation_reuse_matrix",
        source=EDU26_REUSE_MATRIX_SOURCE,
        inputs=(EDU26_REUSE_POLICY_SOURCE, EDU26_REUSE_MATRIX_SOURCE),
        note=(
            "Immutable EDU-26 generated-C generation-safe reuse policy: "
            "metadata, generation actions, tombstones, reservability, ACK "
            "identity, wrap rejection, and stale or contradictory inputs"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu26-generation-reuse snapshot=8359429 "
            "vectors=39 digest=1785986164 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu27_phase_execution_matrix",
        source=EDU27_PHASE_MATRIX_SOURCE,
        inputs=(EDU27_PHASE_MODEL_SOURCE, EDU27_PHASE_MATRIX_SOURCE),
        note=(
            "Immutable EDU-27 phase-aware execution mirror: legal event "
            "advance, exact phase values, prefix and trace evidence, "
            "monotonic ordering, and fail-closed impossible histories"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu27-phase-execution snapshot=9c9e2b0 "
            "vectors=43 digest=1274310908 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu28_artifact_meta_matrix",
        source=EDU28_ARTIFACT_MATRIX_SOURCE,
        inputs=(EDU28_ARTIFACT_POLICY_SOURCE, EDU28_ARTIFACT_MATRIX_SOURCE),
        note=(
            "Exact immutable EDU-28 artifact-metadata validator slice: "
            "identity, byte and chunk bounds, presence bitmap/count "
            "coherence, checksums, unaligned data, and recomputed "
            "contradictions"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu28-artifact-meta snapshot=c195bf2 vectors=35 "
            "digest=2829665416 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu29_30_async_stop_matrix",
        source=EDU29_30_ASYNC_STOP_MATRIX_SOURCE,
        inputs=(
            EDU29_30_ASYNC_STOP_MODEL_SOURCE,
            EDU29_30_ASYNC_STOP_MATRIX_SOURCE,
        ),
        note=(
            "Immutable EDU-29/30 asynchronous activation and cooperative "
            "stop contract mirror: FIFO eligibility, runner identity, "
            "work shape, bounded progress, cancellation/deadline/budget "
            "precedence, terminal evidence, and fail-closed conflicts"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu29-30-async-stop snapshots=2b8189a+fac8fd4 "
            "vectors=56 digest=511270998 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu31_time_arithmetic_matrix",
        source=EDU31_TIME_MATRIX_SOURCE,
        inputs=(EDU31_TIME_MODEL_SOURCE, EDU31_TIME_MATRIX_SOURCE),
        note=(
            "Assembly-derived immutable EDU-31 monotonic-time contract: "
            "calibration windows/pair agreement, checked divide-first "
            "nanosecond conversion, one-wrap unsigned deltas, monotonic "
            "reads, snapshot flags, and overflow rejection"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu31-time snapshot=0d10b3d vectors=32 "
            "digest=1525097272 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_workload_v1_policy_matrix",
        source=MATRIX_SOURCE,
        inputs=(POLICY_SOURCE, MATRIX_SOURCE),
        note=(
            "OS post-EDU-19 policy intake from immutable EDU-32: exact "
            "Workload-v1 parsing, finite-f64 admission, step bounds, "
            "independently pinned results, unaligned input, and fail-closed "
            "identity contradictions"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 workload-v1 snapshot=274f955 vectors=24 "
            "digest=3803321321 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_result_v1_policy_matrix",
        source=RESULT_MATRIX_SOURCE,
        inputs=(RESULT_POLICY_SOURCE, RESULT_MATRIX_SOURCE),
        note=(
            "OS post-EDU-19 policy intake from immutable EDU-33: exact "
            "Result-v1 sector structure, identity bounds, nested and whole "
            "checksums, zero padding, unaligned input, opaque result bits, "
            "and fail-closed identity contradictions"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 result-v1 snapshot=49e4304 vectors=31 "
            "digest=1971389970 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu34_deadline_entry_matrix",
        source=EDU34_MATRIX_SOURCE,
        inputs=(EDU34_POLICY_SOURCE, EDU34_MATRIX_SOURCE),
        note=(
            "OS post-EDU-19 policy intake from immutable EDU-34: exact "
            "durable queue-entry validation for state-dependent deadline "
            "placement, timeout/budget/cancellation terminal evidence, "
            "preserved phase prefixes, bounds, unaligned input, and "
            "recomputed-checksum contradictions"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu34-deadline snapshot=bf95c67 vectors=30 "
            "digest=2603998634 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu35_checkpoint_snapshot_matrix",
        source=EDU35_MATRIX_SOURCE,
        inputs=(EDU35_POLICY_SOURCE, EDU35_MATRIX_SOURCE),
        note=(
            "OS post-EDU-19 policy intake from immutable EDU-35: exact "
            "non-resumable checkpoint validation for owner/policy identity, "
            "phase-three capture, duration/work bounds, repeated partition "
            "evidence, three nested integrity layers, zero padding, "
            "unaligned input, recomputed-checksum contradictions, and the "
            "surrounding path's ownership of nested Workload-v1 semantics"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu35-checkpoint snapshot=5b39037 vectors=47 "
            "digest=2665877225 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu36_resume_entry_matrix",
        source=EDU36_MATRIX_SOURCE,
        inputs=(EDU36_POLICY_SOURCE, EDU36_MATRIX_SOURCE),
        note=(
            "OS post-EDU-19 policy intake from immutable EDU-36: exact "
            "Entry-v8 and Trace-v3 checkpoint-resume validation for "
            "resumed running/complete/interrupted/timeout/cancel states, "
            "restored phase-three evidence, ordinary-path separation, "
            "unaligned input, fail-closed contradictions, and the frozen "
            "validator's current compact-trace flag admission"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu36-resume snapshot=d0e429b vectors=38 "
            "digest=1966400574 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu37_two_owner_store_matrix",
        source=EDU37_MATRIX_SOURCE,
        inputs=(
            POLICY_SOURCE,
            EDU35_POLICY_SOURCE,
            EDU37_MODEL_SOURCE,
            EDU37_MATRIX_SOURCE,
        ),
        note=(
            "OS post-EDU-19 contract mirror from immutable EDU-37: "
            "two-lane zero/valid/unique admission, nested Workload-v1 "
            "semantics, exact-owner then empty then reclaimable selection, "
            "RUNNING and interrupted-prefix-three protection, full-capacity "
            "failure, identity reuse, and unaligned storage; authoritative "
            "implementation remains assembly-owned"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu37-two-owner snapshot=30e34df vectors=36 "
            "digest=1800929311 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu38_control_v13_matrix",
        source=EDU38_CONTROL_MATRIX_SOURCE,
        inputs=(EDU38_CONTROL_POLICY_SOURCE, EDU38_CONTROL_MATRIX_SOURCE),
        note=(
            "Exact immutable EDU-38 control_kernel.c: Wire-v13 admission, "
            "Wire-v12 fail-closed rejection, all twenty operation payload "
            "contracts, checksum/error precedence, bounded identifiers, "
            "workload/deadline/chunk fields, zero padding, and unaligned "
            "request frames"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu38-control-v13 snapshot=59d622a vectors=56 "
            "digest=3919777271 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu38_runner_context_matrix",
        source=EDU38_CONTEXT_MATRIX_SOURCE,
        inputs=(EDU38_CONTEXT_MODEL_SOURCE, EDU38_CONTEXT_MATRIX_SOURCE),
        note=(
            "Assembly-derived immutable EDU-38 runner-context contract: "
            "two 160-byte identities, slot-modulo-two selection, inactive "
            "and sole-active scans, duplicate-active corruption, active "
            "identity/resource shape, terminal evidence, checkpoint lanes, "
            "bounds, isolation, and unaligned records; active limit remains "
            "one and authoritative execution remains OS-owned"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu38-runner-context snapshot=59d622a "
            "vectors=45 digest=3183490976 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu39_phase_owner_matrix",
        source=EDU39_PHASE_OWNER_MATRIX_SOURCE,
        inputs=(
            POLICY_SOURCE,
            EDU39_PHASE_OWNER_MODEL_SOURCE,
            EDU39_PHASE_OWNER_MATRIX_SOURCE,
        ),
        note=(
            "Assembly-derived immutable EDU-39 phase-owner contract: two "
            "independent 224-byte saved owners, exact Workload-v1 and queue "
            "identity correlation, legal load shapes, width-one/width-two/"
            "resumed publication counters, unaligned records, and both "
            "in-flight and completed-but-unjoined switch rejection; active "
            "limit and AP mailbox remain singleton and OS-owned"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu39-phase-owner snapshot=6dd5cd2 vectors=70 "
            "digest=3636582023 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu40_mailbox_owner_matrix",
        source=EDU40_MAILBOX_OWNER_MATRIX_SOURCE,
        inputs=(EDU40_MAILBOX_OWNER_MODEL_SOURCE, EDU40_MAILBOX_OWNER_MATRIX_SOURCE),
        note=(
            "Assembly-derived immutable EDU-40 singleton AP-mailbox contract: "
            "idle/begin and generation exhaustion, exact phase/legacy dispatch "
            "owners, next-generation AP acceptance, distinct completion tuple "
            "publication, successful and correlated-error result envelopes, "
            "retained retirement evidence, width-one silence, and unaligned "
            "records; active limit remains one and authoritative execution "
            "remains OS-owned"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu40-mailbox-owner snapshot=1efb1ac vectors=78 "
            "digest=1787622051 result=PASS\n"
        ),
        expected_stderr="",
    ),
    RuntimeProbe(
        probe_id="15__probe_os_post_edu19_edu41_two_active_runner_matrix",
        source=EDU41_TWO_ACTIVE_MATRIX_SOURCE,
        inputs=(EDU41_TWO_ACTIVE_MODEL_SOURCE, EDU41_TWO_ACTIVE_MATRIX_SOURCE),
        note=(
            "Assembly-derived immutable EDU-41 two-active-runner contract: "
            "bounded active counting, exact RUNNING correlation, eligible "
            "FIFO activation, third-owner rejection, alternating one-boundary "
            "turns, singleton-mailbox owner pinning, exact phase-owner release, "
            "cancel/deadline/budget precedence, peer-preserving retirement, "
            "and unaligned records; authoritative execution remains OS-owned"
        ),
        expected_exit_code=0,
        expected_stdout=(
            "OS-POST-EDU19 edu41-two-active snapshot=695ec66 vectors=79 "
            "digest=3199324360 result=PASS\n"
        ),
        expected_stderr="",
    ),
]

OBJECT_PROBES = [
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu21_control_v1_object",
        source=EDU21_CONTROL_POLICY_SOURCE,
        note=(
            "Exact EDU-21 Wire-v1 control validator must remain "
            "deterministic, freestanding, helper-free, hardware-blind, "
            "no-red-zone, and import-free"
        ),
        required_exports=("edu21_control_validate_request",),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu22_queue_v2_object",
        source=EDU22_QUEUE_POLICY_SOURCE,
        note=(
            "Exact EDU-22 queue/trace policy must remain deterministic, "
            "freestanding, helper-free, hardware-blind, no-red-zone, and "
            "import-free"
        ),
        required_exports=(
            "edu15_queue_entry_action",
            "edu15_queue_meta_valid",
            "edu22_queue_entry_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_two_owner_fault_composition_object",
        source=TWO_OWNER_COMPOSITION_MODEL_SOURCE,
        note=(
            "Two-owner fault-composition unit must remain deterministic, "
            "freestanding, hardware-blind, no-red-zone, and limited to the "
            "exact frozen EDU-40/41 ownership-helper imports"
        ),
        required_exports=(
            "edu42_completion_composed_valid",
            "edu42_dispatch_composed_valid",
            "edu42_policy_action_pair",
            "edu42_retirement_composed_valid",
        ),
        allowed_undefined=(
            "edu40_mailbox_completion_valid",
            "edu40_mailbox_dispatch_valid",
            "edu40_mailbox_retired_valid",
            "edu41_active_count",
            "edu41_choose_context",
            "edu41_mailbox_phase_owner",
            "edu41_policy_action",
            "edu41_release_phase_owner",
            "edu41_retirement_preserves_peer",
            "edu41_running_owner_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_durable_owner_chain_object",
        source=DURABLE_OWNER_CHAIN_MODEL_SOURCE,
        note=(
            "Durable owner-chain composition must remain deterministic, "
            "freestanding, hardware-blind, no-red-zone, and limited to the "
            "exact frozen EDU-26/35/37/39/40/41 helper imports"
        ),
        required_exports=(
            "edu43_checkpoint_owner_valid",
            "edu43_durable_owner_chain_valid",
            "edu43_generation_reuse_owner_valid",
            "edu43_phase_owner_active_valid",
        ),
        allowed_undefined=(
            "edu26_ack_identity_valid",
            "edu26_generation_reservable",
            "edu26_queue_entry_generation_action",
            "edu26_queue_meta_valid",
            "edu35_checkpoint_snapshot_valid",
            "edu37_checkpoint_lane_select",
            "edu37_checkpoint_storage_valid",
            "edu39_phase_owner_matches",
            "edu39_phase_owner_pair_valid",
            "edu40_mailbox_dispatch_valid",
            "edu41_active_count",
            "edu41_mailbox_phase_owner",
            "edu41_retirement_preserves_peer",
            "edu41_running_owner_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_temporal_fault_sequence_object",
        source=TEMPORAL_FAULT_SEQUENCE_MODEL_SOURCE,
        note=(
            "Temporal fault-sequence composition must remain deterministic, "
            "freestanding, hardware-blind, no-red-zone, and limited to the "
            "exact frozen ownership and durable-chain helper imports"
        ),
        required_exports=(
            "edu44_mid_phase_owner_loss_preserves_peer",
            "edu44_post_checkpoint_restart_rejects_stale",
            "edu44_post_completion_retirement_blocks_redispatch",
            "edu44_pre_ack_interruption_blocks_reuse",
        ),
        allowed_undefined=(
            "edu26_ack_identity_valid",
            "edu39_phase_owner_matches",
            "edu39_phase_owner_pair_valid",
            "edu40_mailbox_completion_valid",
            "edu40_mailbox_dispatch_valid",
            "edu40_mailbox_retired_valid",
            "edu41_active_count",
            "edu41_mailbox_phase_owner",
            "edu41_retirement_preserves_peer",
            "edu41_running_owner_valid",
            "edu43_checkpoint_owner_valid",
            "edu43_generation_reuse_owner_valid",
            "edu43_phase_owner_active_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_temporal_pair_fault_object",
        source=TEMPORAL_PAIR_FAULT_MODEL_SOURCE,
        note=(
            "Paired temporal-fault composition must remain deterministic, "
            "freestanding, hardware-blind, no-red-zone, and limited to the "
            "first temporal model plus exact frozen identity validators"
        ),
        required_exports=(
            "edu44_pair_checkpoint_interruption_blocks_reuse",
            "edu44_pair_completion_retirement_order_valid",
            "edu44_pair_owner_loss_peer_corruption_rejected",
            "edu44_stale_evidence_cross_generation_rejected",
            "edu44_unique_recovery_candidate_valid",
        ),
        allowed_undefined=(
            "edu26_ack_identity_valid",
            "edu40_mailbox_dispatch_valid",
            "edu43_checkpoint_owner_valid",
            "edu44_mid_phase_owner_loss_preserves_peer",
            "edu44_post_checkpoint_restart_rejects_stale",
            "edu44_post_completion_retirement_blocks_redispatch",
            "edu44_pre_ack_interruption_blocks_reuse",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_cross_model_temporal_composition_object",
        source=CROSS_MODEL_TEMPORAL_COMPOSITION_MODEL_SOURCE,
        note=(
            "Cross-model temporal composition must remain deterministic, "
            "freestanding, hardware-blind, no-red-zone, and import only the "
            "bounded Queue-v2, Wire-v7, scheduler, and temporal contracts"
        ),
        required_exports=(
            "edu45_cross_model_temporal_admission",
            "edu45_scheduler_handoff_valid",
        ),
        allowed_undefined=(
            "edu22_queue_entry_valid",
            "edu31_wire_v7_valid",
            "edu44_stale_evidence_cross_generation_rejected",
            "osp2_scheduler_choose",
            "osp2_scheduler_init",
            "osp2_scheduler_preemption_count",
            "osp2_scheduler_state",
            "osp2_scheduler_switch_count",
            "osp2_scheduler_yield_count",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu48_bundle_selection_object",
        source=EDU48_BUNDLE_SELECTION_MODEL_SOURCE,
        note=(
            "The source-derived EDU-48 frozen-program selection model must "
            "remain freestanding, hardware-blind, no-red-zone, and import "
            "only Workload-v1 validation plus deterministic reduction"
        ),
        required_exports=(
            "edu48_frozen_program_result",
            "edu48_frozen_program_valid",
        ),
        allowed_undefined=(
            "edu12_reduce_result",
            "edu32_workload_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
        scalar_sse2=True,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu23_parallelism_object",
        source=EDU23_PARALLELISM_POLICY_SOURCE,
        note=(
            "EDU-23 generated-C parallelism policy mirror must remain "
            "deterministic, freestanding, helper-free, hardware-blind, "
            "no-red-zone, and import-free"
        ),
        required_exports=(
            "edu23_admission_action",
            "edu23_compute_value",
            "edu23_entry_grant_valid",
            "edu23_grant_value",
            "edu23_path_evidence_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu24_artifact_exchange_object",
        source=EDU24_ARTIFACT_MODEL_SOURCE,
        note=(
            "EDU-24 assembly-derived artifact-exchange mirror must remain "
            "deterministic, freestanding, helper-free, hardware-blind, "
            "no-red-zone, and import-free"
        ),
        required_exports=(
            "edu24_artifact_chunk_final",
            "edu24_artifact_chunk_length",
            "edu24_artifact_chunk_offset",
            "edu24_artifact_request_valid",
            "edu24_artifact_response_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu24_31_wire_history_object",
        source=EDU24_31_WIRE_POLICY_SOURCE,
        note=(
            "Exact EDU-24..31 historical wire-policy slices must remain "
            "deterministic, freestanding, helper-free, hardware-blind, "
            "no-red-zone, and import-free"
        ),
        required_exports=(
            "edu24_wire_v2_valid",
            "edu26_wire_v3_valid",
            "edu28_wire_v4_valid",
            "edu29_wire_v5_valid",
            "edu30_wire_v6_valid",
            "edu31_wire_v7_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu25_loader_geometry_object",
        source=EDU25_LOADER_MODEL_SOURCE,
        note=(
            "EDU-25 assembly-derived loader-geometry mirror must remain "
            "deterministic, freestanding, helper-free, hardware-blind, "
            "no-red-zone, and import-free"
        ),
        required_exports=(
            "edu25_boot_geometry_valid",
            "edu25_transfer_chunk",
            "edu25_transfer_count",
            "edu25_transfer_destination",
            "edu25_transfer_lba",
            "edu25_transfer_plan_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu26_generation_reuse_object",
        source=EDU26_REUSE_POLICY_SOURCE,
        note=(
            "EDU-26 generated-C generation-reuse policy mirror must remain "
            "deterministic, freestanding, helper-free, hardware-blind, "
            "no-red-zone, and import-free"
        ),
        required_exports=(
            "edu26_ack_identity_valid",
            "edu26_generation_reservable",
            "edu26_queue_entry_generation_action",
            "edu26_queue_meta_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu27_phase_execution_object",
        source=EDU27_PHASE_MODEL_SOURCE,
        note=(
            "EDU-27 phase-execution mirror must remain deterministic, "
            "freestanding, helper-free, hardware-blind, no-red-zone, and "
            "import-free"
        ),
        required_exports=(
            "edu27_event_advance",
            "edu27_expected_phase_value",
            "edu27_phase_prefix_valid",
            "edu27_trace_prefix_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu28_artifact_meta_object",
        source=EDU28_ARTIFACT_POLICY_SOURCE,
        note=(
            "Exact EDU-28 artifact-metadata validator slice must remain "
            "deterministic, freestanding, helper-free, hardware-blind, "
            "no-red-zone, and import-free"
        ),
        required_exports=("edu28_artifact_meta_valid", "edu28_fnv1a32"),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu29_30_async_stop_object",
        source=EDU29_30_ASYNC_STOP_MODEL_SOURCE,
        note=(
            "EDU-29/30 assembly-derived async activation and stop mirror "
            "must remain deterministic, freestanding, helper-free, "
            "hardware-blind, no-red-zone, and import-free"
        ),
        required_exports=(
            "edu29_activation_select",
            "edu29_runner_identity_valid",
            "edu30_budget_terminal_valid",
            "edu30_budget_value",
            "edu30_cancel_terminal_valid",
            "edu30_stop_action",
            "edu30_work_shape_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu31_time_arithmetic_object",
        source=EDU31_TIME_MODEL_SOURCE,
        note=(
            "EDU-31 assembly-derived monotonic-time arithmetic mirror must "
            "remain deterministic, freestanding, helper-free, "
            "hardware-blind, no-red-zone, and import-free"
        ),
        required_exports=(
            "edu31_calibration_pair_hz",
            "edu31_calibration_window_hz",
            "edu31_delta_to_ns",
            "edu31_raw_delta",
            "edu31_read_admissible",
            "edu31_snapshot_flags",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_workload_v1_policy_object",
        source=POLICY_SOURCE,
        note=(
            "OS post-EDU-19 Workload-v1 policy object from immutable EDU-32 "
            "must remain deterministic, freestanding, helper-free, "
            "hardware-blind, no-red-zone, and scalar-SSE2 enabled"
        ),
        required_exports=(
            "edu12_simulate_partition",
            "edu12_reduce_result",
            "edu32_workload_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
        scalar_sse2=True,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_result_v1_policy_object",
        source=RESULT_POLICY_SOURCE,
        note=(
            "OS post-EDU-19 Result-v1 policy object from immutable EDU-33 "
            "must remain deterministic, freestanding, helper-free, "
            "hardware-blind, and no-red-zone"
        ),
        required_exports=("edu33_result_payload_valid",),
        allowed_relocations=("R_X86_64_PC32",),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu34_deadline_entry_object",
        source=EDU34_POLICY_SOURCE,
        note=(
            "Complete immutable EDU-34 queue policy object must remain "
            "deterministic, freestanding, helper-free, hardware-blind, and "
            "no-red-zone across the embedded deadline-entry validator"
        ),
        required_exports=(
            "edu15_queue_meta_valid",
            "edu22_queue_entry_valid",
            "edu26_queue_entry_generation_action",
            "edu15_queue_entry_action",
            "edu33_result_payload_valid",
        ),
        allowed_relocations=("R_X86_64_PC32",),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu35_checkpoint_snapshot_object",
        source=EDU35_POLICY_SOURCE,
        note=(
            "Immutable EDU-35 checkpoint validator object must remain "
            "deterministic, freestanding, helper-free, hardware-blind, and "
            "no-red-zone"
        ),
        required_exports=("edu35_checkpoint_snapshot_valid",),
        allowed_relocations=("R_X86_64_PC32",),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu36_resume_entry_object",
        source=EDU36_POLICY_SOURCE,
        note=(
            "Immutable EDU-36 Entry-v8 resume validator object must remain "
            "deterministic, freestanding, helper-free, hardware-blind, and "
            "no-red-zone"
        ),
        required_exports=("edu22_queue_entry_valid",),
        allowed_relocations=("R_X86_64_PC32",),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu37_two_owner_store_object",
        source=EDU37_MODEL_SOURCE,
        note=(
            "EDU-37 assembly-derived two-owner contract mirror must remain "
            "deterministic, freestanding, hardware-blind, no-red-zone, and "
            "limited to the exact inherited snapshot/workload validators"
        ),
        required_exports=(
            "edu37_checkpoint_storage_valid",
            "edu37_checkpoint_lane_select",
        ),
        allowed_undefined=(
            "edu32_workload_valid",
            "edu35_checkpoint_snapshot_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu38_control_v13_object",
        source=EDU38_CONTROL_POLICY_SOURCE,
        note=(
            "Complete immutable EDU-38 control policy object must remain "
            "deterministic, freestanding, helper-free, hardware-blind, and "
            "no-red-zone"
        ),
        required_exports=(
            "edu21_control_validate_request",
            "edu28_artifact_meta_valid",
            "edu28_fnv1a32",
        ),
        allowed_relocations=("R_X86_64_PC32",),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu38_runner_context_object",
        source=EDU38_CONTEXT_MODEL_SOURCE,
        note=(
            "EDU-38 assembly-derived runner-context mirror must remain "
            "deterministic, freestanding, helper-free, hardware-blind, and "
            "no-red-zone"
        ),
        required_exports=(
            "edu38_runner_context_for_slot",
            "edu38_runner_find_active",
            "edu38_runner_contexts_valid",
        ),
        allowed_relocations=("R_X86_64_PC32",),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu39_phase_owner_object",
        source=EDU39_PHASE_OWNER_MODEL_SOURCE,
        note=(
            "EDU-39 assembly-derived phase-owner mirror must remain "
            "deterministic, freestanding, hardware-blind, no-red-zone, and "
            "limited to the exact inherited Workload-v1 validator import"
        ),
        required_exports=(
            "edu39_phase_inflight",
            "edu39_phase_owner_for_context",
            "edu39_phase_owner_loadable",
            "edu39_phase_owner_matches",
            "edu39_phase_owner_pair_valid",
            "edu39_phase_owner_record_valid",
            "edu39_phase_publication_path_class",
            "edu39_phase_switch_allowed",
        ),
        allowed_undefined=("edu32_workload_valid",),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu40_mailbox_owner_object",
        source=EDU40_MAILBOX_OWNER_MODEL_SOURCE,
        note=(
            "EDU-40 assembly-derived AP-mailbox mirror must remain "
            "deterministic, freestanding, helper-free, hardware-blind, "
            "no-red-zone, and import-free"
        ),
        required_exports=(
            "edu40_mailbox_ap_accepts",
            "edu40_mailbox_completion_publishable",
            "edu40_mailbox_completion_valid",
            "edu40_mailbox_dispatch_valid",
            "edu40_mailbox_idle_can_begin",
            "edu40_mailbox_next_generation",
            "edu40_mailbox_reset_valid",
            "edu40_mailbox_retired_valid",
            "edu40_mailbox_unchanged",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
    ObjectProbe(
        probe_id="15__probe_os_post_edu19_edu41_two_active_runner_object",
        source=EDU41_TWO_ACTIVE_MODEL_SOURCE,
        note=(
            "EDU-41 assembly-derived two-active-runner mirror must remain "
            "deterministic, freestanding, helper-free, hardware-blind, "
            "no-red-zone, and import-free"
        ),
        required_exports=(
            "edu41_activation_select",
            "edu41_active_count",
            "edu41_choose_context",
            "edu41_mailbox_phase_owner",
            "edu41_next_turn",
            "edu41_policy_action",
            "edu41_release_phase_owner",
            "edu41_retirement_preserves_peer",
            "edu41_running_owner_valid",
        ),
        allowed_relocations=("R_X86_64_PC32", "R_X86_64_PLT32"),
        forbidden_instructions=HARDWARE_BLIND_FORBIDDEN,
    ),
]

DIAG_PROBES = []
DIAG_JSON_PROBES = []
