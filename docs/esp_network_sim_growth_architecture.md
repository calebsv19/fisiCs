# ESP Network Sim Growth Architecture

Status: cross-system design handoff; proposed growth contract, not implemented
simulation behavior

Last reviewed: 2026-09-04

## Purpose

This document records how the ESP Network Sim should grow from its first
deterministic CAN replay workbench into a scalable network, control, and
physical-environment simulation system. It also defines the intended boundary
between the simulator, `ESP_Work`, and the fisiCs metadata and static-graph
pipeline.

The immediate audience is the agent or maintainer developing
`esp_network_sim`. Compiler work should use this document after the
application's runtime contracts stabilize. It is deliberately architecture
first: it does not add simulator code, compiler syntax, firmware generation,
or physical-hardware claims.

## Program Goal

The long-term program should let an operator author or load a versioned ESP/CAN
network preset, execute it under deterministic simulation time, inspect every
meaningful event, and compare predicted, simulated, recorded, and eventually
physical behavior without confusing those evidence layers.

A mature workbench should support:

- individual node clocks, task schedules, sensors, controllers, actuators, and
  fault states;
- deterministic CAN queues, arbitration, delivery, and controller state;
- pause, single-step, seek, replay, accelerated headless execution, and exact
  run reproduction;
- networks that grow from two nodes to tens of nodes without tying simulation
  throughput to the render frame rate;
- versioned adapters that connect node behavior to thermal, airflow, light,
  mechanical, electrical-load, or other physical models;
- stable compiler-derived identities, quantities, units, topology, and
  compatibility facts; and
- comparison of one run with retained software traces or separately collected
  HIL and physical evidence.

The program is not expected to infer a complete simulation from C source.
Source metadata can describe identity and contracts, but schedules, latencies,
model parameters, physical equations, calibration, and scenario stimuli must
remain explicit authored inputs.

## Current Working State

The initial `esp_network_sim` Main Edit workbench is a deterministic replay and
inspection system, not yet a generated network simulator. At the reviewed
snapshot it provides:

- a `core_sim`-driven 1 ms clock with pause, play, single-step, reset, and
  playback speed controls;
- a bounded saved-workbench document;
- component and connection topology;
- signal cards, checkpoint charts, raw and decoded frame inspection, and
  component-contract inspection;
- time, frame, and checkpoint seeking, including reconstruction after backward
  seeks;
- a C ABI bridge to the original `ESP_Work` generated protocol codec,
  `SignalStore`, and `PeerHealthStore` rather than copied wire policy;
- replay of one receiver's 45-frame observation stream across 18,001 ms;
- exact comparison against 19 retained checkpoints; and
- source/protocol provenance plus malformed-input rejection.

The authoritative application-side snapshot is maintained in:

- `esp_network_sim/docs/current_truth.md`;
- `esp_network_sim/docs/architecture.md`;
- `esp_network_sim/docs/workbench.md`;
- `esp_network_sim/docs/replay-verification.md`; and
- `esp_network_sim/docs/future_intent.md`.

That slice does **not** yet execute node schedules, generate sensor samples,
schedule transmissions, model CAN arbitration or bit timing, represent
multiple receiver viewpoints, execute actuators, or evolve a physical
environment. Its raw trace records receiver-visible frames, so an absent frame
does not distinguish an unsent message, queue rejection, arbitration delay,
transmission error, or receiver loss.

The replay work should be retained. It is the first permanent evidence reader
and a useful golden reference for later generated execution.

## Target Layering

The simulator should grow around the following one-way evidence and execution
flow:

```text
portable C and authored project inputs
    -> fisiCs translation-unit metadata and units evidence
    -> semantic project graph
    -> physical composition graph
    -> hardware compatibility graph
    -> authored simulation-binding manifest
    -> deterministic execution engine
    -> immutable event journal and state checkpoints
    -> UI projections, assertions, and comparison reports
```

Recorded software or hardware observations enter through a separate trace
intake and may be compared with a simulated run. They do not rewrite the
declared graph or automatically calibrate a model.

### Layer ownership

| Concern | Owner |
| --- | --- |
| Portable firmware or policy C/C++ | `ESP_Work` |
| Semantic protocol and wire identity | `ESP_Work/protocol` |
| C parsing, extension validation, and translation-unit artifacts | fisiCs |
| Cross-source semantic, physical, and static hardware graphs | fisiCs |
| Model selection, parameters, schedules, faults, and run configuration | `esp_network_sim` |
| Deterministic scheduling, CAN behavior, physics adapters, and projections | `esp_network_sim` |
| Firmware candidates, upload, telemetry, and physical experiments | `ESP_Work` evidence lanes |

This preserves the existing rule that the compiler owns static artifacts while
programs own runtime and simulation behavior.

## Permanent Dual-Engine Model

The UI should not be coupled directly to either recorded frames or the future
execution engine. Both paths should produce a common event and projection
contract:

```text
retained trace -----> trace intake -----+
                                        +-> event journal -> projections -> UI
authored preset ----> execution engine --+
```

This structure allows the same inspectors, charts, timeline, assertions, and
comparison tools to operate over:

- the current retained two-node replay;
- a generated software-only run;
- an imported firmware/SIL trace;
- a component-HIL trace; or
- a separately authorized physical capture.

Every run must retain its origin and evidence class. A visually identical
projection must not erase whether it came from replay, simulation, or hardware.

## Proposed Artifact Set

A complete saved simulation should be a versioned artifact set rather than one
unbounded document. The logical parts are:

### 1. Declared graph bundle

References exact semantic, physical, and hardware graph schema versions and
digests. The simulator must reject incompatible or stale graph inputs rather
than editing their hashes.

### 2. Simulation-binding manifest

Maps stable graph identities to executable model identities and versions. Each
binding should record:

- component or signal identity;
- realization class such as `scripted`, `behavioral`, `calibrated`, or
  `environmental`;
- model and adapter version;
- typed parameters and units;
- declared model inputs and outputs; and
- the graph contract each port satisfies.

The first binding validator can live in the application. fisiCs integration
should begin by consuming and checking existing graph artifacts, not by adding
new C attributes.

### 3. Preset and run configuration

Owns deterministic execution choices:

- initial state;
- global time resolution;
- node clock parameters;
- task and sample schedules;
- CAN-controller and queue configuration;
- scenario stimuli and fault injections;
- physics adapter configuration;
- deterministic seed; and
- stop time and capture/checkpoint policy.

### 4. Event journal

Records immutable domain events in deterministic order. Events should have a
stable event kind, global simulation timestamp, sequence/tie-break identity,
origin component, relevant semantic identities, and typed payload. Examples
include sample start/completion, task release, signal publication decision,
CAN enqueue, arbitration result, transmission interval, delivery, rejection,
freshness transition, actuator decision, physics-boundary update, and model
fault.

### 5. State checkpoints

Periodically snapshot complete deterministic engine state. Backward seeking
should restore the nearest earlier checkpoint and replay only the remaining
events. The current reconstruct-from-start behavior is acceptable for the
small retained trace but is not the scaling design.

### 6. Run receipt

Binds the exact graph bundle, binding manifest, preset, model versions, seed,
engine version, journal digest, and checkpoint policy. A receipt may compare a
run with external evidence, but it must not promote simulated evidence into a
runtime or physical claim.

The serialized representation may begin as strict bounded JSON. A binary pack
or indexed journal can be introduced only after event and checkpoint contracts
are stable and measured scale justifies it.

## Time And Causality Contract

Simulation time should use deterministic integer values. A 1 ms initial step
is useful but must not become an assumption embedded in every domain model.
The system must distinguish:

- global simulation time;
- node-local time, including offset, drift, and wrap behavior;
- sensor acquisition start and completion time;
- node task release and execution time;
- CAN enqueue time;
- arbitration and transmission start/end time;
- receiver delivery time; and
- UI wall-clock and render time.

The engine should freeze an explicit same-time pass order. A practical first
contract is:

1. apply scenario and external-environment inputs;
2. complete due sensor acquisitions;
3. release and execute due node tasks/controllers;
4. enqueue requested CAN transmissions;
5. arbitrate and advance CAN serialization;
6. deliver completed frames to each receiver;
7. update peer health, signal stores, freshness, and controller inputs;
8. resolve actuator-owner decisions;
9. advance physical adapters using the accepted actuator state; and
10. emit assertions, measurements, checkpoints, and UI projections.

If a future model needs sub-step integration, it must do so inside its adapter
or propose an explicit scheduler-contract revision. Hidden same-tick feedback
and container-iteration ordering must never decide simulation behavior.

## CAN Fidelity Staircase

CAN behavior should expand in separately testable levels.

### CAN-0: retained receiver replay

This is the current level. It reconstructs receiver stores from retained raw
frames and checkpoints. It makes no bus-timing or transmission claim.

### CAN-1: logical scheduled delivery

Execute editable node and sensor schedules, invoke production-compatible
encoding/decoding, and use deterministic logical delivery. The current
two-node trace remains a golden comparison preset.

### CAN-2: timed queues and arbitration

Add per-controller transmit queues, configured bitrate, frame duration, bus
occupancy, simultaneous-request arbitration, and identifier priority. Keep
semantic signal identity separate from 11-bit arbitration identity.

### CAN-3: controller and network faults

Model bounded capacity, acknowledgement availability, retries, error counters,
bus-off and recovery, disconnection, duplicated delivery, receiver-specific
loss, and explicit overflow outcomes where the selected model supports them.

### CAN-4: observed timing comparison

Import timestamped SIL, firmware, or HIL traces and compare event windows,
ordering, latency, and state projections. Differences are comparison results,
not automatic model calibration.

### CAN-5: optional electrical model

Analog signaling, transceiver voltage domains, termination, impedance,
propagation, noise, and wiring faults require a dedicated electrical adapter or
external solver. The ordinary CAN engine must not label simplified logical
behavior as electrical proof.

## Physics Adapter Contract

Physics belongs behind versioned adapters. The network engine should exchange
typed quantities with adapters rather than depend on a particular solver.

An adapter should declare:

- its identity and version;
- supported input and output physical contracts;
- parameter schema and units;
- integration step or event policy;
- deterministic-state serialization requirements;
- seed/noise behavior;
- failure and out-of-domain behavior; and
- fidelity and calibration class.

Sensors sample adapter-owned world state at explicit acquisition times.
Actuator owners write accepted boundary conditions or intents to the adapter.
The adapter then advances world state under the documented pass order.

The first useful physical preset should remain small: one or two lumped
thermal zones, an ambient boundary, one heat source, a fan with an authored
response curve, and an AHT20-like sensor with response lag. This can establish
the sensor-to-CAN-to-controller-to-actuator-to-environment loop before a
spatial airflow or fluid solver is introduced.

The UI must distinguish:

- predicted physical field or model state;
- simulated sensor output;
- recorded or live telemetry; and
- calibration residuals or comparison error.

A smooth heat map is a model projection, not a measurement.

## Scaling And Runtime Structure

The control surface may expose a 1,000-tick-per-second clock without scanning
every node on every millisecond. The execution engine should eventually use a
deterministic due-event queue or equivalent ordered scheduler.

Required scaling properties are:

- simulation update rate independent from render rate;
- headless faster-than-real-time execution;
- deterministic ordering for equal timestamps;
- bounded queues and explicit overflow outcomes;
- periodic state checkpoints;
- indexed event access for timeline and inspector views;
- stable results across play, single-step, batch, and seek workflows; and
- measurement of event count, memory, and execution time for 2-, 10-, and
  50-node reference presets.

`core_sim` remains suitable for pause/play/fixed-step control. The application
owns the domain scheduler. Shared queue, trace, scene, space, or worker modules
should be adopted only when the concrete event and scaling contract proves
their fit.

## Visual Workbench Direction

The existing network, signal, frame, timeline, and inspector surfaces should
remain projections over engine state. Rendering and input code must not own
simulation truth.

Expected growth includes:

- per-node task and local-clock inspection;
- transmit/receive queues and bus-occupancy timeline;
- arbitration contenders and winner reasoning;
- signal provenance from semantic producer through receiver store;
- selectable receiver viewpoints;
- freshness, fault, reboot, and recovery history;
- actuator-owner request/result/state inspection;
- physical-field and sensor overlays;
- comparison between simulated and observed traces; and
- run-receipt and graph-provenance inspection.

Interactive preset editing should mutate an authored working copy and produce
a new run identity. It should not silently modify retained evidence or a prior
run receipt.

## fisiCs Integration Boundary

fisiCs currently supplies the correct static layers:

- semantic declaration metadata and translation-unit export;
- cross-translation-unit semantic project graphs;
- physical composition graphs for quantity, dimension, unit policy, value
  form, reference semantics, and frame; and
- hardware compatibility graphs for supplied component catalogs, topology,
  profiles, buses, devices, drivers, allocations, and signal bindings.

Portable firmware source can use `<fisics/extensions.h>` and `FISICS_*`
macros. They retain metadata under fisiCs and erase under ordinary compilers.
They neither enable an extension automatically nor add a firmware runtime.

The first compiler-facing simulator slice should therefore:

1. define a versioned graph-intake and simulation-binding contract;
2. load exact existing graph artifacts without changing them;
3. validate schema versions, digests, stable identities, and model-port joins;
4. reject quantity, dimension, unit, value-form, frame, topology, or model
   incompatibility;
5. preserve all simulation, deployment, runtime, calibration, and physical
   assessment states separately; and
6. emit a deterministic intake or run receipt.

No new metadata syntax is justified until a stable application binding exposes
a specific fact that existing semantic, physical, and hardware layers cannot
represent. The compiler must not infer task schedules, physics equations,
noise distributions, CAN IDs, pins, drivers, or calibration from declaration
names.

## Software Validation Matrix

The implementation program should grow through small, headless, deterministic
gates before visual inspection.

| Area | Required proof |
| --- | --- |
| Replay preservation | Current retained checkpoints and frame projections remain exact |
| Determinism | Identical inputs emit byte-identical journal and receipt artifacts |
| Control | Pause mutates no simulation state; single-step and batch execution agree |
| Causality | Same-time pass order and tie breaking are fixture-tested |
| Seeking | Direct execution and checkpoint restore/replay reach identical state |
| Scheduling | Exact rate boundaries, missed deadlines, drift, and wrap fixtures |
| CAN | Arbitration, queue capacity, latency, loss, retry, and recovery micro-scenarios |
| Physics | Adapter conservation/invariant tests plus fixed golden scenarios |
| Graph intake | Stale hash, unknown identity, incompatible unit/frame/model ports fail closed |
| Projection | Rendering and selection do not mutate engine truth |
| Scaling | Bounded 2-, 10-, and 50-node time/memory/event-count benchmarks |

The retained two-node temperature/humidity scenario should remain the first
cross-layer oracle. Generated logical execution need not reproduce every
historical fault through identical internal causes, but its receiver
projection must match wherever the preset explicitly claims equivalence.

## Bench And Physical-Test Boundary

No bench execution is needed to design or validate:

- artifact schemas and graph intake;
- deterministic clocks, scheduling, event ordering, and seeking;
- logical or timed CAN queues and arbitration;
- the replay and projection system;
- headless scaling;
- physics-adapter APIs; or
- uncalibrated deterministic physical-model behavior.

New physical work is justified only for a named stronger claim, such as:

- firmware execution on a selected target;
- observed CAN timing or error behavior;
- electrical-bus behavior;
- sensor or actuator calibration;
- validation of a physics model against measurements; or
- explicit physical acceptance of an assembly.

Retained exact-version evidence should be attempted first. Any new bench run
should name the precise evidence gap it closes and should remain in an
ESP-owned experiment or HIL lane.

## Recommended Growth Slices

### S0. Freeze the replay foundation

Complete the active replay workbench, retain its exact golden artifacts, and
freeze the UI and headless invariants that subsequent slices must preserve.

### S1. Establish the event contract

Define event kinds, time domains, tie-breaking, run identity, immutable journal
rules, and periodic deterministic state checkpoints. Adapt retained replay to
the common event/projection boundary.

### S2. Execute logical presets

Add editable node/sample schedules and deterministic logical CAN delivery.
Use the existing protocol codec and stores. Preserve the retained trace as a
reference rather than converting it into generated evidence.

### S3. Add CAN scheduling fidelity

Introduce queues, bitrate, frame duration, arbitration, receiver viewpoints,
and bounded network/controller faults through focused micro-scenarios.

### S4. Add compiler graph intake

Define and validate the simulation-binding manifest against exact fisiCs
semantic, physical, and hardware graph artifacts. Begin with existing
metadata; add no speculative syntax.

### S5. Close one physical control loop

Implement one deterministic lumped thermal adapter and connect sensor
acquisition, CAN delivery, controller decisions, actuator ownership, and world
state. Keep simulated and measured series visibly distinct.

### S6. Prove scale and richer spatial adapters

Add indexed checkpointing, 10- and 50-node benchmarks, and only then bind more
expensive spatial, airflow, collision, or fluid solvers through the established
adapter contract.

### S7. Compare external evidence

Import exact-version SIL/HIL/physical traces under separate receipts and report
differences without rewriting the declared graph or promoting acceptance.

## Handoff Rules For The Application Task

The `esp_network_sim` implementation task may use this document as design
input, but should continue to treat its own repository docs and current source
as application truth. Before implementing a slice it should:

1. read the current application worktree and preserve concurrent changes;
2. select only one bounded growth slice;
3. define its authored inputs, generated outputs, and negative cases;
4. keep replay, generated simulation, observation, and physical evidence
   labeled separately;
5. run the narrowest headless proof before visual or package checks; and
6. update application-owned current-truth and architecture docs after the
   behavior lands.

If application experience contradicts this proposal, the application task
should record the concrete conflict and revise the cross-system contract
rather than silently diverging.

## Explicit Non-Goals

This architecture does not authorize or imply:

- compiler-generated firmware, drivers, schedules, CAN identifiers, or pins;
- a fisiCs ESP backend, linker, SDK, or embedded metadata runtime;
- inference of physics equations or calibration from C metadata;
- treating a static compatibility graph as an executed simulation;
- treating simulation as runtime observation or physical acceptance;
- replacing retained ESP experiment evidence with generated traces;
- analog CAN/electrical claims from a logical bus model;
- immediate adoption of a universal registry or dynamic plugin system; or
- hardware contact, upload, deployment, publication, or release.

## Completion Definition

The architecture is realized only when a versioned preset can bind exact
compiler graphs to explicit simulation models, execute deterministically,
produce a reproducible journal and receipt, restore identical state through
checkpoints, drive the existing workbench projections, and retain honest
evidence labels. Physical accuracy remains a separate calibration and
acceptance question even after that software system is complete.
