# ESP Sensor/CAN Frame And Queue Transport Pass 4

Status: complete through deterministic software-only transport proof

Last verified: 2026-09-11

## Outcome

Pass 4 consumes the exact Pass 3 simulated-observation payload and turns every
semantic delivery into a bounded Classical CAN frame. It adds authored CAN
identifiers, a fixed payload codec, CRC-15, bit stuffing, cross-node
arbitration, per-node FIFO queues, simulation-time latency, controlled ACK-loss
faults, retry behavior, queue-overflow accounting, and bus-utilization
receipts.

The Pass 4 11-bit/little-endian codec is a purpose-built transport model, not
the retained ESP protocol-v1 wire format. Pass 6 preserves this behavioral
evidence while adding the actual extended-29-bit, big-endian ESP protocol
projection; see
[`esp_can_protocol_session_pass6.md`](esp_can_protocol_session_pass6.md).

The input is fixed to Pass 3 projection SHA-256
`e142643a7a361431ceac65e38a9b595be58c81bc33e26ad74e5687b922581a35`.
The verified transport projection SHA-256 is
`366c64620d01fb4c38fc327bcef81a3e412728914f80df08954d36b0c2f18437`.

No firmware was built or flashed, and no CAN adapter, ESP board, transceiver,
sensor, or wired bus was contacted.

## Authored CAN Identities

Pass 4 assigns standard 11-bit identifiers explicitly rather than deriving
priority from semantic-name spelling:

| CAN ID | Semantic identity | Producer | Consumer |
| --- | --- | --- | --- |
| `0x180` | `signal.bench.ultrasonic.distance` | distance endpoint | supervisor endpoint |
| `0x280` | `signal.bench.environment.air_temperature` | environment middle | supervisor endpoint |
| `0x281` | `signal.bench.environment.relative_humidity` | environment middle | supervisor endpoint |

Lower identifiers win cross-node arbitration. Each node's local queue is FIFO,
so arbitration occurs between the current head frame from each node rather
than allowing the simulator to reorder a producer's queue by priority.

## Payload And Frame Model

Every source observation becomes one eight-byte data payload:

```text
bytes 0..1  sequence, unsigned 16-bit little-endian
bytes 2..5  raw sensor value, signed 32-bit little-endian
byte 6      valid/fresh/usable status bits
byte 7      quality
```

The encoder requires every payload to decode back to the source observation.
For each standard data frame, the model constructs SOF, 11-bit identifier,
RTR, IDE, reserved bit, DLC, and data bits; computes CAN CRC-15 with polynomial
`0x4599`; applies stuffing from SOF through the CRC sequence; and includes CRC
delimiter, ACK slot, ACK delimiter, EOF, and intermission in the wire-bit and
duration totals.

ACK success or loss is a simulation outcome. The model does not claim to
reproduce electrical dominant/recessive sampling.

## Scheduling And Capacity

The existing sensor profiles use different time scales. Pass 4 therefore
authors a one-second publication boundary. Each source record keeps its
original acquisition time but is released on the next whole simulation-second
boundary. Receipts report two different latency measures:

- `transport_latency_us`: release to successful delivery; and
- `source_to_delivery_us`: original simulated acquisition to delivery,
  including publication batching.

Frames enter bounded per-node FIFO queues using drop-newest overflow policy.
The 500 kbit/s bus chooses the lowest CAN identifier among node queue heads,
transmits the complete stuffed frame, and then repeats arbitration. A retrying
frame remains at its queue head and re-enters arbitration after its modeled
intermission.

## Scenario Results

### Baseline with one transient ACK loss

| Measure | Result |
| --- | --- |
| source frames | 149 |
| delivered frames | 149 |
| transmit attempts | 150 |
| retry attempts | 1 |
| overflow drops | 0 |
| retry-exhausted drops | 0 |
| arbitration-loss events | 15 |
| maximum transport latency | 3,188 us |
| attempted wire bits | 18,260 |
| modeled horizon | 67,000,490 us |
| modeled utilization | 545 ppm, or 0.0545% |

The injected first-attempt ACK loss targets ultrasonic sequence 2. Its second
attempt succeeds, and every source frame reaches the supervisor.

### Constrained queue with persistent ACK loss

The distance-node capacity is reduced from 16 to 4 while its initial records
remain batched at the first-second boundary. The scenario produces:

- six deterministic drop-newest queue overflows;
- two attempts for ultrasonic sequence 2;
- one retry-exhausted final drop;
- 142 delivered frames; and
- 523 ppm modeled utilization.

This scenario is deliberately a capacity/fault proof, not a recommended
production queue size.

## Determinism And Negative Proof

The runner regenerates the complete transport result twice in memory and
requires exact bytes. It independently replays every scenario during
validation, then checks the pinned transport hash.

Nine mutations must be rejected:

1. wrong Pass 3 observation hash;
2. corrupted payload bytes;
3. incorrect CRC;
4. an attempt carrying the wrong CAN identifier;
5. negative latency;
6. incorrect utilization; and
7. a broken retry-attempt sequence;
8. duplicate CAN identifiers; and
9. an invalid zero-capacity queue.

## Run The Complete Gate

```sh
make integration-esp-can-transport \
  ESP_WORK_ROOT=/absolute/path/to/ESP_Work
```

The Make target first runs the complete Pass 3 gate. That regenerates Pass 2,
executes both ESP-owned sensor profiles twice, and verifies the pinned semantic
observation. Pass 4 then consumes only that exact observation and receipt.

## Authored And Generated Files

Authored inputs:

- `tests/fixtures/esp_can_transport_v1/contract.json`;
- `tests/fixtures/esp_can_transport_v1/README.md`; and
- `tests/integration/run_esp_can_transport.py`.

Generated outputs:

- `build/integration-esp-can-transport/transport.json`; and
- `build/integration-esp-can-transport/receipt.json`.

## Evidence Boundary

Pass 4 establishes deterministic **software simulation** of the authored frame,
queue, arbitration, fault, retry, latency, and utilization rules. It does not
establish:

- equivalence with a particular ESP32-C6 TWAI peripheral or driver;
- oscillator timing, controller register behavior, or hardware error states;
- transceiver voltage, propagation delay, electrical arbitration, reflections,
  termination, wiring, or electromagnetic noise;
- firmware build, task scheduling, deployment, or installed identity;
- physical CAN traffic or sensor accuracy; or
- real-time performance on the native visualizer.

That receiver refinement is now implemented as
[`esp_can_receiver_pass5.md`](esp_can_receiver_pass5.md). It adds protocol
conformance vectors, duplicate/out-of-order handling, freshness expiry,
supervisor queue backpressure, latest-good state, and replayable debugger
slices. It also proves that source-session restart cannot be disambiguated by
the current v1 payload because no session identity is carried. A hardware lane
remains unnecessary until actual driver or electrical equivalence is the
desired claim.
