# fisiCs Extension Agent Guide

Status: repository-owned operating contract

Last verified: 2026-09-03

This guide tells coding agents how to choose, author, inspect, and validate the
current fisiCs extension and static-graph layers. It is an agent-facing router,
not a replacement for the individual language and artifact contracts.

## Source-Of-Truth Order

When documentation and an example appear to disagree, use this order:

1. current compiler source and CLI behavior;
2. the focused contract for the selected layer;
3. the public example and its checked script;
4. this routing guide;
5. external program or skill notes.

Do not silently invent syntax, schema fields, runtime behavior, or physical
claims. If current behavior differs from a public contract, report the drift
and use the narrowest verification path that demonstrates it.

## Route The Request First

Choose the smallest layer that can answer the user's request:

| User intent | Route | Primary evidence |
| --- | --- | --- |
| Compile ordinary portable C | no overlay | object, executable, or normal tests |
| Validate physical arithmetic | `physics-units` | diagnostics, semantic dump, units JSON |
| Check direct allocation/free lifecycle | `memory-check` | instrumented runtime report |
| Attach stable meaning to declarations | `semantic-metadata` | semantic dump or translation-unit metadata export JSON |
| Resolve identities across source files | semantic project graph | fresh translation-unit inputs and resolved project graph |
| Describe quantities, references, and frames | physical composition graph | semantic project graph, units evidence, physical contracts |
| Validate authored catalog/topology compatibility | hardware compatibility graph | physical composition graph, catalogs, topology, scoped static assessment |
| Prove a device exists or behaves correctly | no current static layer | separately authorized runtime and physical evidence |

Do not invoke offline graph pipeline merely because a declaration has a hardware-like
name. A canonical semantic identity describes what the author means; it does
not infer a category or establish physical existence.

## Keep The Two Layer Families Distinct

`physics-units`, `memory-check`, and `semantic-metadata` are opt-in compiler
overlays applied while compiling C translation units.

The semantic project, physical composition, and hardware compatibility graphs
are terminal offline artifact modes:

```text
C source
  -> translation-unit semantic and units evidence
  -> resolved semantic project graph
  -> physical composition graph
  -> static hardware compatibility graph
```

Do not pass normal compilation work into an offline graph invocation. Do not
treat an offline graph as if it rewrites, links, runs, flashes, or deploys C.

## Author Source Conservatively

For fisiCs-only compiler tests or examples, native attributes are the canonical
syntax:

```c
[[fisics::id("signal.room.temperature")]]
double room_temperature
    [[fisics::dim(temperature)]]
    [[fisics::unit(kelvin)]] = 294.0;
```

For source intentionally shared with strict Clang, include the portability
header and use its declaration macros:

```c
#include <fisics/extensions.h>

FISICS_ID("signal.room.temperature")
double room_temperature
    FISICS_DIM(temperature)
    FISICS_UNIT(kelvin) = 294.0;
```

The macros erase for non-fisiCs compilers. That preserves ordinary-C source
compatibility; it does not give Clang units or metadata semantics.

Do not perform broad source migration to the macros as a side effect of agent
guide, documentation, or graph work. Macro adoption across existing examples
and programs is a separate behavior-sensitive adoption effort with its own corpus
and dual-toolchain checks.

## Authoring Responsibilities

### Physics units

- Treat `dim(...)` as physical semantic truth.
- Treat `unit(...)` as concrete representation identity.
- Prefer canonical singular `snake_case` unit names.
- Keep conversion explicit at input, storage, subsystem, and display
  boundaries.
- Do not annotate counters, ids, or retry limits as physical quantities.

Use the specialized `fisics-units-authoring` skill when it is available.

### Semantic metadata

- Give `id` a canonical authored semantic identity.
- Use no more than one `domain` and one `role` on a declaration.
- Use repeatable `produces` and `consumes` edges for relationships.
- Preserve source identity even when no physical contract identity is appropriate.
- Do not derive behavior from spelling conventions such as `device.` or
  `signal.`.

### Project and hardware manifests

- Keep authored manifests separate from generated artifacts.
- Use exact source paths and current digests.
- Resolve duplicate identities according to the semantic project graph contract; do not merge
  unrelated declarations because their display names are similar.
- Keep vendor catalogs reusable and project topology authored separately.
- Model only facts supplied by the source, manifest, catalog, or topology.

## Artifact Freshness And Provenance

Before consuming a generated artifact, verify:

1. its schema and contract version are supported;
2. every recorded source path identifies the intended current source;
3. required source or input digests still match;
4. all prerequisite artifacts belong to the same intended project run;
5. the output location is generated space, not an authored manifest path; and
6. the producing command completed successfully.

The offline graph pipeline readers enforce their own provenance requirements. Agents
must not weaken or bypass those checks to make stale evidence pass. Regenerate
the earliest stale input and then rebuild downstream artifacts in order.

Generated example output belongs under `build/`. Public authored fixtures and
manifests belong under `examples/` or `tests/fixtures/`.

## Evidence Vocabulary

Use exact claim levels in reports and generated documentation:

| Claim | What can establish it |
| --- | --- |
| Authored | source, manifest, catalog, or topology contains the declaration |
| Compiler-validated | the relevant overlay accepted current source |
| Statically compatible | the relevant semantic, physical, or hardware graph completed successfully |
| Simulated | an identified simulation executed with stated inputs/model |
| Runtime-observed | fresh telemetry or process/device execution evidence |
| Physically accepted | an explicit physical test and acceptance record |
| Built | a compiler or build command succeeded |
| Packaged | the requested package workflow succeeded |
| Released/deployed/flashed | only the corresponding separately authorized operation and readback |

A stronger claim never follows automatically from a weaker one. In
particular, built is not deployed, statically compatible is not
runtime-observed, and simulated is not physically accepted.

## Validation Matrix

Run the narrowest gate that proves the touched surface, then widen according to
`docs/compiler_test_confidence_tiers.md` when compiler behavior changed.

| Touched surface | Minimum public gate |
| --- | --- |
| Extension routing/docs only | Markdown links, stale references, `git diff --check` |
| Units authoring example | `make examples-physics-units` |
| Semantic declaration authoring | `make examples-semantic-metadata` |
| One shared semantic source | `make examples-semantic-metadata-compare` |
| Digest-bound ESP portable C intake | `make integration-esp-portable-c ESP_WORK_ROOT=/absolute/path/to/ESP_Work` |
| E016 semantic and physical composition | `make integration-esp-semantic-physical ESP_WORK_ROOT=/absolute/path/to/ESP_Work` |
| E016 static hardware compatibility | `make integration-esp-hardware-compatibility ESP_WORK_ROOT=/absolute/path/to/ESP_Work` |
| E016 deterministic positive/negative proof | `make integration-esp-deterministic-proof ESP_WORK_ROOT=/absolute/path/to/ESP_Work` |
| E016 retained realization/evidence receipt | `make integration-esp-realization-evidence ESP_WORK_ROOT=/absolute/path/to/ESP_Work` |
| Project-neutral deployment/raw-observation receipt validation | `make integration-external-evidence-receipts` |
| Public macro adoption or parity | `make examples-extension-portability` |
| Static physical-system manifests or examples | `make examples-semantic-hardware` |
| Overlay/compiler implementation | focused owning integration target, then the required confidence tier |
| Public release claims | `docs/release_confidence_checklist.md`; release actions remain separately authorized |

When diagnosing rather than changing behavior, gather evidence and report the
cause. Do not implement a fix unless the request includes implementation.

## Failure Routing

- Parse or attachment failure: inspect the selected overlay and declaration
  placement.
- Dimension/unit mismatch: use the units specialist and inspect the last legal
  formula intermediate.
- Unknown or duplicate semantic identity: inspect translation-unit declaration ownership,
  then the semantic project graph manifest.
- Stale digest/source rejection: regenerate upstream artifacts; never edit a
  digest to force acceptance.
- Quantity, reference, or coordinate-frame mismatch: inspect physical
  contracts and units evidence.
- Profile, model, address, bus, driver, or binding conflict: inspect hardware
  catalog/topology inputs.
- Missing live behavior: stop at the static boundary and request or design a
  separate runtime/physical evidence lane.
- Narrative-only runtime evidence: preserve it as documentary history; do not
  promote it to raw-capture-bound evidence. Use
  `docs/external_evidence_receipts.md` for the next exact chain.

## Agent Completion Report

For extension work, report:

1. selected layer and why it was sufficient;
2. authored files versus generated artifacts;
3. exact validation commands and outcomes;
4. the strongest evidence level actually established;
5. explicit non-claims; and
6. the smallest next authority if runtime, hardware, packaging, or release work
   remains.

## Public References

- [Extension Selection And Quickstart](extensions_guide.md)
- [Portable Extension Authoring](extension_portability.md)
- [Extension Overlay Reference](extension_overlays.md)
- [Metadata System Concepts](metadata_system_concepts.md)
- [Semantic Metadata Contract](semantic_metadata_contract.md)
- [Translation-Unit Metadata Export Contract](semantic_metadata_export_contract.md)
- [Semantic Project Graph Contract](semantic_metadata_project_graph_contract.md)
- [Physical Composition Graph Contract](semantic_metadata_physical_composition_contract.md)
- [Hardware Compatibility Graph Contract](semantic_metadata_hardware_contract.md)
- [External Deployment And Observation Receipts](external_evidence_receipts.md)
- [ESP Portable C And Static Graph Case Study](esp_compiler_bridge_case_study.md)
- [Semantic Metadata Examples](../examples/semantic_metadata/README.md)

## Current Macro Adoption Boundary

The repository public examples have completed a focused portable-source macro
adoption pass. Native compiler/parser fixtures and probes remain direct syntax
by design. The permanent parity gate is:

```bash
make examples-extension-portability
```

Future migration of external customer repositories must be separately scoped.
For each customer, inventory direct attributes, preserve fisiCs-only fixtures,
migrate only shared-source surfaces, and prove strict Clang plus fisiCs
semantic and runtime parity before changing its adoption status.
