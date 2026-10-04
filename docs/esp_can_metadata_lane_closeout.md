# ESP CAN Metadata And Simulation Lane Closeout

Status: historical initial software base preserved; authored source adopted
through compiler Main Edit stabilization

Last verified: 2026-09-11

## Purpose

This is the durable restart page for the completed initial fisiCs-to-ESP CAN
metadata and simulation lane. It records what is established, where the
authored contracts live, how to regenerate the generated receipts, what remains
outside the evidence boundary, and which refinement should be selected next.

For the reproducible CAN lane, start here instead of reconstructing chat
history. For the cross-system capability map and proposed execution-binding
sequence, use [ESP metadata integration](esp_metadata_integration_roadmap.md).

## Closeout State

The initial software base is established through six bounded passes:

1. preserve and reconcile the existing E016 metadata and retained evidence;
2. author and validate a deterministic three-node sensor/CAN static topology;
3. execute the ESP-owned AHT20 and ultrasonic host simulations and project
   their observations through the static graph identities;
4. model Classical CAN framing, queues, arbitration, capacity, latency,
   injected loss, retry behavior, and utilization;
5. model supervisor receive queues, payload admission, sequence handling,
   latest-good state, freshness, and debugger replay slices; and
6. reconcile the behavioral model with the actual ESP protocol-v1 extended
   identifiers, big-endian payloads, heartbeat versioning, and session state.

The result is a deterministic, digest-bound path:

```text
portable C and authored manifests
  -> fisiCs semantic metadata
  -> semantic / physical / hardware graphs
  -> ESP-owned host sensor simulation
  -> simulated observation projection
  -> behavioral CAN queue and receiver models
  -> actual ESP protocol-v1 signal and heartbeat projection
  -> session-aware debugger replay and receipts
```

This is a stable **software base**, not a released feature or physical CAN
acceptance. No compiler-core source was changed by Passes 1-6.

## Canonical Reading Order

| Order | Document | Role |
| --- | --- | --- |
| 1 | [ESP compiler bridge case study](esp_compiler_bridge_case_study.md) | Explains how ordinary shared C is consumed by fisiCs and the ESP toolchain without adding a metadata runtime. |
| 2 | [Pass 1 preservation checkpoint](esp_metadata_preservation_checkpoint.md) | Protects the existing worktrees, retained E016 evidence, and evidence boundaries. |
| 3 | [Pass 2 static topology](esp_can_topology_pass2.md) | Owns the three-node graph, component catalogs, signal bindings, and termination intent. |
| 4 | [Pass 3 sensor simulation](esp_can_simulation_pass3.md) | Owns the exact ESP-host simulation intake and semantic observation projection. |
| 5 | [Pass 4 CAN transport](esp_can_transport_pass4.md) | Owns the custom behavioral queue/arbitration/fault model. |
| 6 | [Pass 5 receiver](esp_can_receiver_pass5.md) | Owns the custom behavioral receiver, freshness, and replay model. |
| 7 | [Pass 6 protocol/session](esp_can_protocol_session_pass6.md) | Owns compatibility with the actual ESP protocol-v1 wire format and the stable-base decision. |
| 8 | [ESP Network Sim growth architecture](esp_network_sim_growth_architecture.md) | Defines the future simulator/compiler ownership boundary and application growth path. |

The repository-wide extension contract remains
[the extension agent guide](agent_extension_guide.md). It defines the evidence
vocabulary and the minimum validation command for each layer.

## Authored Source Of Truth

The Pass 2-6 inputs are authored repository fixtures and runners:

| Pass | Authored fixtures | Runner |
| --- | --- | --- |
| 2 | `tests/fixtures/esp_can_topology_v1/` | `tests/integration/run_esp_can_topology.py` |
| 3 | `tests/fixtures/esp_can_simulation_v1/` | `tests/integration/run_esp_can_simulation.py` |
| 4 | `tests/fixtures/esp_can_transport_v1/` | `tests/integration/run_esp_can_transport.py` |
| 5 | `tests/fixtures/esp_can_receiver_v1/` | `tests/integration/run_esp_can_receiver.py` |
| 6 | `tests/fixtures/esp_can_protocol_session_v1/` | `tests/integration/run_esp_can_protocol_session.py` |

The Make targets in `makefile` are the public execution surface. Do not edit
generated files under `build/`; regenerate them from these fixtures and
runners.

## Pinned Artifact Chain

The 2026-09-11 closeout run established these deterministic identities:

| Layer | SHA-256 |
| --- | --- |
| Pass 2 semantic graph | `f66fd927586112b3e9c4c1aadca7968ee8d6ac02f13f657f821617d005f80de9` |
| Pass 2 physical graph | `f73898ace505643374408eba2769e0b71ab19ec33376ebbdd4a63730cf49e274` |
| Pass 2 hardware graph | `85d5bcd8d2e478f54146d1fc0613d6583c6ff19bdf1475257291582acfc9e6e4` |
| Pass 3 simulated observation | `e142643a7a361431ceac65e38a9b595be58c81bc33e26ad74e5687b922581a35` |
| Pass 4 behavioral transport | `366c64620d01fb4c38fc327bcef81a3e412728914f80df08954d36b0c2f18437` |
| Pass 5 behavioral receiver | `caf657b7c62ceba44da209feef08d1b1ac1a71289da709e0678c23c19ff56491` |
| Pass 5 replay slices | `122a22e966fae44f47ed24faa148fca8bd32b327caffd1ef9702c873a4bf4520` |
| Pass 6 protocol/session projection | `ca9ad1479d14c1e313ef648d523c4cbc261ced34a77d94e063cbe1d9ea36da08` |
| Pass 6 replay slices | `dff8b1229c833224907083d852a87aaa6aee511d893fab99f91d779cff66a5a4` |

Pass 6 also pins ESP protocol schema, generated-header, and golden-vector
digests in `tests/fixtures/esp_can_protocol_session_v1/contract.json`. The
external ESP snapshot used by the closeout run was commit
`2b8b11a9be092bb6a75f153aded12222f6616ab1`; its Git status and selected inputs
were unchanged by the gate. These are exact input identities, not a statement
that the moving ESP checkout will remain at that state.

## Regenerate And Verify

From the fisiCs repository root, run:

```bash
make integration-esp-can-protocol-session \
  ESP_WORK_ROOT=/absolute/path/to/ESP_Work
```

This single target runs the complete Pass 2-6 dependency chain. Generated
receipts are written to:

```text
build/integration-esp-can-topology/receipt.json
build/integration-esp-can-simulation/receipt.json
build/integration-esp-can-transport/receipt.json
build/integration-esp-can-receiver/receipt.json
build/integration-esp-can-protocol-session/receipt.json
```

`build/` is intentionally ignored. The authored contracts and documentation
are durable; the receipts are reproducible products. A future run must fail
closed if an expected external source or protocol digest has changed. Review
and intentionally repin the authored contract only after assessing that drift.

The closeout run passed with:

- Pass 3: 2 profiles, 4 executions, 149 records, 3 degraded or stale records,
  and 6 negative cases;
- Pass 4: 149 baseline deliveries, 1 retry, 15 arbitration losses, 6 stress
  queue overflows, 1 retry-exhausted drop, and 9 negative cases;
- Pass 5: 149 nominal records processed, 136 latest-good updates, 3 degraded
  updates, 10 constrained-queue overflows, 11 freshness transitions, 11
  conformance vectors, and 6 negative cases; and
- Pass 6: 149 signals, 136 heartbeats, 2 sessions, 134 refreshes, 11
  conformance timelines, and 5 negative cases.

Pass 6 also ran the ESP-owned `make test-protocol` gate: generated output was
fresh, the C++ codec tests passed, and all 16 Python protocol tests passed.

## Evidence Established

| Evidence plane | Closeout result |
| --- | --- |
| Authored | Portable-C mappings, semantic identities, physical contracts, component catalogs, topology, simulation contracts, protocol mapping, and conformance cases are present. |
| Compiler-validated | Current Pass 2 sources produce valid semantic metadata and resolved project graphs. |
| Statically compatible | Current physical and hardware graphs complete deterministically for the supplied catalogs and topology. |
| Simulated | ESP-host sensor behavior, behavioral CAN transport/receiver behavior, and protocol-v1 session behavior execute deterministically. |
| Source compatible | The independent Pass 6 projection matches the retained ESP protocol-v1 golden vectors and the ESP-owned codec tests pass. |
| Runtime-observed on ESP hardware | Not assessed by this lane. |
| Physically accepted | Not assessed by this lane. |
| Packaged, released, deployed, or flashed | Not performed by this lane. |

Passes 4 and 5 must remain labeled as behavioral predecessors. Their
11-bit/little-endian codec is useful for deterministic arbitration, queue,
fault, receiver, and freshness evidence, but it is not the actual ESP wire
format. Pass 6 is the protocol-compatible projection: extended 29-bit CAN IDs,
big-endian eight-byte payloads, and heartbeat session identity.

## Known Limitation

Protocol v1 carries the session ID in heartbeats, not in each signal frame.
The receiver can reset sequence state after a new heartbeat, but a delayed
signal cannot prove which producer session created it. The retained operational
precondition is therefore:

```text
On restart, flush pending signal frames before transmitting the new-session heartbeat.
```

The Pass 6 conformance receipt demonstrates the boundary. It does not claim
per-frame session attribution.

## Safe Future Restart

Before changing this lane:

1. inspect `git status --short` in both repositories and preserve unrelated or
   concurrent work;
2. read this closeout and the Pass 6 contract;
3. run the complete Pass 2-6 target against the intended ESP root;
4. treat a digest mismatch as source drift to review, not as a hash to bypass;
5. choose one bounded successor; and
6. keep simulator adoption, ESP implementation, hardware testing, release,
   and deployment as separately owned actions.

The smallest useful software successors are:

1. enforce and host-test the restart queue-flush rule in the ESP-owned
   transport implementation; or
2. ingest the Pass 6 replay slices through the native visualizer's versioned
   trace adapter and verify the same receiver projections.

An optional protocol-v2 design may add per-frame session attribution if that
stronger guarantee becomes necessary. It is not required to use the current
stable base.

No new bench run is required to close or reproduce this compiler lane. Bench
work is justified only by a specifically named electrical, deployed-runtime,
calibration, or physical-acceptance question that software evidence cannot
answer.

## Worktree And Preservation Boundary

At closeout, the Pass 1-6 documentation, fixtures, runners, and Make targets
remain uncommitted in the fisiCs worktree. They must be treated as protected
work: do not clean, reset, overwrite, stage, or commit them implicitly. A
future commit is a separate source-control decision.

The validation target reads the ESP repository and runs its host protocol
tests, but it must leave that repository's Git status unchanged. It does not
build firmware, open a serial port, contact a board, transmit on a physical CAN
bus, or change the native visualizer.

## 2026-10-04 source stabilization

The compiler Main Edit adoption preserves these authored contracts and their
historical pins. Its compiler-generated attachment and frozen-input gates pass
from the isolated checkout. `make test-esp-aht20-binding-contract` now produces
fresh graphs in a temporary directory and checks the declaration/projection
contract without requiring previous build artifacts or external ESP execution.
Those unit controls bind their own generated bytes and do not replace the
historical execution pins.

Replaying the full CAN protocol/session gate against a temporary copy of the
current ESP source (`5f381f9930a00f7bff0111b368bb79a3aaa2c245`) rejects the
`protocol_schema` external-input digest before execution. The moving ESP source
has drifted from the historical fixture. This is an expected admission boundary,
not current end-to-end CAN requalification. Original fixtures and graph/run pins
remain unchanged. Restoring the exact captured inputs or a separately approved
contract migration is required before claiming a new full CAN or AHT20 execution
pass. The live ESP checkout was not modified by this stabilization.
