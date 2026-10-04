# Declared attachment handoff contract

Status: v1 and opt-in v2 read-only source-tool interfaces implemented. Not a runtime or a
packaged compiler CLI flag. Python 3.9+ standard library only.

Verification checkpoint (2026-09-12): the public compiler hardware example,
11 baseline handoff groups, six freshly compiler-generated physical variants
with 36 facet rejections, and optional producer-provenance checks pass. The
AHT20 declaration migration passed two deterministic executions and 134 bound
records with unchanged graph/run pins. Nine focused AHT20 groups cover the
preserved API and mandatory delegation to the generic verifier.

## Audit decision

The existing language and graph layers already supply the required attachment
facts. No new annotation, AST field, quantity system or compiler parser change
is needed for this boundary.

| Concern | Owner and disposition |
| --- | --- |
| Source identity, roles and relationships | Existing semantic metadata/project graph; reuse |
| Quantity, dimensions, unit policy, value shape, reference and frame | Existing physical composition graph; preserve without narrowing to scalar sensors |
| Device/model/driver revisions and physical-port joins | Existing hardware graph; reuse |
| Authored adapter reference and expected physical contract | New project-neutral handoff manifest |
| AHT20 instance 12, rear-left selection, raw scaling and profile execution | Existing integration witness; not generic compiler semantics |
| Receiver freshness, simulation model availability and execution | Consumer-owned; never inferred by the handoff verifier |
| Deployment and physical acceptance | Separate evidence contracts |

The AHT20 declaration policy now lives in `tools/aht20_declaration.py` and
delegates generic graph joins to this verifier. The integration runner keeps its
legacy `validate` API; temporary bundle materialization changes neither retained
result format nor digest pins. The native witness's legacy result/projection
import remains a separate consumer-owned coupling. This declaration migration
does not replace that execution-result verifier.

## Entry points

From a source checkout:

```sh
python3 -B tools/attachment_handoff.py /path/to/bundle/handoff.json
make test-attachment-handoff
```

The verifier writes one deterministic JSON receipt to stdout, creates no files,
and returns 0 for verified input or 2 for rejected input. CLI usage errors use
argparse's stderr/exit-2 behavior. Consumers must require both exit 0 and the
expected receipt schema/version/status; do not parse human error text.

Python consumers may import `verify_handoff(path)` from
`tools/attachment_handoff.py`. It returns the receipt dictionary or raises
`HandoffError`. No test runner, ESP checkout, installed application, source
compiler binary, hardware access or third-party Python package is required.
The CLI JSON surface is the preferred cross-process interface. Release archive
inclusion has not been performed or verified by this work.

## Input schema

The v1 root has exactly these fields:

```json
{
  "schema": "fisiCs.attachment_handoff",
  "schema_version": 1,
  "project_id": "my.project",
  "artifacts": {
    "semantic": {"path": "semantic.graph.json", "sha256": "<64 lowercase hex>"},
    "physical": {"path": "physical.graph.json", "sha256": "<64 lowercase hex>"},
    "hardware": {"path": "hardware.graph.json", "sha256": "<64 lowercase hex>"}
  },
  "attachments": [{
    "attachment_id": "attachment.sensor",
    "device_id": "device.sensor",
    "adapter": {"id": "consumer.sensor_model", "revision": "1"},
    "ports": [{
      "port_id": "port.measurement",
      "subject_id": "signal.temperature",
      "physical_contract": {
        "quantity_kind": "quantity.temperature.absolute",
        "unit_policy": {"mode": "exact_unit", "unit": "kelvin"},
        "value_form": {"kind": "scalar", "component_count": null, "tensor_rank": null, "tensor_shape": []},
        "reference_semantics": {"kind": "absolute", "reference_subject": null, "period_binary64": null},
        "frame_id": null
      }
    }]
  }]
}
```

The digest placeholders above are explanatory, not a runnable fixture.
`adapter` may instead be null. Names have no executable meaning, paths or module
import semantics. V1 permits 1–256 attachments and at most one attachment per
device. Duplicate attachment IDs, device assignments or port IDs are rejected.
Every required physical port on an attached model must be included; unselected
devices are outside this handoff's attachment scope.

Artifact paths are canonical relative POSIX paths inside the bundle. Absolute
paths, traversal, backslashes and symlinks in artifact paths are rejected.
Each JSON artifact is bounded to 16 MiB. Duplicate object keys, nonfinite JSON,
unsupported versions (including boolean version values), unknown manifest or
attachment fields, missing files and byte-digest mismatches fail closed.

V2 adds exactly one root field, `provenance`, with the same `{path, sha256}`
descriptor shape. It requires a producer receipt binding the semantic input and
physical output bytes; see [optional upstream provenance](attachment_handoff_provenance.md).
All three graph schemas remain version 1. V1 artifacts retain their original
weaker semantic-parent guarantee and are not silently upgraded.

## Verification and output

The verifier checks graph family/version, complete flags, project agreement,
the hardware graph's existing physical-parent length/hash receipt, selected
source/contract indices and exact device/model/physical-port/semantic-subject
joins. The five expected physical-contract fields must equal the graph's
quantity, unit policy, value form, reference semantics and frame, including JSON
types. It does not insert conversions, discard reference semantics or promote
a dimension-only policy to a concrete unit.

Success uses `fisiCs.attachment_handoff_receipt`, the input version, status `verified`.
It includes the manifest SHA-256, three graph SHA-256 values and sorted
attachment/port projections with source records, quantity definitions/dimension
vectors, physical contracts, compiler conversion recipes, port directions, and model/driver references.
The `conversion` projection is additive to the initial source-tool receipt;
consumers must not assume receipts from older helper revisions have identical
bytes. Existing input graph bytes and AHT20 retained-result schemas are unchanged.
Output has no timestamps or resolved bundle path, so relocating unchanged bytes
does not change the receipt. Embedded original source locations remain intact.

Attachment state is `unbound` for a null adapter, otherwise `declared`.
`unsupported`, consumer admission and executable support require the consuming
program's own capability check; the compiler verifier cannot establish them.
Incompatible input returns `rejected`, never a partially successful receipt.
The common CLI rejection envelope remains receipt version 1 for both inputs.

## Evidence limits

Verification means unsigned artifact integrity and selected reference joins.
It does **not** rerun full compiler graph validation or authenticate who produced
the graphs. In particular, older physical graphs do not carry a semantic-parent
content digest; the handoff pins both artifacts and checks selected joins, not
an authenticated semantic production chain. Current-source freshness is not
checked, and embedded source paths are never followed.

Generate graphs through their existing compiler gates before handing them off.
Use an independently trusted manifest digest if authenticity/admission matters.
Verification assumes inputs are not concurrently modified. The receipt does
not grant execution, driver existence, simulation fidelity, calibration, safety
authority, deployment or physical acceptance.

## Hardware-free example and proof

```sh
make examples-semantic-hardware
python3 -B examples/semantic_metadata/run_attachment_handoff.py \
  --out /path/to/new-handoff-directory
python3 -B tools/attachment_handoff.py /path/to/new-handoff-directory/handoff.json
```

The authored example lives in
`examples/semantic_metadata/hardware_project/attachments.json`. It deliberately
uses an absolute-temperature producer and a temperature-difference command
consumer. These are metadata contract witnesses, not a real fan driver or
simulation. One adapter is declared and one remains unbound. This second
non-CAN witness proves neither AHT20 nor ESP names are built into the interface.
The example writer refuses an existing directory and may leave an incomplete
directory on failure; only a successful receipt establishes completion.

## Qualified physical declarations

`test_attachment_physical_cases.py` regenerates semantic, physical and hardware
graphs twice per case, proving deterministic output for vector/frame,
tensor/frame, relative-reference, cyclic-period, convertible-unit and
dimension-only contracts. Every expected facet is checked, including well-typed
but different frames, shapes, references, periods and unit policies.

These are declaration witnesses over the public temperature example, not
implementations of vector storage, coordinate transforms or periodic thermal
dynamics. Convertible-unit coverage uses the supported kelvin identity recipe;
it does not establish non-identity conversion execution. Conversion recipes are
preserved, never executed. Semantic-ID subjects and source-compatible contracts
remain required; external/non-source-compatible admission is not expanded.

## Next compiler-owned actions

1. The [frozen offline input contract](frozen_graph_inputs.md) now closes the
   graph data-input snapshot/replay boundary. Use its separate build-receipt
   verification when frozen-input evidence is required. Current live-source
   freshness, full C preprocessing/export reproducibility, and authentication
   remain distinct; the optional v2 receipt still means only parent-byte association.
2. Qualify concrete non-identity conversion and broader physical cases against
   existing registry capabilities before expanding admission. Require explicit
   policy for external or unverified source contracts; no fallback admission.
3. Consumer owners can adopt the declaration API independently while retaining
   their model, snapshot, quality and freshness checks. No native changes were
   made by this compiler-side migration.
4. Include tools and examples in a future approved packaging lane and prove
   isolated archive usage. No release or distribution changes are included.
