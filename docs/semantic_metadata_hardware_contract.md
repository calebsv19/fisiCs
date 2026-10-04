# Hardware Compatibility Graph Contract

Status: implemented and validated

Optional detached operation: [Frozen Offline Graph Inputs](frozen_graph_inputs.md)
captures the complete offline data inputs and rebuilds this graph after its
semantic and physical predecessors. Static assessment remains separate from
runtime or physical acceptance.

Hardware manifest contract ID: `fisiCs.hardware_project_manifest`

Hardware catalog contract ID: `fisiCs.hardware_component_catalog`

Hardware topology contract ID: `fisiCs.hardware_topology`

Hardware graph contract ID: `fisiCs.hardware_project_graph`

Schema version: `1`

Primary input: complete `fisiCs.physical_project_graph` schema `1`

Last audited against source: 2026-09-03

This document defines the implemented, reusable static hardware-compatibility
consumer of the physical composition graph. Its permanent proof fixture is
one node and board profile, one bus, one sensor, one actuator, and one small
external-declared component catalog. Those fixture counts bound the proof;
they do not hard-code a van, ESP board, CAN network, sensor family, actuator
family, or simulation engine into the schema.

The hardware compatibility graph answers whether an authored hardware configuration is internally
coherent. It does not prove that any device is purchased, present, genuine,
wired, powered, flashed, calibrated, reachable, sampled, safe, or functioning.

## 1. Completion Question

One deterministic hardware compatibility graph artifact must be able to answer:

```text
Which reusable board, bus, device, and driver contracts were declared?
Which concrete node, bus, and device instances were configured?
Do all profile, ownership, attachment, driver/model, and allocation references
resolve without contradiction?
Which device physical ports bind to which physical contracts, and are
their declared physical requirements compatible?
Which claims remain declarations rather than observations?
```

The words `exists`, `resolved`, and `compatible` are scoped to supplied static
artifacts. They never mean physical presence or successful runtime operation.

## 2. Layer Ownership

```text
physical composition graph
    owns semantic subjects, quantity kinds, dimensions, unit compatibility,
    value form, reference semantics, frames, and source-evidence state

hardware component catalogs
    own reusable board, bus, device-model, driver-contract, endpoint,
    attachment-port, and physical-port declarations

hardware topology
    owns configured node, bus, device, connection, resource-claim, and
    physical-port-binding instances

hardware compatibility graph
    owns deterministic joins, scoped compatibility, provenance, normalized
    allocation keys, coverage, and explicit not-assessed states

future runtime and physical-evidence artifacts
    own discovery, identity attestation, transport sessions, observations,
    timestamps, uncertainty, calibration evidence, health, and acceptance

simulation systems
    own geometry, materials, transfer functions, dynamics, solvers, state
    evolution, model calibration, prediction, and measured-model comparison
```

The hardware compatibility graph must consume physical composition facts. It must not copy the units registry,
reparse dimension expressions, infer quantity meaning, or recalculate physical composition graph
conversion recipes.

## 3. Truth Vocabulary

The hardware compatibility graph uses deliberately scoped terms:

| Term | Exact meaning |
| --- | --- |
| `catalog_resolved` | an exact profile/model/revision key exists in the supplied catalogs |
| `declared_instance_resolved` | an authored topology instance resolves to valid catalog declarations |
| `declared_topology_compatible` | every required static join and conflict check passed |
| `source_compatible` | copied from the referenced physical contract; not upgraded by the hardware graph |
| `source_unverified` | copied from physical composition graph and permitted only by manifest policy |
| `external_declared` | copied from physical composition graph or declared by an external catalog; not externally verified |
| `not_assessed` | the artifact has no authority to make this claim |

The hardware graph assessment always contains:

```json
{
  "catalog_integrity": "validated",
  "declared_topology": "compatible",
  "hardware_presence": "not_assessed",
  "wiring": "not_assessed",
  "driver_implementation": "not_assessed",
  "calibration": "not_assessed",
  "runtime_observation": "not_assessed",
  "simulation": "not_assessed",
  "deployment": "not_assessed",
  "physical_acceptance": "not_assessed"
}
```

No hardware compatibility graph input may replace a `not_assessed` value.

## 4. Hard Non-Goals

The hardware compatibility graph v1 must not:

- add C syntax, source attributes, pragmas, expression metadata, or callable
  port inference;
- change translation-unit metadata export, semantic project graph, physical composition graph, build-manifest v0, or frontend ABI schemas;
- generate, select, compile, install, or execute a driver;
- fetch catalogs, datasheets, packages, firmware, or network resources;
- probe a host, serial port, GPIO controller, CAN interface, or device;
- perform CAN, I2C, SPI, UART, GPIO, USB, network, or other transport I/O;
- encode transport-specific packet, register, arbitration, or timing rules;
- validate voltage/current limits, pull-ups, termination, grounding, power
  budgets, connector orientation, or physical placement;
- define calibration curves, calibration observations, telemetry, timestamps,
  uncertainty, health, freshness, or fault handling;
- generate firmware, deployment configuration, wiring diagrams, or simulation
  models;
- claim that an external catalog is trusted, current, vendor-authenticated, or
  complete; or
- claim physical presence, wiring correctness, deployment, safety, operation,
  or acceptance.

Electrical constraints, placement, datasheet provenance, calibration, and
simulation-model identity remain useful future catalog extensions, but they
are not placeholder fields in schema v1. Adding them requires a new contract
version or separately versioned evidence artifact.

## 5. Identity And Revision Model

The hardware compatibility graph keeps these identities separate:

| Identity | Example | Owner |
| --- | --- | --- |
| catalog identity | `catalog.vendor.example` | catalog |
| reusable profile/model identity | `board.esp32.generic` | catalog |
| opaque profile revision | `rev-a` | catalog |
| configured instance identity | `node.van.front` | topology |
| local port/endpoint identity | `port.measurement` | enclosing model/profile |
| physical contract identity | `contract.cabin.temperature` | physical graph |
| future runtime observation identity | not present in hardware compatibility graph | runtime evidence owner |

Canonical identities use the existing lowercase dotted identity grammar and
are at most 127 bytes. Revisions are nonempty opaque printable ASCII strings
of at most 64 bytes. A reusable definition key is the exact pair `(id,
revision)`; revisions are never numerically ordered or guessed.

Configured identities are project-global. Local endpoint and port identities
are unique only within their owning profile/model. No category, ownership,
transport, or location may be inferred from spelling.

## 6. Provenance And Catalog Authority

Every consumed file receives a receipt containing its canonical path, raw byte
length, and FNV-1a-64 hash. The physical graph receipt also records its project
ID, schema, completion flag, unit-registry fingerprint, and assessment boundary.
The hardware compatibility graph rehashes every current source named by the physical composition graph source receipts and rejects
changed length or bytes. It does not require the physical composition graph producer version or unit
registry fingerprint to equal the current executable; both are retained as
provenance because hardware compatibility graph performs no unit calculation.

A catalog authority is exactly one of:

```json
{"kind":"project","identity":"authority.van.project"}
```

or:

```json
{"kind":"external_declared","identity":"authority.vendor.example"}
```

`external_declared` records authorship intent only. Local possession, a hash,
or successful parsing does not authenticate the purported vendor or source.
Signatures, trust stores, licenses, download locations, and update policy are
outside hardware compatibility graph v1.

## 7. Hardware Project Manifest

The manifest has exactly these eight fields:

```json
{
  "schema": "fisiCs.hardware_project_manifest",
  "schema_version": 1,
  "project_id": "van.control",
  "physical_project_graph_path": "physical.graph.json",
  "hardware_catalog_paths": ["project.hardware.json", "vendor.hardware.json"],
  "hardware_topology_path": "hardware.topology.json",
  "physical_evidence_policy": "require_source_compatible",
  "required_port_policy": "require_all"
}
```

Paths are resolved relative to the canonical manifest directory and then
canonicalized. Catalog order is semantically irrelevant. Empty paths,
duplicate canonical paths, aliases to another input, unknown fields, wrong
types, missing fields, noncanonical IDs, and trailing JSON are rejected.

`physical_evidence_policy` is one of:

- `require_source_compatible`: every signal binding must reference a physical composition graph
  contract in `source_compatible` state;
- `allow_declared`: `source_compatible`, `source_unverified`, and
  `external_declared` may be retained without promotion.

`required_port_policy` is one of:

- `require_all`: every required attachment and physical port is bound exactly
  once;
- `report_unbound`: unbound required ports are retained in coverage and make
  the graph complete but not `declared_topology_compatible`.

The initial canary and completion gate use `require_source_compatible` and
`require_all`.

## 8. Component Catalog

The root has exactly these fields:

```json
{
  "schema": "fisiCs.hardware_component_catalog",
  "schema_version": 1,
  "catalog_id": "catalog.vendor.example",
  "authority": {
    "kind": "external_declared",
    "identity": "authority.vendor.example"
  },
  "bus_profiles": [],
  "board_profiles": [],
  "device_models": [],
  "driver_contracts": []
}
```

The schema uses general capabilities and allocation spaces. Transport names
such as CAN or I2C are catalog values, not compiler enums.
Every catalog object has exactly the fields shown below. Unknown, missing, or
conditionally inconsistent fields reject the catalog.

### 8.1 Bus profile

```json
{
  "profile_id": "bus.i2c.controller",
  "revision": "v1",
  "transport_kind": "transport.i2c",
  "controller_capability": "endpoint.i2c.controller",
  "participant_capability": "endpoint.i2c.participant",
  "allocation_spaces": [
    {
      "space_id": "address",
      "value_kind": "uint64_decimal",
      "minimum_uint64": "0",
      "maximum_uint64": "127",
      "claim_required": true,
      "collision_policy": "exclusive"
    }
  ]
}
```

All six fields are required. Decimal bounds are canonical unsigned 64-bit
strings without signs or leading zeroes except `0`. A `canonical_id`
allocation space uses `null` for both bounds. `collision_policy` is
`exclusive` or `shared`. `claim_required` determines whether every participant
connection must claim exactly one value in that space. hardware compatibility graph checks only
identity, presence, range, and collision; it does not know protocol semantics.

### 8.2 Board profile

```json
{
  "profile_id": "board.esp32.generic",
  "revision": "rev-a",
  "capability_ids": ["board.microcontroller"],
  "endpoints": [
    {
      "endpoint_id": "controller.i2c0",
      "capability_ids": ["endpoint.i2c.controller"],
      "allocation_policy": "exclusive"
    }
  ]
}
```

The four fields are exact and required. Capability IDs are opaque canonical
identities. Endpoint allocation is `exclusive` or `shared`; sharing does not
waive bus-allocation collision rules.
An endpoint may represent one physical pin, a grouped peripheral controller,
or another board-local attachment resource; its capabilities carry the
meaning, and hardware compatibility graph does not infer that meaning from the endpoint ID.

### 8.3 Device model

```json
{
  "model_id": "device.environment.sensor",
  "revision": "rev-a",
  "capability_ids": ["device.sensor"],
  "driver_policy": "required",
  "attachment_ports": [
    {
      "port_id": "port.control",
      "connection_kind": "bus_participant",
      "required_capability": "endpoint.i2c.participant",
      "required_transport_kind": "transport.i2c",
      "required": true
    }
  ],
  "physical_ports": [
    {
      "port_id": "port.measurement",
      "direction": "produces",
      "required": true,
      "requirement": {
        "mode": "exact_quantity",
        "quantity_kind": "quantity.temperature.absolute",
        "dimension_system": null,
        "dimension_vector": [],
        "value_form": "scalar",
        "reference_kind": "absolute"
      }
    }
  ]
}
```

The six device-model fields are exact. `driver_policy` is `required`,
`optional`, or `none`; topology driver selection must follow it. This permits
passive and externally controlled physical devices without pretending that
every component has executable software. The frozen sensor and actuator both
use `required`.

`connection_kind` is `bus_participant` or `direct_endpoint`. A bus port
requires non-null capability and transport IDs. A direct endpoint port
requires a capability and has `null` transport. `direction` is hardware
centric: `produces`, `consumes`, or `bidirectional`. Direction is retained but
does not infer or validate semantic project graph semantic edges.

A physical requirement mode is:

- `exact_quantity`: `quantity_kind` is non-null and must exactly match the
  bound physical composition graph contract; dimension fields are null/empty;
- `dimension_only`: `quantity_kind` is null and the exact eight-element Dim8
  vector is compared with the quantity definition resolved by the bound physical composition graph
  contract.

`value_form` and `reference_kind` are nullable additional exact constraints.
The hardware compatibility graph compares already-resolved physical composition graph values and never parses dimension text or
performs conversions. Frames remain owned by physical composition graph; adding a hardware-frame
transform requires a later geometry contract.

### 8.4 Driver contract

```json
{
  "driver_id": "driver.environment.sensor",
  "revision": "v1",
  "supported_models": [
    {"model_id":"device.environment.sensor","revision":"rev-a"}
  ],
  "required_transport_kinds": ["transport.i2c"]
}
```

A driver contract is compatibility metadata, not executable code. Successful
pairing proves only that the selected driver declaration lists the exact model
key and required transport. hardware compatibility graph does not locate a function, library, package,
binary, or installed driver.

## 9. Hardware Topology

The topology root has exactly these fields:

```json
{
  "schema": "fisiCs.hardware_topology",
  "schema_version": 1,
  "project_id": "van.control",
  "nodes": [],
  "buses": [],
  "devices": [],
  "connections": [],
  "signal_bindings": []
}
```

Every topology object has exactly the fields shown in this section. Array
order is non-normative. The topology project ID must exactly equal both the
manifest project ID and the physical composition graph project ID.

### 9.1 Instances

A node selects one exact board profile:

```json
{
  "node_id": "node.van.front",
  "board_profile": {"profile_id":"board.esp32.generic","revision":"rev-a"}
}
```

A bus is owned by one node and allocates one controller endpoint:

```json
{
  "bus_id": "bus.van.front.i2c0",
  "owner_node_id": "node.van.front",
  "scope": "node_local",
  "bus_profile": {"profile_id":"bus.i2c.controller","revision":"v1"},
  "controller_endpoint_id": "controller.i2c0"
}
```

`scope` is `node_local` or `project_shared`. A device connected to a local bus
must have the same owner node. A project-shared bus may connect devices owned
by any declared node; this is still authored topology, not proof of wiring.

A device selects one model and one driver contract:

```json
{
  "device_id": "device.van.temperature.front",
  "owner_node_id": "node.van.front",
  "model": {"model_id":"device.environment.sensor","revision":"rev-a"},
  "driver": {"driver_id":"driver.environment.sensor","revision":"v1"}
}
```

`driver` is null only when permitted by the model's driver policy. This
establishes declared node ownership only. It does not imply memory
ownership, scheduling, process placement, flashing, or runtime supervision.

### 9.2 Connections and allocation claims

```json
{
  "connection_id": "connection.temperature.control",
  "device_id": "device.van.temperature.front",
  "attachment_port_id": "port.control",
  "target": {
    "kind": "bus",
    "bus_id": "bus.van.front.i2c0",
    "board_endpoint_id": null
  },
  "allocation_claims": [
    {"space_id":"address","value_uint64":"68","value_id":null}
  ]
}
```

For `target.kind == bus`, `bus_id` is non-null and `board_endpoint_id` is null.
For `direct_endpoint`, the reverse is required and the endpoint belongs to the
device owner node. Every claim must name an allocation space defined by the
resolved bus profile and use exactly the declared value kind. The normalized
collision key is:

```text
(bus_instance_id, allocation_space_id, canonical_value)
```

Two claims with the same exclusive key are rejected with both connection
origins. Shared claims are allowed only when the profile explicitly declares
that allocation space shared. hardware compatibility graph does not infer protocol-specific aliases or
overlap between different spaces.

Controller endpoints and direct endpoints also have normalized keys:

```text
(node_instance_id, board_endpoint_id)
```

An exclusive endpoint may be owned by only one bus or direct connection.

For a bus connection, the model attachment port's required transport must equal
the resolved bus transport and its required capability must equal the bus
profile's participant capability. A bus controller endpoint must contain the
bus profile's controller capability. For a direct connection, the selected
board endpoint must contain the attachment port's required capability.

### 9.3 Signal bindings

```json
{
  "binding_id": "binding.temperature.measurement",
  "device_id": "device.van.temperature.front",
  "physical_port_id": "port.measurement",
  "physical_contract_id": "contract.temperature"
}
```

The physical contract ID must resolve exactly in the physical composition graph. The hardware
port requirement is compared to that contract's resolved quantity definition,
value form, and reference semantics. The physical composition graph evidence state is copied to the
hardware binding and checked against manifest policy; it is never promoted.

A `(device_id, physical_port_id)` has at most one binding. One physical
contract may intentionally bind to several hardware ports. No relation is
inferred from symbol names, semantic domains/roles, or `produces`/`consumes`
edges.

## 10. Duplicate And Conflict Policy

- Catalog definitions use `(definition kind, ID, revision)` keys. Structurally
  identical definitions from multiple catalogs deduplicate deterministically;
  differing definitions with the same key fail and report both catalog paths.
  Deduplicated definitions retain every sorted origin catalog and its authority;
  project authority never replaces or upgrades external-declared authority.
- Catalog IDs must be globally unique even if their contents are identical.
- Repeated values inside capability/model/transport sets are rejected; input
  order is otherwise non-normative.
- Topology instance IDs, connection IDs, and binding IDs are project-global
  and must be unique. Identical duplicates are rejected rather than deduped
  because each represents an authored configured instance or relationship.
- Duplicate node ownership, model selection, driver selection, attachment-port
  connection, physical-port binding, exclusive endpoint, and exclusive
  allocation claims fail closed.
- Multiple instances of the same model/profile, multiple connections to a
  shared bus, and multiple bindings to one physical contract are valid when
  the more specific uniqueness rules remain satisfied.

## 11. Validation Order

Validation is deterministic and fail-closed in this order:

1. Parse and canonicalize the manifest and every path.
2. Validate the complete physical composition graph identity, project ID, receipts, and fixed
   `not_assessed` physical-acceptance boundary.
3. Read and hash topology and catalog bytes under the frozen limits.
4. Parse exact schemas and canonical scalar forms.
5. Merge catalog definitions and reject identity conflicts.
6. Resolve node board profiles.
7. Resolve bus profiles, owners, and controller endpoint capabilities.
8. Resolve device models, owners, and driver/model/transport pairings.
9. Resolve attachment connections and normalized allocation claims.
10. Resolve signal bindings and compare physical requirements.
11. Enforce evidence and required-port coverage policy.
12. Sort owned state into normative output order and atomically publish.

The first diagnostic is deterministic for identical bytes regardless of input
catalog order.

An exact driver pairing requires the driver contract to list the selected
model/revision. Each driver-required transport must occur on at least one
resolved connection for that device. Every required allocation space must have
exactly one claim on each bus participant connection. Optional driver
selection, optional claims, and shared claims remain explicit rather than
inferred.

## 12. Hardware Project Graph

The output is `fisiCs.hardware_project_graph` schema `1`. Root keys are emitted
in this normative order:

```text
schema
schema_version
producer
project
complete
assessment
coverage
physical_graph_receipt
input_receipts
catalogs
bus_profiles
board_profiles
device_models
driver_contracts
nodes
buses
devices
connections
signal_bindings
unbound_required_ports
```

Definitions are sorted by `(ID, revision)`. Instances, connections, bindings,
capabilities, ports, supported-model references, transport requirements, and
allocation claims use their documented identity tuples. Output indices always
refer to these final sorted arrays.

Resolved output records retain authored IDs plus explicit indices to their
resolved definitions and owners. Signal bindings retain their referenced physical composition graph
contract index, copied physical composition graph evidence state, compatibility mode, and result.
Connections retain normalized allocation values and resolved target indices.

`complete: true` means the artifact was completely assembled under its stated
policy. It is not a hardware-completeness or acceptance claim. Under
`report_unbound`, `assessment.declared_topology` is `incomplete` rather than
`compatible`, and coverage identifies every missing required port.

## 13. CLI Contract

The hardware compatibility graph adds one paired terminal offline mode:

```bash
./fisics \
  --hardware-project-manifest hardware.manifest.json \
  --emit-hardware-project-graph-json hardware.graph.json
```

Both options are required exactly once. Their order is irrelevant. This mode
rejects source files, `-c`, `-o`, overlays, build manifests, translation-unit metadata export/semantic project graph/physical composition graph
emission or aggregation flags, dump modes, profiler modes, and every unrelated
argument before normal compiler initialization.

The output must not alias the manifest, physical graph, topology, any catalog,
or any current source path recorded by the physical composition graph.

## 14. Resource Limits

- one manifest and one topology file;
- 1 to 256 catalog files;
- 64 MiB per input file and 512 MiB total input bytes;
- 4,096 definitions per definition kind;
- 65,536 total configured nodes, buses, devices, connections, and bindings;
- 1,024 ports/endpoints per profile or model;
- 256 capabilities, supported models, transport kinds, allocation spaces, or
  claims per owning record;
- 4,096 bytes per ordinary string, with the tighter identity/revision limits
  taking precedence; and
- checked arithmetic for every count, byte total, allocation, and index.

These are admission limits, not recommendations for one embedded project.

## 15. Publication And Failure Contract

The graph is written to a temporary file in the destination directory,
flushed, synchronized, closed, and renamed only after complete success. Any
parse, provenance, resolution, conflict, limit, allocation, serialization,
flush, sync, close, or rename failure preserves an existing destination and
removes temporary files.

No partial graph is published. No input is modified. Failure-injection seams
must cover graph allocation and graph writing. Diagnostics identify the
failing identity and both origins for cross-file conflicts without embedding
pointer values, temporary path suffixes, or input-order-dependent indices.

## 16. Frozen Canary

The initial positive fixture contains exactly:

- one project catalog defining one board profile and one bus profile;
- one small `external_declared` catalog defining one sensor model, one actuator
  model, and their driver contracts;
- one node selecting the board profile;
- one node-local bus selecting the bus profile and controller endpoint;
- one sensor and one actuator owned by that node;
- one attachment connection per device with distinct exclusive allocations;
- one sensor-produced and one actuator-consumed physical port;
- two `source_compatible` physical composition graph contract bindings; and
- no unbound required ports.

The fixture may use ESP- and van-shaped names, but implementation logic and
goldens must remain vocabulary-neutral.

## 17. Acceptance Matrix

Focused proof must cover at least:

| Area | Required evidence |
| --- | --- |
| Positive canary | one board/node, bus, sensor, actuator, external catalog |
| Determinism | repeat run and permuted manifest catalog-path order produce identical bytes; semantically permuted catalog/topology bytes retain the same resolved body but distinct raw-byte receipts |
| Provenance | raw-byte receipts and project/physical-graph identity retained |
| Catalog authority | external declaration retained without trust promotion |
| Identity | exact ID/revision joins; missing and conflicting revisions reject |
| Ownership | node, bus, and device owners resolve; invalid local ownership rejects |
| Capability | controller/participant/direct endpoint requirements checked |
| Driver pairing | supported exact model succeeds; unsupported model/transport rejects |
| Allocation | distinct claims succeed; range, kind, and exclusive collisions reject |
| Physical signal | exact quantity and dimension-only positive cases |
| Physical negative | dimension, quantity, form, reference, missing contract reject |
| Evidence policy | unverified/external state rejected or retained exactly by policy |
| Coverage | required attachment/physical ports enforced or reported by policy |
| Duplicates | identical definitions dedupe; conflicting definitions fail with origins |
| CLI | exact paired mode; missing, repeated, mixed, and alias cases reject |
| Failure | allocation/write injection preserves destination and leaves no temp |
| Isolation | ordinary compilation and object bytes remain unchanged |
| Regression | translation-unit metadata export, semantic project graph, physical composition graph, build-manifest, units, and overlay gates stay green |
| Sanitizers | focused positive, negative, duplicate, and injected failures are clean |
| Broad confidence | repository tier justified by touched shared CLI/compiler code passes |

## 18. Implementation Record

The hardware compatibility graph was implemented in this order:

1. **[complete] Vocabulary and fixture lock.** Froze the positive canary,
   negative corpus, canonical IDs, normalized ordering, and output golden.
2. **[complete] Owned readers and models.** Implemented strict manifest, catalog,
   and topology parsing, limits, canonical paths, receipts, and destruction.
3. **[complete] Catalog merge and provenance.** Verifies the physical composition graph, merges
   exact profile/model revisions, retain authority, and reject conflicts.
4. **[complete] Instance and ownership resolution.** Resolves nodes, buses,
   devices, endpoints, attachment ports, scope, and required capabilities.
5. **[complete] Driver and allocation validation.** Proves exact driver/model/
   transport pairing and normalized endpoint/allocation conflict handling.
6. **[complete] Physical-signal bridge.** Resolves physical composition graph contract bindings, compares
   physical requirements, propagate evidence state, and enforce coverage.
7. **[complete] Graph serializer and terminal CLI.** Emits deterministic owned
   state atomically through only the exact paired hardware graph mode.
8. **[complete] Focused and broad proof.** Completes the acceptance matrix,
   sanitizer runs, inherited graph/overlay parity, and justified broad gate.
9. **[complete] Documentation closeout.** Marks hardware compatibility graph implemented from passing
   evidence and preserve runtime, calibration, simulation, deployment, and
   physical-acceptance boundaries.

Stop and return to contract review if implementation requires protocol packet
semantics, electrical calculations, source-level callable ports, driver code
lookup, catalog fetching, runtime discovery, observations, calibration,
simulation, deployment, or physical acceptance.

Implementation evidence on 2026-09-03:

- `make BUILD_PROFILE=sanitized -j2` and direct sanitized execution of
  `tests/integration/run_hardware_project_graph.sh ./fisics` passed;
- the focused static physical-system graph pipeline integration targets passed together;
- `make frontend-contract-test` and `make build-manifest-contract-test` passed;
- `make test` passed; and
- the normalized positive-canary digest is
  `26eda12875a1f1cb95f0902abe42c5b67e14a44fddb670c50ce7a24fee0c5b3a`.

## 19. Completion Boundary And Next Layers

The hardware compatibility graph is complete only when the frozen canary can produce one deterministic
graph proving that its supplied static hardware declarations compose with its
physical contracts, while every real-world state remains explicitly
`not_assessed`.

A later runtime-evidence contract may bind an observation stream to an hardware compatibility graph
device/port instance and carry time, uncertainty, calibration identity, health,
and provenance. A simulation consumer may bind the same physical composition graph contract and hardware compatibility graph
device model to geometry and dynamics. Neither later layer may rewrite physical composition graph
physical meaning or promote hardware compatibility graph declaration state without new evidence.
