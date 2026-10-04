# ESP Sensor/CAN Software Simulation Pass 3

Status: complete through deterministic software-only projection proof

Last updated: 2026-09-10

## Purpose

Pass 3 turns the exact Pass 2 declared graph into a software-executed behavior
receipt without crossing into firmware or bench claims. It runs the admitted
ESP-owned AHT20 and ultrasonic host profiles, selects the sensor records named
by the Pass 2 protocol mapping, and delivers those records across the declared
three-node CAN topology in one deterministic projection.

This is the first **simulated** evidence layer for this topology. It is not
runtime observation from ESP boards and it is not physical acceptance.

## What Executes

The gate invokes the ESP repository's own `control_data` plan/run boundary for:

- `environment.aht20.v1`, scenario `environment`; and
- `distance.ultrasonic.v1`, scenario `proximity`.

Each profile executes twice in new temporary output roots. Both plans and both
`result/run.json` payloads must be byte-identical. The expected run-payload
hashes are pinned in the Pass 3 contract, and the ESP execution receipt must
join the same run hash while declaring `host_simulation` and
`physical_hardware: false`.

The gate retains only the two deterministic run payloads and a derived
projection under `build/integration-esp-can-simulation/`. Temporary compiler
and runtime products remain execution evidence owned by the upstream ESP
adapter and are not treated as authored fisiCs inputs.

## Projection

The software adapter selects:

- rear-left AHT20 instance 12 temperature and humidity; and
- every ultrasonic proximity record's raw millimeter distance.

It retains each source record's simulated time, sequence, validity, quality,
freshness, usability, session where present, and raw value. Exact rational
conversions apply the Pass 2 mapping:

| Semantic identity | Source value | Projected physical value |
| --- | --- | --- |
| `signal.bench.ultrasonic.distance` | `raw_mm` | raw / 1000 meter |
| `signal.bench.environment.air_temperature` | `temperature.raw` | raw / 100 + 273.15 kelvin |
| `signal.bench.environment.relative_humidity` | `humidity.raw` | raw / 10000 fraction |

The resulting observed-edge projection requires all three declared
producer-node to supervisor-node relationships. Invalid or stale model states
are retained with `usable: false`; they are not silently removed.

The verified projection contains 149 delivered semantic records: 146 usable
and three retained invalid or stale ultrasonic states. Its pinned SHA-256 is
`e142643a7a361431ceac65e38a9b595be58c81bc33e26ad74e5687b922581a35`.

## CAN Model Boundary

Pass 3 models deterministic semantic delivery on
`bus.can.sensor_backbone` at the declared 500 kbit/s identity. The current ESP
family profiles are independent sensor simulations, so the fisiCs adapter
provides the cross-profile scheduling and graph join.

It does not model:

- arbitration among simultaneous CAN identifiers;
- frame layout, CRC, bit stuffing, retransmission, or queue pressure;
- transceiver voltage, termination resistance, wiring, or noise;
- ESP firmware scheduling or driver execution; or
- wall-clock real-time behavior.

Those omissions are explicit fields in the authored contract and final
receipt. Consequently the result supports a simulated semantic-network claim,
not a full CAN-controller or electrical-network claim.

## Negative Proof

The focused gate must reject six mutations:

1. a wrong exact static hardware graph hash;
2. an unmapped semantic identity;
3. an undeclared publisher node;
4. a per-signal sequence regression;
5. an incorrect raw-to-physical conversion; and
6. an undeclared consumer node.

Pass 2 still owns topology, GPIO, physical quantity, transport-profile, and
termination-intent negatives. Pass 3 does not duplicate those policies.

## Run The Gate

From the fisiCs repository root:

```sh
make integration-esp-can-simulation \
  ESP_WORK_ROOT=/absolute/path/to/ESP_Work
```

The runner first regenerates and validates Pass 2, including its exact graph
hashes and unchanged-ESP-tree requirement. It then executes four total profile
runs, derives the projection twice, checks six negative cases, and requires the
ESP Git status to remain byte-identical across the complete operation.

## Authored And Generated Files

Authored Pass 3 inputs:

- `tests/fixtures/esp_can_simulation_v1/contract.json`;
- `tests/fixtures/esp_can_simulation_v1/README.md`; and
- `tests/integration/run_esp_can_simulation.py`.

Generated readback:

- `build/integration-esp-can-simulation/environment-aht20-v1.run.json`;
- `build/integration-esp-can-simulation/distance-ultrasonic-v1.run.json`;
- `build/integration-esp-can-simulation/simulated-observation.json`; and
- `build/integration-esp-can-simulation/receipt.json`.

## Evidence Boundary And Next Refinement

On a passing gate, the compiler state is usable as a deterministic metadata
and simulation join: ordinary C metadata carriers declare stable identities,
the compiler emits exact semantic/physical/hardware graphs, ESP-owned models
produce behavior records, and this adapter checks those records against the
declared routes.

That frame/queue refinement is now implemented as
[`esp_can_transport_pass4.md`](esp_can_transport_pass4.md). It consumes the
same pinned projection and adds CAN identifiers, payload encoding, CRC and bit
stuffing, arbitration, capacity, latency, controlled loss/retry, and bus-load
receipts while remaining hardware-free. A later firmware-host or bench lane
would be needed only to claim actual driver behavior, flashed identity, real
traffic, electrical behavior, or sensor accuracy.
