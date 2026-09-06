# Metadata System Concepts

This page names the public parts of the fisiCs metadata system so users can
navigate it by purpose, authored inputs, generated outputs, and evidence level.

## Semantic Declaration Metadata

The `semantic-metadata` overlay attaches canonical identities, domains, roles,
and `produces` or `consumes` relationships to supported C declarations.

## Translation-Unit Metadata Export

`--emit-semantic-metadata-json` writes one deterministic, machine-readable
receipt for one analyzed translation unit. It contains accepted declaration
records, relationship edges, source identity, and source provenance.

## Semantic Project Graph

The semantic project graph combines fresh translation-unit metadata artifacts,
resolves project-wide identities and relationships, rejects conflicts, and
retains explicitly allowed external targets.

## Physical Contract

A physical contract describes a semantic subject using independent facets:
quantity kind, dimension, unit policy, value form, reference semantics, and an
optional coordinate frame. None of these facets is inferred from a name.

## Physical Composition Graph

The physical composition graph joins the semantic project graph, current-source
units evidence, and authored physical contracts. It reports scoped static
compatibility and explicit unverified or unbound states.

## Hardware Component Catalog

A hardware component catalog contains reusable declarations for boards, buses,
device models, driver contracts, endpoints, attachment ports, and physical
ports. A catalog entry is an authored declaration, not proof that hardware is
present or genuine.

## Hardware Topology

A hardware topology describes configured node, bus, device, connection,
resource-claim, and physical-port-binding instances for one project.

## Hardware Compatibility Graph

The hardware compatibility graph validates that supplied component catalogs,
configured topology, and physical contracts compose without contradiction. It
does not prove presence, wiring, driver execution, calibration, telemetry,
deployment, safety, or physical acceptance.

## Static Physical-System Graph Pipeline

The complete offline flow is:

```text
annotated C source
    -> translation-unit semantic and units evidence
    -> semantic project graph
    -> physical composition graph
    -> hardware compatibility graph
```

Each generated layer verifies the provenance required by the next. When source
or an authored input changes, regenerate from the earliest stale artifact; do
not edit generated receipts or digests by hand.

## Evidence Vocabulary

Use the strongest term supported by current evidence:

| Term | Meaning |
| --- | --- |
| Authored | present in source, a manifest, a catalog, or a topology file |
| Compiler-validated | accepted by the selected compiler overlay |
| Statically compatible | the relevant offline graph completed successfully |
| Simulated | an identified model executed with stated inputs |
| Runtime-observed | fresh process, telemetry, or device evidence exists |
| Physically accepted | an explicit physical test and acceptance record exists |

A stronger state never follows automatically from a weaker one.
