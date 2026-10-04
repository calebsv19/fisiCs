# ESP Sensor/CAN Static Topology Pass 2

Status: complete through deterministic static compatibility proof

Last verified: 2026-09-10

## Outcome

Pass 2 establishes one bounded three-node ESP/CAN topology using current
ESP-owned protocol and admitted simulation-profile inputs. It is a compiler and
static-graph result. It did not execute a simulation, contact hardware, inspect
the current wired CAN line, flash firmware, or measure termination resistance.

The selected topology is:

```text
distance endpoint               environment middle               supervisor endpoint
ESP32-C6 + HC-SR04              ESP32-C6 + AHT20                 ESP32-C6 observer
TJA1050 + termination intent    TJA1050, no termination          TJA1050 + termination intent
          \__________________ Classical CAN 500 kbit/s __________________/
```

The termination devices represent authored 120-ohm intent. They do not prove
component value, installation, removal, continuity, or effective bus resistance.

The distance-node fixture allocates ultrasonic trigger to GPIO4 and a
`protected_5v` echo endpoint to GPIO5. This is a new static candidate allocation
chosen to avoid the CAN TX/RX allocation on GPIO10/GPIO11. It is not a claim
that the earlier E004 bench was rewired: E004 used GPIO10/GPIO11 and its
documented echo divider. Any physical adoption must re-check ESP32-C6 pin
suitability and retain the required 5 V echo protection before wiring.

## Identity And Provenance

The fixed external contract pins exactly five ESP-owned inputs:

- current `protocol/schema/protocol_v1.yaml`;
- the admitted AHT20 profile and binding; and
- the admitted ultrasonic profile and binding.

The runner verifies their SHA-256 digests, required protocol semantic types and
instances, profile IDs, adapters, admitted state, and explicit
`physical_hardware: false` boundaries. It checks those exact files again after
graph construction and requires the ESP Git status to remain unchanged. It does
not recursively fingerprint unrelated retained files.

The active ESP checkout remained at the same Git commit throughout the pass.
One additional untracked status entry appeared between the earlier Pass 1
inventory and Pass 2 closeout, outside the bounded final gate. The final gate
proved its complete ESP status remained unchanged internally and did not adopt
or modify the concurrent entry.

The protocol source byte digest and generated schema identity are deliberately
separate: the pinned source file is
`342db8e1521df7a1616ac0a76433f7eb053510b03c481afeaca12984eace4eaf`,
while both admitted profiles name generated protocol identity
`2970b24a30d90795ac5f14c2953cc7f1cbe0c437873ed23ce241f115755e921f`.
The gate requires that agreement rather than incorrectly comparing a source-file
digest with the generated schema identity.

The C metadata carriers are compiler-owned declarations. They provide stable
semantic identities and units without claiming that `fisiCs` parsed the
Arduino C++ firmware.

## Canonical Physical Identities

Pass 2 uses these identities for new graph work:

| Retained mixed-bundle identity | New canonical identity |
| --- | --- |
| `quantity.dc.temperature` | `quantity.environment.air_temperature.absolute` |
| `quantity.dc.relative_humidity` | `quantity.environment.relative_humidity` |

The first is absolute temperature represented in kelvin. The second is a
dimensionless scalar; wire-level percent-RH conversion remains an execution or
protocol boundary. The retained 0.6.0 mixed bundle is not rewritten.

Ultrasonic distance uses `quantity.distance.length` with exact unit `meter`.

The external contract also makes the protocol-to-physical mapping explicit:

| Protocol type and instance | Semantic identity | Raw conversion into physical representation |
| --- | --- | --- |
| `DISTANCE` / `BENCH_ULTRASONIC` | `signal.bench.ultrasonic.distance` | raw x 1/1000 meter |
| `AIR_TEMPERATURE` / `BENCH_ENVIRONMENT_REAR_LEFT` | `signal.bench.environment.air_temperature` | raw x 1/100 + 273.15 kelvin |
| `RELATIVE_HUMIDITY` / `BENCH_ENVIRONMENT_REAR_LEFT` | `signal.bench.environment.relative_humidity` | raw x 1/10000 fraction |

These are authored mapping contracts, not proof that a runtime frame was
received or converted.

## Authored Inputs

The fixed authored fixture is under
`tests/fixtures/esp_can_topology_v1/`:

- `signals.c` declares the ultrasonic and environmental signals plus producers;
- `supervisor.c` declares one consumer of all three signals;
- `physical.catalog.json` defines quantities and physical contracts;
- `hardware.catalog.json` defines CAN/I2C buses, three board roles, devices,
  attachment ports, physical ports, and driver declarations;
- `hardware.topology.json` selects nodes, buses, devices, connections, and
  physical bindings; and
- `external.contract.json` pins the selected ESP inputs and expected topology.

Generated manifests, metadata, graphs, repeat graphs, objects, and the receipt
belong under `build/integration-esp-can-topology/`.

## Static Result

The focused gate resolves:

| Projection | Result |
| --- | --- |
| semantic records | 6 |
| semantic producer/consumer edges | 6 |
| nodes | 3 |
| buses | 2: one project-shared CAN and one middle-node-local I2C |
| devices | 8 |
| connections | 15 |
| signal bindings | 6 |
| unbound required ports | 0 |

The generated fisiCs 0.7.0 graph hashes from the verified run are:

| Artifact | SHA-256 |
| --- | --- |
| semantic project graph | `f66fd927586112b3e9c4c1aadca7968ee8d6ac02f13f657f821617d005f80de9` |
| physical composition graph | `f73898ace505643374408eba2769e0b71ab19ec33376ebbdd4a63730cf49e274` |
| hardware compatibility graph | `85d5bcd8d2e478f54146d1fc0613d6583c6ff19bdf1475257291582acfc9e6e4` |

Every graph is emitted twice and compared byte-for-byte.

## CAN Termination Decision

Hardware graph v1 can represent endpoint/middle roles as opaque board-profile
capabilities and can represent termination intent as passive devices attached to
the shared CAN bus. It does not contain a general rule language for asserting
that every CAN topology has exactly two termination devices.

Pass 2 therefore adds a bounded intent validator that requires:

- exactly two endpoint-role nodes;
- exactly one middle-role node;
- exactly two `120_ohm_intent-v1` termination devices;
- one termination owned by each endpoint node;
- no termination owned by the middle node;
- one TJA1050 declaration on every node; and
- one project-shared Classical CAN 500 kbit/s bus.

This is sufficient for the selected topology. A reusable arbitrary-CAN
cardinality policy would require a later additive schema or policy layer; it was
not smuggled into generic name inference.

## Negative Proof

The gate requires rejection of:

1. a missing endpoint termination;
2. a termination moved to the middle node;
3. an exclusive TWAI GPIO collision;
4. a missing required physical signal binding;
5. an AHT20 physical quantity mismatch; and
6. a CAN transport mismatch.

The first two are rejected by the bounded CAN intent validator. The remaining
four are rejected by the generic hardware compatibility graph.

## Run The Gate

From the `fisiCs` repository root:

```bash
make integration-esp-can-topology \
  ESP_WORK_ROOT=/absolute/path/to/ESP_Work
```

Success establishes compiler-validated semantic metadata, statically compatible
physical contracts, a statically compatible supplied hardware topology,
deterministic graph bytes, selected-input provenance, and six negative cases.

## Evidence Boundary And Next Layer

This pass establishes the **declared graph** only. The following remain
`not_assessed` or not executed:

- simulation execution and model behavior;
- runtime CAN traffic and observed publishers/consumers;
- firmware build, deployment, and installed identity;
- current wiring and component presence;
- actual termination resistance;
- calibration and sensor accuracy; and
- physical acceptance.

That next software layer is now implemented as
[`esp_can_simulation_pass3.md`](esp_can_simulation_pass3.md). It binds the exact
static graph hashes to repeated ESP-owned sensor-model executions and emits a
deterministic simulated observed-event projection. It still requires no
physical bench run. A bench lane is justified only for facts that remain
specifically electrical or physical after simulation and runtime-contract work.
