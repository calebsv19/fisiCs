# ESP Metadata Preservation And Reconciliation Checkpoint

Status: Pass 1 preservation baseline established; Pass 2 topology completed by
`docs/esp_can_topology_pass2.md`

Observed: 2026-09-10

This document is the historical preservation baseline. For the completed
Pass 1-6 state and future restart procedure, see
[ESP CAN Metadata And Simulation Lane Closeout](esp_can_metadata_lane_closeout.md).

## Purpose

Preserve the current boundary between the `fisiCs` compiler, the ESP simulation
profiles, retained static graph artifacts, native simulator integration, and
future physical CAN work before authoring a larger multi-node topology.

This checkpoint is an inventory and adoption map. It does not adopt the whole
ESP working tree, rewrite retained artifacts, contact hardware, or promote
simulation evidence into runtime or physical evidence.

## Repository Provenance

| Surface | Observed Git state | Interpretation |
| --- | --- | --- |
| `fisiCs` | `main` at `5341b675ace4010acf46bbc5895e431fbe3fa80c`, clean, compiler `0.7.0` | Current compiler authority for future regeneration |
| `ESP_Work` | `main` at `2b8b11a9be092bb6a75f153aded12222f6616ab1`, 14 tracked changes and 68 untracked status entries at Pass 2 closeout | Active multi-lane workspace; not safe for a blanket checkpoint commit |
| ESP Network Sim Main Edit | `codex/esp_network_sim-main-edit` at `897742ec49a1d8c9df17fc7e2af682fe3349b345`, clean | Protected source checkpoint containing the newer generic backend inspection work |

The initial Pass 1 ESP status projection had SHA-256
`725802bd38939b4d56175eafed9f2a5c890935369d5c39fcd743ea3abe49a685`.
This hash identifies the Git status listing only; it is not a content digest of
the dirty worktree and cannot reconstruct its bytes.

During Pass 2, another untracked ESP status entry appeared outside the bounded
compiler gate. The closeout status projection is
`74a853bc8a542af7645286ba338a009c0c0de55cdadc2ef57b4e7cc500874f81`.
The Pass 2 runner does not attribute or adopt that concurrent work: its final
run proved the five selected external inputs and the complete porcelain-v2 Git
status stayed unchanged from the start to the end of that run.

No broad ESP commit is part of this compiler-owned pass. The changed and
untracked paths span device, sensor, protocol, simulation, documentation, and
evidence lanes and require ESP-owned classification before staging.

## Preserved Metadata Layers

### Direct portable-C metadata

Direct `FISICS_*` source annotation remains intentionally narrow. The current
ESP source matches are the vendored portability header and
`NormalizedSignalPolicy.h`. The E016 carrier remains the established example of
one ordinary-C policy compiled by strict Clang, `fisiCs`, and the separate ESP
vendor toolchain.

This does not mean that `fisiCs` parses the broader Arduino C++ firmware.
Generated declaration-only C carriers and external manifests remain the honest
bridge for C++-owned systems.

### Retained mixed-system static graph

The admitted `dc.mixed_timed.v1` profile retains a compiler-authored graph
bundle and an independent execution binding:

| Item | SHA-256 |
| --- | --- |
| assigned graph bundle | `3d19500482e01c8b38fc5bfb2461923b71df8e35bdd9e25d4002e7ff39b70c8d` |
| semantic graph in bundle | `6e61c4b16897681e024ae4cc5e74d222c45852979c28711c1d9cad0b56331f6e` |
| physical graph in bundle | `2a302a0e9ddfc716c59bd7f513dc4c59db285afd31735c981934a9b70873a8ed` |
| hardware graph in bundle | `efa7ef5625228948269337575f1720ffa375bc81156baa2f8eb8998b1dafabfe` |
| execution binding file | `39cf65410bd8998582418c18c109dd9aa530142ed565aa2fa7b07b16ca6cd17e` |
| graph builder source | `1e308df701584be6604e15e6c2e17cb10770b979cf47df7ad61118a298a7e78b` |
| execution-binding source | `045883bc5c4310b19ee3ffd915fcbd8c19f211a487d9047a0c1ddf8e8048d4dd` |

The bundle records producer version `fisiCs 0.6.0` and analysis contract
`fisiCs.analysis.contract` `1.9.0`. It is retained historical evidence. It must
not be edited or relabelled as a `0.7.0` artifact. A `0.7.0` projection must be
freshly generated from authored inputs into a separate output location.

The execution binding remains a separate artifact because static graph
compatibility does not establish that a simulator model executed.

## Simulation Profile Adoption Matrix

All listed profiles are admitted by the ESP-owned registry. "Compiler graph
coverage" refers to an exact profile or component identity represented in a
retained `fisiCs` static graph; it does not refer merely to having units in
simulator code.

| Profile | Adapter | Simulation metadata | Compiler graph coverage at this checkpoint |
| --- | --- | --- | --- |
| `dc.mixed_timed.v1` | `dc_timed_v1` | admitted | exact retained mixed bundle, produced by `fisiCs 0.6.0` |
| `environment.aht20.v1` | `family_v1` | admitted | AHT20 component represented in the retained mixed bundle; family profile is not independently graph-bound |
| `lighting.lm393.v1` | `family_v1` | admitted | not yet adopted |
| `distance.ultrasonic.v1` | `family_v1` | admitted | not yet adopted |
| `input.joystick.v1` | `family_v1` | admitted | not yet adopted |
| `lighting.freenove.v1` | `family_v1` | admitted | not yet adopted |
| `lighting.joystick_pixel.v1` | `family_v1` | admitted | not yet adopted |
| `motion.servo.v1` | `family_v1` | admitted | not yet adopted |
| `motion.joystick_servo.v1` | `family_v1` | admitted | not yet adopted |
| `control.joystick_servo_pixel.v1` | `family_v1` | admitted | not yet adopted |
| `input.mpu6050.v1` | `family_v1` | admitted | not yet adopted |
| `input.mpu6050_tilt.v1` | `family_v1` | admitted | not yet adopted |
| `input.mpu6050_fusion.v1` | `family_v1` | admitted | not yet adopted |
| `input.mpu6050_conditioned.v1` | `family_v1` | admitted | not yet adopted |
| `pwm.led_reference.v1` | `pwm_led_v1` | admitted | not yet adopted |
| `lighting.coupled.v1` | `coupled_lighting_v1` | admitted | not yet adopted |

The matrix deliberately does not require every simulator profile to receive a
compiler graph. Compiler adoption should occur at stable physical and topology
boundaries where identity, units, placement, address, bus, driver, or binding
validation adds value.

## Reconciliation Findings

### Environmental quantity namespace

The current INA219/AHT20 bridge uses `quantity.dc.temperature` and
`quantity.dc.relative_humidity`. Those identifiers are internally consistent
across the retained graph and execution binding, but the `dc` namespace is not
the correct long-term physical category for environmental quantities.

Pass 1 preserves these exact retained identifiers. Pass 2 must author replacement
canonical identities, define an explicit old-to-new migration decision, and
regenerate from the earliest authored manifest. It must not search-and-replace
inside generated or retained JSON.

### C and C++ provenance

The direct E016 carrier proves shared ordinary-C adoption. The mixed INA219/AHT20
lane proves external-manifest and generated-C graph construction. Neither proves
automatic metadata extraction from production Arduino C++. Pass 2 should retain
this boundary and cross-reference firmware configuration through explicit
receipts or generated carriers.

### Declared, executed, and physical state

The retained hardware graph leaves presence, wiring, driver execution,
calibration, runtime observation, simulation, deployment, and physical
acceptance unassessed. The execution binding and simulator receipts can establish
a separate simulated claim. They cannot establish the current CAN wiring,
termination resistance, resistor removal, transceiver voltage behavior, or a
fresh ESP exchange.

## Validation Performed During This Checkpoint

The compiler-owned realization gate was invoked against the exact current ESP
root:

```bash
make integration-esp-realization-evidence \
  ESP_WORK_ROOT=/Users/calebsv/Desktop/CodeWork/ESP_Work
```

Its five contract suites completed successfully:

- 18 portable-C intake contract tests;
- 14 semantic/physical contract tests;
- 16 hardware-compatibility contract tests;
- 8 deterministic-proof contract tests; and
- 8 realization-evidence contract tests.

The terminal runner was then interrupted while computing its initial recursive
content fingerprint over the large retained ESP worktree. It had not reached
graph or receipt regeneration. Therefore this checkpoint establishes 64 passing
contract tests, but it does **not** claim a newly completed `0.7.0` E016 receipt
or a fresh exact full-tree fingerprint. The interruption did not change the ESP
Git status.

This scaling issue belongs to future compiler test-harness refinement: a bounded,
contract-owned intake fingerprint should avoid recursively hashing unrelated
retained build and evidence trees while still proving that selected external
inputs remain unchanged.

## Pass 2 Entry Contract

Completion update: the entry contract below was implemented as
`esp.can_pass2.sensor_backbone`. It selects ultrasonic distance on one endpoint,
AHT20 environment sensing on the middle node, and a supervisor at the other
endpoint. The deterministic fisiCs 0.7.0 gate and exact results are recorded in
`docs/esp_can_topology_pass2.md`.

Pass 2 may begin when its bounded representative topology is selected. It should
start from authored inputs and create a new projection rather than mutate the
retained mixed bundle.

The recommended first topology is one small multi-node CAN composition with:

1. stable node and device identities;
2. one selected subset of already-admitted sensor models;
3. CAN bus identity, bitrate, and transceiver revisions;
4. endpoint versus middle-node roles and authored termination intent;
5. local I2C bus/address and GPIO allocations;
6. semantic message producer and consumer relationships;
7. physical quantity and concrete execution-conversion contracts;
8. exact simulator model/version bindings; and
9. all deployment, observation, and physical fields left `not_assessed`.

Before graph generation, Pass 2 must decide:

- the canonical replacements for the two `quantity.dc.*` environmental IDs;
- which one representative sensor/CAN topology is in scope;
- whether termination intent fits hardware graph v1 or needs an additive schema
  extension; and
- how the authored topology references current ESP protocol and firmware
  configuration without claiming direct C++ compiler provenance.

No bench run is a prerequisite for authoring or statically validating that
topology. Physical work becomes justified only for a specifically identified
fact that static metadata and simulation cannot establish.

## Claim Boundary

Established by this checkpoint:

- the current repository and retained-artifact identities were inventoried;
- the protected Main Edit is cleanly checkpointed;
- the active ESP tree was left byte-owned by its existing lanes;
- the current 16-profile adoption surface is recorded;
- the retained mixed graph and execution-binding hashes are preserved;
- the environmental namespace drift and C/C++ provenance boundary are recorded;
- 64 focused contract tests passed; and
- Pass 2 has an explicit software-only entry contract.

Not established:

- a blanket preservation commit for the active ESP tree;
- a freshly generated `fisiCs 0.7.0` mixed or E016 graph receipt;
- runtime CAN observation;
- current firmware deployment;
- present wiring or termination state;
- simulation-to-physical accuracy; or
- physical acceptance of any sensor, ESP, transceiver, or CAN line.
