# ESP Sensor/CAN Receiver And Conformance Pass 5

Status: complete through deterministic software-only receiver proof

Last verified: 2026-09-11

## Outcome

Pass 5 consumes the exact Pass 4 transport projection, processes its baseline
deliveries through a bounded supervisor receive queue, validates the v1 payload
contract, enforces per-signal sequence rules, maintains latest-good state,
expires freshness, and emits compact replay slices for the visual debugger.

The pinned inputs and outputs are:

| Artifact | SHA-256 |
| --- | --- |
| Pass 4 transport | `366c64620d01fb4c38fc327bcef81a3e412728914f80df08954d36b0c2f18437` |
| Pass 5 receiver projection | `caf657b7c62ceba44da209feef08d1b1ac1a71289da709e0678c23c19ff56491` |
| Pass 5 debugger replay slices | `122a22e966fae44f47ed24faa148fca8bd32b327caffd1ef9702c873a4bf4520` |

No firmware, native application, ESP device, or physical CAN bus participates
in this pass.

## Receiver Pipeline

The modeled supervisor uses one FIFO RX queue with drop-newest overflow. A
frame is admitted only when:

- the configured payload codec is `fisiCs.sensor_scalar_le_v1`;
- its standard CAN identifier resolves to one authored semantic route;
- DLC is exactly eight;
- payload bytes decode with no reserved status bits; and
- its sequence is strictly newer than the last processed frame for that
  semantic identity.

Protocol admission and sensor usability remain separate. A correctly encoded
frame with invalid, stale, or unusable status advances received sequence state
but does not replace latest-good sensor state.

## Nominal Receiver Result

The nominal queue has capacity 16 and fixed 150 microsecond service time:

| Measure | Result |
| --- | --- |
| arrivals | 149 |
| processed | 149 |
| RX queue overflows | 0 |
| queue high-water | 1 |
| latest-good updates | 136 |
| accepted degraded samples | 3 |
| protocol rejections | 10 duplicate sequences |
| maximum receiver queue latency | 0 us |

The ten duplicate rejections are discovered behavior in the retained AHT20
fault/recovery output. Pass 3 preserved repeated sequence values, Pass 4
transported them faithfully, and Pass 5 correctly prevents them from rewriting
receiver state. This is not transport loss.

## Receiver Backpressure Result

The constrained receiver uses capacity two and 800 microsecond service time:

| Measure | Result |
| --- | --- |
| arrivals | 149 |
| processed | 139 |
| RX queue overflows | 10 |
| queue high-water | 2 |
| maximum queue latency | 1,552 us |
| latest-good updates | 127 |
| degraded samples | 4 |
| protocol rejections among processed frames | 8 |

This scenario proves supervisor-side backpressure independently of the
producer-side transmit queues modeled by Pass 4.

## Freshness And Latest-Good State

The receiver applies authored simulation timeouts:

- ultrasonic distance: 1.5 seconds;
- environmental temperature: 2.5 seconds; and
- environmental humidity: 2.5 seconds.

It emits eleven fresh/stale transitions. Ultrasonic distance expires at
3,501,128 microseconds after its last good simulated update. Repeated AHT20
sequences also create late-run environmental stale/recovery transitions. At the
70,000,490 microsecond inspection horizon, all three signals are stale while
their last good value and sequence remain retained. Stale state therefore does
not erase diagnostic history or make an old value usable.

## Protocol Conformance Vectors

Eleven deterministic vectors pass:

1. initial valid frame updates latest-good state;
2. duplicate sequence is rejected;
3. older sequence is rejected;
4. an apparent sequence reset is rejected as out of order;
5. unknown CAN identifier is rejected;
6. wrong DLC is rejected;
7. reserved status bits are rejected;
8. wrong configured codec is rejected;
9. invalid sample is admitted as degraded;
10. stale sample is admitted as degraded; and
11. unusable sample is admitted as degraded.

Six validator mutations additionally prove rejection of the wrong transport
hash, false latest-good counts, missing freshness transitions, altered
conformance dispositions, invented session identity, and zero RX capacity.

## Honest Session And Version Boundary

The current eight-byte v1 payload contains sequence, raw value, status, and
quality. It does not contain a source-session identity or an in-frame codec
version. Therefore:

- a receiver cannot distinguish a restarted producer at sequence zero from an
  old/out-of-order frame;
- Pass 5 rejects that reset rather than silently accepting it; and
- codec selection is currently an out-of-band route/configuration contract.

This is the main limitation of the Pass 5 custom codec. Repository
reconciliation subsequently confirmed that the retained ESP protocol already
has a versioned heartbeat with a 32-bit session ID. Pass 6 adopts that existing
contract; see
[`esp_can_protocol_session_pass6.md`](esp_can_protocol_session_pass6.md).
Signal frames still do not carry the session ID themselves, so producer queue
flush at restart remains an explicit protocol-v1 precondition.

## Debugger Replay Slices

`replay-slices.json` contains three compact, hash-bound views:

- the first-second arbitration and ACK-retry interval;
- all freshness transitions and final latest-good state; and
- all protocol conformance outcomes plus wire-capability gaps.

These are static replay inputs suitable for later native visualizer adoption.
Pass 5 does not claim that the current ESP Network Sim UI has loaded or rendered
them.

## Run The Complete Gate

```sh
make integration-esp-can-receiver \
  ESP_WORK_ROOT=/absolute/path/to/ESP_Work
```

This chains Pass 2 static graphs, Pass 3 ESP-owned sensor execution, Pass 4 CAN
transport, and Pass 5 supervisor receive validation from their earliest inputs.

## Full Compiler And Simulation State

The current integration is now a five-layer, one-directional evidence pipeline:

```text
portable C metadata declarations
  -> deterministic semantic / physical / hardware graphs
  -> ESP-owned AHT20 and ultrasonic host-model records
  -> deterministic Classical CAN frames, queues, arbitration and faults
  -> supervisor RX admission, latest-good state, freshness and debugger replay
```

The compiler does not translate Arduino C++ into ESP firmware. Instead, it owns
the stable semantic identities and static compatibility graphs. The ESP
simulation system owns sensor behavior. The Pass 3–5 adapters join those two
systems through exact hashes and fail closed when an input changes.

This makes the compiler metadata operationally useful today for:

- proving which node is authorized to produce or consume each signal;
- connecting physical units and hardware topology to simulated values;
- assigning protocol identifiers without losing semantic identity;
- tracing one value from simulated acquisition through wire payload to
  supervisor state;
- reproducing arbitration, queue, retry, freshness, and fault decisions; and
- supplying small deterministic replay slices to future debugging UIs.

It is not a deployment pipeline in the firmware sense. Source compilation,
vendor firmware build, flashing, installed identity, physical traffic, and
electrical acceptance remain separately owned evidence lanes.

## Authored And Generated Files

Authored inputs:

- `tests/fixtures/esp_can_receiver_v1/contract.json`;
- `tests/fixtures/esp_can_receiver_v1/README.md`; and
- `tests/integration/run_esp_can_receiver.py`.

Generated outputs:

- `build/integration-esp-can-receiver/receiver.json`;
- `build/integration-esp-can-receiver/replay-slices.json`; and
- `build/integration-esp-can-receiver/receipt.json`.

## Next Software Boundary

That compatibility pass is now complete in Pass 6 using the existing ESP
protocol-v1 heartbeat rather than inventing a payload v2. The next optional
software step is either an ESP-owned restart/queue-flush host test or native
visualizer adoption of the replay slices. Neither requires a bench run.
