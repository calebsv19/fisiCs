# E016 Sensor Metadata Bridge V1 Plan

Status: compiler-owned bridge complete through deterministic proof,
retained-evidence comparison, and public case study; one historical deployment
conflict and the absence of a retained raw capture remain explicitly reported

## Purpose

Make the current fisiCs metadata and static physical-system graph layers usable
against one production-connected sensor without expanding the compiler into an
ESP toolchain, firmware generator, deployment authority, or physical-evidence
owner.

The selected carrier is the E016 Bench B potentiometer path. Its two pure C99
operations are compiled into current ESP firmware, annotated with portable
fisiCs metadata, validated under strict Clang and fisiCs, built by the ESP
vendor toolchain, flashed to identity-matched boards, and accepted in a bounded
recovered CAN and physical-response regression.

This milestone begins with a read-only audit. It must not start implementation
until the audit reports current repository state, authoritative inputs,
provenance, schema fit, unresolved decisions, and the smallest safe first
slice.

## Current Proven Layers

### Compiler-owned

- portable `FISICS_*` declaration macros;
- canonical declaration identities and repeatable relationships;
- deterministic translation-unit metadata export;
- semantic project graph resolution;
- physical composition graph validation;
- hardware catalog/topology compatibility validation; and
- strict Clang/fisiCs public portability and static hardware examples.

### ESP-owned

- production C99 normalization and publication-decision operations;
- frozen six-case normalization and twelve-case publication vector corpus;
- authored protocol-to-source semantic identity mapping;
- byte-identical vendored portability header with an exact parity gate;
- Arduino C++ adoption without moving ADC, filtering, protocol, CAN, commit,
  LED, or node policy into the C slice;
- manifest-v2 source receipts and identity-bound firmware candidates;
- separately authorized uploads to Bench A and Bench B;
- recovered bidirectional CAN and fresh semantic signal observation; and
- bounded operator-confirmed potentiometer response.

## Retained Evidence Limit

The compiler lane now composes the translation-unit artifact's two
source-operation records and external relationships to
`signal.bench_potentiometer.test_level` through a semantic project graph and
an honest dimensionless physical contract, followed by a bounded static
hardware compatibility graph. The realization receipt now compares that
evidence with:

- retained firmware candidates and USB identities; and
- the documentary E016 runtime and operator evidence.

The historical sources disagree about the selected Bench B deployment and do
not retain the selected raw serial capture. The bridge reports those gaps
rather than upgrading them. Static graph results and live evidence remain
separate even when one report links them. New exact deployment and observation
chains use [the external receipt contract](external_evidence_receipts.md).

## Authority And Ownership

| Concern | Owner |
| --- | --- |
| Compiler syntax, validation, metadata export, and static graph readers | `fisiCs` |
| Production C source, behavior vectors, and Arduino adoption | `ESP_Work` / `CodeWorkEspCore` |
| Semantic CAN object and wire identity | `ESP_Work/protocol/schema/` |
| Node placement, pins, devices, buses, and experiment wiring | `ESP_Work` |
| Firmware candidates and upload receipts | `ESP_Work` |
| Serial/CAN traces and physical acceptance | numbered ESP experiments |
| Simulation models and predicted state | ESP simulation system |

The compiler must not allocate CAN IDs, select pins, generate drivers or
firmware, contact hardware, or promote static compatibility into runtime or
physical truth.

## Frozen Initial Semantic Boundary

The first source operations are:

```text
operation.esp.normalized_signal.adc_normalize
operation.esp.normalized_signal.publication_decide
```

Their protocol-owned relationship target is:

```text
signal.bench_potentiometer.test_level
```

The first audit must decide whether the project graph should retain this as an
explicitly allowed protocol-owned external identity or consume a dedicated
protocol carrier artifact. It must not add a dummy C declaration merely to make
resolution appear complete.

## Physical Meaning Boundary

The current value is an integer representation from 0 through 1000 normalized
against the filtered ADC domain. It is dimensionless and uncalibrated. V1 must
not relabel it as voltage, shaft angle, travel distance, lux, or calibrated
percentage.

Contract refinement must select explicit quantity, value-form, unit-policy,
and reference-semantics fields supported by the existing physical graph. If
the current schema cannot express a normalized representation honestly, the
audit must report that gap rather than invent a physical unit.

## Declared Hardware Scope

The initial graph should contain only the hardware needed to explain the
accepted E016 sensor path:

- Bench A and Bench B ESP32-C6 boards;
- the Bench B potentiometer input on GPIO1;
- the two local TJA1050 CAN transceivers;
- GPIO10 TX and the protected GPIO11 RX junction on each node;
- Classical CAN at 500 kbit/s;
- CANH, CANL, and shared CAN ground;
- separate local 5 V domains; and
- Bench B producer and Bench A consumer bindings for the canonical signal.

Catalog and topology entries are authored declarations. Passing the hardware
graph must still leave presence, wiring, driver execution, calibration,
deployment, runtime observation, and physical acceptance `not_assessed`.

## Planned Work Slices

### A. Read-only audit and contract refinement

Status: complete. The audit selected an explicitly allowed protocol-owned
external identity, a dimensionless and uncalibrated physical contract, and a
bounded hardware representation that does not promote electrical or live
claims.

1. Inventory both dirty worktrees without resetting, cleaning, staging, or
   rewriting unrelated files.
2. Re-read the compiler contracts, ESP portable-core contract and evidence,
   protocol mapping, production C source, E016 node profiles, candidates, and
   experiment record.
3. Confirm every authoritative path, schema version, source digest, toolchain
   identity, and current validation command.
4. Compare the E016 requirements with the existing semantic, physical, and
   hardware schemas; identify exact reuse and any schema gap.
5. Resolve the external-target and dimensionless-representation decisions.
6. Return a proposed file map, command map, negative-test matrix, and smallest
   implementation slice. Stop without editing source, manifests, tests, or
   generated artifacts.

### B. Compiler-owned external intake

Status: complete. The fixed digest-bound lane is implemented by
`tests/integration/run_esp_portable_c.py`, its hostile contract tests, and the
`integration-esp-portable-c` make target. Generated results remain under
`build/integration-esp-portable-c/`; no ESP-owned file is modified.

The focused `integration-esp-portable-c` lane accepts an explicit ESP
repository root, verifies exact input digests, runs the frozen vectors under
strict Clang and fisiCs, emits fresh metadata under `build/`, validates the
expected identities and edges, fails closed on zero selection or drift, and
leaves external source untouched.

A later compiler-owned source snapshot may support standalone CI only if it is
labelled as a provenance-bound regression fixture and never becomes a second
production owner.

### C. E016 semantic and physical composition

Status: complete. The fixed `integration-esp-semantic-physical` lane
regenerates the external translation-unit metadata, explicitly allows only the
protocol-owned target, retains both relationships as external with null local
target indices, and composes one catalog-declared physical contract.

The authored representation contract freezes the integer range `0..1000`,
the filtered-ADC normalization basis, dimensionless scalar form, absence of a
concrete unit, linear reference semantics, and uncalibrated state. It rejects
voltage, shaft-angle, travel-distance, and calibrated-percentage
interpretations. Generated semantic, units, physical, and receipt artifacts
remain under `build/integration-esp-portable-c/`. Both graphs are emitted
twice and compared byte-for-byte; stale semantic and units provenance and an
unapproved external target are rejected. Hardware compatibility remains
`not_assessed`.

### D. E016 hardware declaration

Status: complete. The fixed `integration-esp-hardware-compatibility` lane
authors and validates one reusable component catalog and one bounded E016
topology. It contains the ESP32-C6-DevKitC-1/WROOM-1 board profile, Classical
CAN at 500 kbit/s, two declared TJA1050 transceivers, the Bench B
potentiometer GPIO1 attachment, protected GPIO11 receive and GPIO10 transmit
attachments on each node, explicit application driver contracts, and Bench B
producer/Bench A consumer bindings for the normalized signal.

The graph resolves two nodes, one shared bus, five devices, nine connections,
and two signal bindings with zero unbound required ports. Both bindings retain
`external_declared` physical evidence. The graph is emitted twice and compared
byte-for-byte, and focused negatives reject incorrect board, device, driver,
bus, endpoint, allocation, required-port, and physical-contract joins.

Hardware graph v1 cannot encode CANH/CANL continuity, common CAN ground,
separate local 5 V domains, the potentiometer's local 3.3 V supply,
termination resistance, or divider resistor values. The Slice D contract pins
those as declarations outside the graph schema rather than inventing fields or
claiming live wiring evidence. Hardware presence, wiring, driver execution,
calibration, runtime observation, deployment, and physical acceptance remain
`not_assessed`.

### E. Deterministic positive and negative proof

Status: complete. The fixed `integration-esp-deterministic-proof` lane rebuilds
the complete portable-C intake, semantic, physical, and hardware chain; pins
the exact terminal graph corpus and digests; and records byte-identical double
emission for every terminal graph. Its proof receipt covers all required
negative classes, including source/protocol digest staleness, conflicting
semantic identity, physical quantity/form/unit/reference incompatibility,
revision and driver joins, an explicit GPIO collision, missing allocation and
binding, and attempted promotion of live evidence. The selected ESP worktree
is fingerprinted before and after and remains read-only.

The lane establishes host C behavior parity, compiler metadata validation,
semantic and physical composition, and static hardware compatibility only.
Hardware presence, wiring, driver execution, calibration, runtime observation,
simulation, deployment, and physical acceptance remain `not_assessed`.

Emit each terminal graph twice and require byte identity. Positive assertions
must lock intended records, edges, components, bindings, and `not_assessed`
evidence fields. Negative fixtures must cover at least:

- stale source or protocol digest;
- unknown or disallowed relationship target;
- conflicting duplicate identity;
- incompatible physical quantity, value form, unit policy, or reference kind;
- wrong board, device, driver, bus, or endpoint revision;
- GPIO or other exclusive allocation collision;
- missing required physical-port binding; and
- any attempted promotion of deployment, runtime, calibration, or physical
  acceptance from static inputs.

### F. Separate realization and evidence receipt

Status: complete with reported retained-evidence mismatches. The fixed
`integration-esp-realization-evidence` lane validates the retained Slice E
receipt and exact static artifacts plus both retained candidate manifests, confirms that
both candidates contain the same portable C/header/protocol inputs, and emits
a deterministic companion receipt without contacting hardware or modifying
the ESP worktree.

The detailed portable-core evidence documents upload of the selected Bench B
0.2.0 candidate, while the physical-CAN milestone says that candidate remained
compile-only and Bench B 0.1.0 remained deployed. The receipt preserves this as
`conflict_unresolved`; it does not silently choose a source or upgrade the
selected pair to exact deployed/runtime/physical status. It also records that
the selected evidence contains narrative summaries rather than a retained raw
serial transcript. Historical pongs, semantic sequences, `quality=GOOD`, and
operator response remain documentary evidence; fresh runtime state,
calibration, and current hardware state remain `not_assessed`.

The current ESP protocol schema also differs from the digest retained by the
Slice E graph and both candidates. Slice F therefore validates the retained
Slice E receipt and immutable candidate contents without pretending they can
be regenerated from the moving current worktree. The current drift is recorded
as a second mismatch and no external bytes are rewritten.

Create a deterministic companion receipt that cross-references, without
absorbing authority from:

- translation-unit and terminal static graph digests;
- Bench A and Bench B candidate manifests and USB identities;
- the failed initial no-peer/bus-off observation;
- the recovered pong and `quality=GOOD` semantic-sequence window; and
- the operator-confirmed potentiometer response.

This receipt may compare declared, deployed, and observed projections. It must
report mismatches rather than silently rename, infer, or repair them.

### G. Public case study and agent path

Status: complete. The
[ESP portable C and static graph case study](esp_compiler_bridge_case_study.md)
documents one runnable path from annotated C to terminal static graphs and the
separately owned evidence comparison. It explains the shared C/C++ boundary,
portability macros, artifacts, regeneration commands, strongest claims, and
hardware non-claims.

## Completion Gates

The milestone is complete only when:

1. the compiler-owned external intake is digest-bound and repeatable;
2. strict Clang/fisiCs behavior and metadata expectations pass;
3. the complete E016 static graph chain is deterministic;
4. every required positive and negative test passes;
5. the static hardware assessment retains all live-evidence non-claims;
6. the realization receipt links exact source, candidate, and observation
   evidence without collapsing ownership;
7. a public agent/user case study reproduces the supported path; and
8. focused gates pass before any broader compiler regression selected by the
   confidence-tier guide.

## Explicit Non-goals

- new metadata attribute syntax;
- direct Arduino C++ parsing or metadata export;
- an ESP32-C6 fisiCs backend, linker, SDK, or runtime;
- compiler-generated CAN IDs, pins, drivers, firmware, or deployment;
- migration of all `CodeWorkEspCore` code to C;
- a universal component registry or dynamic driver loader;
- calibration of the potentiometer or ADC;
- replacing numbered hardware experiments with graph output; or
- changing firmware, reflashing boards, or contacting hardware during the
  compiler-owned bridge workflow.

## Continuation Contract

Future compiler refinement should use the project-neutral external receipt
validator and keep E016 as an immutable historical case study. ESP-owned upload
and capture tooling may adopt the receipt shapes in a separately scoped change.
No E016 bench replay is justified unless that owner explicitly chooses to close
the exact Bench B deployment or missing-raw-capture gap. Compiler changes must
not rewrite ESP evidence, contact hardware, flash devices, or collapse static,
deployment, runtime, and physical claims.
