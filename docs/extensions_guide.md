# fisiCs Extension Selection And Quickstart

This is the shortest public guide for choosing and using the current fisiCs
extension and static graph layers. Ordinary C compilation remains the default;
every extension is explicit.

Agents applying these layers should also follow the repository-owned
[Extension Agent Guide](agent_extension_guide.md), especially its provenance,
evidence-language, and completion-report boundaries.

For concise definitions of each capability, see
[Metadata System Concepts](metadata_system_concepts.md).

For a read-only handoff of declared device/port associations to an external
tool, use [Attachment Handoff](attachment_handoff_contract.md). This source-tool
interface checks retained graph references; it is not an execution overlay.
For copied-input offline graph regeneration and detached verification, use
[Frozen Offline Graph Inputs](frozen_graph_inputs.md). This is not full C-build
reproducibility or model execution.

## Choose The Smallest Relevant Layer

| Need | Use | Result |
| --- | --- | --- |
| Compile ordinary C | no overlay | normal object or executable |
| Validate physical arithmetic | `physics-units` | compile-time dimension and concrete-unit diagnostics |
| Inspect direct allocation/free lifecycle | `memory-check` | runtime summary, leak, and misuse diagnostics |
| Attach stable meaning to declarations | `semantic-metadata` | typed IDs, domains, roles, and relationships |
| Resolve metadata across translation units | semantic project graph | deterministic project-wide identity graph |
| Describe physical meaning | physical composition graph | deterministic quantity/unit/reference compatibility graph |
| Validate supplied devices and topology | hardware compatibility graph | deterministic static hardware compatibility assessment |
| Prove hardware exists or works | not currently supported | future runtime and physical evidence is required |

`physics-units`, `memory-check`, and `semantic-metadata` are compiler overlays.
The semantic project, physical composition, and hardware compatibility graphs
are terminal offline artifact modes, not overlay names.

## 1. Physics Units

Canonical direct syntax:

```c
double distance [[fisics::dim(length)]] [[fisics::unit(meter)]] = 2.0;
```

Portable shared-source syntax:

```c
#include <fisics/extensions.h>

double distance FISICS_DIM(length) FISICS_UNIT(meter) = 2.0;
```

Compile with:

```bash
./fisics --overlay=physics-units -c source.c -o source.o
```

Use [the physics-units example](../examples/physics_units/README.md) for valid
formulae, intentional failures, and conversion guidance.

## 2. Memory Check

Compile and run with:

```bash
./fisics --overlay=memory-check source.c -o app
./app
```

This instruments direct `malloc`, `calloc`, `realloc`, and `free` calls. It is
not a general memory-safety sanitizer. See
[the memory-check example](../examples/memory_check/README.md).

## 3. Semantic Metadata

Canonical direct syntax:

```c
[[fisics::id("method.room.observe")]]
[[fisics::domain("thermal")]]
[[fisics::role("sensor.read")]]
[[fisics::produces("signal.room.temperature")]]
double observe_temperature(void);
```

Portable shared-source syntax:

```c
#include <fisics/extensions.h>

FISICS_ID("method.room.observe")
FISICS_DOMAIN("thermal")
FISICS_ROLE("sensor.read")
FISICS_PRODUCES("signal.room.temperature")
double observe_temperature(void);
```

Inspect the semantic model:

```bash
./fisics --overlay=semantic-metadata --dump-sema \
  -c source.c -o source.o
```

Emit deterministic translation-unit metadata:

```bash
./fisics --overlay=semantic-metadata \
  --emit-semantic-metadata-json metadata.json \
  -c source.c -o source.o
```

The supported attributes are:

| Attribute | Meaning |
| --- | --- |
| `id` | canonical authored semantic identity |
| `domain` | one compiler-known domain classification |
| `role` | one compiler-known role classification |
| `produces` | repeatable authored relationship to a canonical target identity |
| `consumes` | repeatable authored relationship to a canonical target identity |

Names are semantic identities, not inferred categories. For example, a
`device.` prefix does not prove that a device exists.

Run the smallest public example with:

```bash
make examples-semantic-metadata
```

## 4. Project, Physical, And Hardware Graphs

The layered artifact flow is:

```text
annotated C source
    -> translation-unit semantic and units artifacts
    -> semantic project graph
    -> physical composition graph
    -> hardware compatibility graph
```

The semantic project graph resolves authored identities across fresh
translation-unit artifacts:

```bash
./fisics --semantic-project-manifest semantic.manifest.json \
  --emit-semantic-project-graph-json semantic.graph.json
```

The physical composition graph combines semantic identity, units evidence, and
physical contracts:

```bash
./fisics --physical-project-manifest physical.manifest.json \
  --emit-physical-project-graph-json physical.graph.json
```

The hardware compatibility graph combines the physical graph, reusable
component catalogs, and topology:

```bash
./fisics --hardware-project-manifest hardware.manifest.json \
  --emit-hardware-project-graph-json hardware.graph.json
```

All three are terminal offline modes. They reject compilation flags and other
unrelated operations. Inputs are provenance-checked and outputs are published
atomically.

Run the complete example with:

```bash
make examples-semantic-hardware
```

## 5. Clang And fisiCs Together

Direct `[[fisics::...]]` attributes are the native syntax, but Clang diagnoses
them as unknown attributes and, in C99 mode, as C23 extension syntax. Use
`<fisics/extensions.h>` when the same source must pass a strict Clang build.

```bash
make examples-semantic-metadata-compare
make examples-extension-portability
```

The comparison proves ordinary runtime parity while separately proving fisiCs
metadata and units output. See
[Portable Extension Authoring](extension_portability.md) for the exact
contract.

## 6. What Static Graphs Do Not Prove

No current metadata or graph artifact proves:

- hardware presence, identity, wiring, or electrical correctness;
- driver implementation or execution;
- telemetry freshness or runtime health;
- calibration;
- simulation accuracy;
- deployment or flashing; or
- physical acceptance.

Those require separately designed evidence layers. Static declarations must
not be promoted into runtime claims.

## Reference Contracts

- [Extension Overlay Reference](extension_overlays.md)
- [Metadata System Concepts](metadata_system_concepts.md)
- [Semantic Metadata Contract](semantic_metadata_contract.md)
- [Translation-Unit Metadata Export Contract](semantic_metadata_export_contract.md)
- [Semantic Project Graph Contract](semantic_metadata_project_graph_contract.md)
- [Physical Composition Graph Contract](semantic_metadata_physical_composition_contract.md)
- [Hardware Compatibility Graph Contract](semantic_metadata_hardware_contract.md)
