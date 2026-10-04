# Optional upstream attachment provenance

Status: source-tool producer receipt v1 and attachment handoff v2 implemented.
This is a separate, opt-in layer; existing graph-v1 bytes and pinned AHT20
artifacts are unchanged. It adds no compiler syntax or native runtime behavior.

## Produce a receipt

First generate a complete semantic graph and physical manifest using the
existing compiler workflow. With quiescent inputs and a new output directory:

```sh
make examples-semantic-hardware
python3 -B tools/physical_graph_provenance.py \
  --compiler ./fisics \
  --manifest build/examples/semantic_metadata/hardware_project/physical.manifest.json \
  --out /absolute/path/to/new-provenance-bundle
```

The wrapper invokes the selected compiler to generate `physical.graph.json`,
retains the exact semantic input as `semantic.graph.json`, and writes
`provenance.json` last. It checks semantic bytes, manifest bytes and compiler
digest before/after invocation, complete graph schemas, project agreement and
the emitted semantic-parent path. Existing output directories are rejected.
A failure can leave an incomplete new directory; directory existence is not
success. Require exit 0 and the receipt. No simulation or hardware is executed.

The producer receipt has exactly these fields:

- `schema`: `fisiCs.physical_graph_provenance`; `schema_version`: integer 1.
- `producer`: `operation: physical_project_graph` and `compiler_sha256`.
- `project_id` and `manifest_sha256`.
- `semantic` and `physical`: each has lowercase `sha256` and integer
  `length_bytes`, computed over the exact respective graph bytes.

## Admit the association

Generate the hardware graph with its manifest pointing to the newly produced
physical graph. Place that graph and an authored handoff manifest in the bundle.
Use the normal [handoff contract](attachment_handoff_contract.md), set
`schema_version` to 2, and add:

```json
"provenance": {"path": "provenance.json", "sha256": "<receipt-byte SHA-256>"}
```

Compute all three artifact descriptors from the retained bundle bytes. The
handoff verifier checks the receipt digest, exact producer schema/version,
project, and both parent/output SHA-256 values and lengths. Missing provenance
or mismatched parent/output bytes rejects v2, even if an artifact pin was
updated. Success adds `provenance_sha256` and evidence
`semantic_parent: producer_receipt_verified` to receipt version 2.

V1 remains accepted without this receipt and makes no producer-parent claim.
Changing an input to v1 is a caller policy choice, not an automatic downgrade;
consumers requiring provenance must require successful receipt version 2.
The common rejected CLI envelope remains version 1 and never authorizes intake.

## Evidence limits and next boundary

The receipt is unsigned. Anyone able to replace both manifest and receipt can
author another consistent bundle; the verifier does not authenticate the
producer or reopen the compiler/manifest to establish current-source freshness.
Compiler and manifest hashes are recorded provenance, not independently trusted
execution attestations. Use a trusted expected manifest digest for admission.

Before/after checks assume quiescent inputs; they are not an atomic snapshot and
cannot detect a transient mutation restored during execution. This version
does not snapshot or bind the full units-metadata/catalog/source closure. It
also does not replace full compiler graph validation, the existing hardware
physical-parent check, or consumer execution and physical acceptance evidence.

The separate [frozen offline graph input contract](frozen_graph_inputs.md) now
captures all data inputs consumed by the three offline stages and replays them
without live-source reads. It retains this producer-parent format unchanged.
Complete C preprocessing/export reproducibility is not part of either contract.
Authentication and packaging remain separate approvals, not implied by hashes.

## Qualification

`make test-attachment-handoff` includes real-compiler production twice with
identical receipt bytes; successful v2 admission; rejection of missing receipts,
unsupported versions, substituted semantic input and incorrect physical output
length; v1 backward compatibility; and create-only output enforcement.
