# Frame model and declaration contracts (policy v6)

P6.3.2 adds an opt-in compiler-owned model and declaration validation alongside
unchanged Dim8, quantity kinds, affine roles, and C storage. P6.3.3 now adds
[static frame/axis propagation](physical_frame_propagation.md); directed rotation
calls now have [bounded rotation qualification](physical_rotations.md).
V1–v5 policy and receipt behavior, catalog v1, graph schemas, and units schemas
remain unchanged.

## Model

A v6 policy has the ten v5 root fields plus exactly `frame_contexts`,
`reference_points`, `frames`, and `frame_transforms`. Entries use canonical IDs;
unknown/missing/extra/duplicate fields and duplicate IDs (including identical
entries) fail. The repository [fixture](../tests/fixtures/physical_semantics/frames/policy.json)
is a complete authored example.

| Table | Exact entry fields | Ceiling |
| --- | --- | --- |
| `frame_contexts` | `id`, `observer_id`, `state_id` | 64 |
| `reference_points` | `id`, `context_id` | 256 |
| `frames` | `id`, `context_id`, `dimension`, `convention`, `origin_point`, `affine_origin` | 256 |
| `frame_transforms` | `id`, `source_frame`, `target_frame`, `operation`, `input_kind`, `input_representation`, `output_kind`, `output_representation`, `provenance` | 512 |

Frames have dimension 2 or 3 and convention
`cartesian_orthonormal_right_handed`. Their origin point must belong to the same
observer/state context. `affine_origin` is null or an explicitly linked position
origin; equal spelling never substitutes for the link. Coordinate frames,
physical points, affine origins, and moment reference points remain separate.
Observer/state identities are authored premises, not runtime discoveries.

Directed transforms require known endpoints in one context and dimension.
The initial transform subset is 3D double `x/y/z` records by value:

- `rotate_vector`: equal input/output kinds and representations, force or
  displacement with polar storage, or torque with axial storage.
- `map_point`: equal position input/output kinds and polar representations,
  a declared position/displacement affine pair, and position origins on both
  endpoint frames. Loading this model is supported; source point record and
  point-helper bindings are supported by the bounded [point contract](physical_points.md).

Transform provenance is exactly `declared_implementation_and_basis`. Reverse
and identity transforms require explicit entries; no inverse or composition is
inferred. No numeric matrix, moment rebase, reflection, nonorthonormal metric,
moving observer, tensor transform, or dynamic member-instance contract is added.

Tables are sorted before reference handles are assigned. Fingerprints are
order-independent, domain-separated (`fisiCs.frame_model.v1` and
`fisiCs.physical_semantics_policy.v6`), and include directed relationships and
referenced definitions. The standalone frame submodel writer/matcher uses the
six frozen fields: `version`, `fingerprint`, and the four tables, each containing
sorted `{id, fingerprint}` entries. This is an internal model consistency check,
not a published source coverage receipt or a cryptographic signature.

Loads use temporary ownership. Success replaces the live model; every failure
preserves it. Existing 1 MiB policy bounds remain. Internal resource options may
lower the four table limits, never raise them. All new loader-owned allocations
participate in the existing deterministic failure seam.

## Source declarations

Use one attribute per bracket pair. Portable macros from `<fisics/extensions.h>`
erase under ordinary C compilers without adding runtime data or validation.

| Macro | Native attribute | Declaration meaning |
| --- | --- | --- |
| `FISICS_FRAME(id)` | `fisics::frame(id)` | Coordinate frame |
| `FISICS_COMPONENT_AXIS(id)` | `fisics::component_axis(id)` | `axis.x`, `axis.y`, or `axis.z` scalar component |
| `FISICS_FRAME_TRANSFORM(id)` | `fisics::frame_transform(id)` | Directed unary helper on its result declaration |
| `FISICS_POINT_ID(id)` | `fisics::point_id(id)` | Position's physical point subject |
| `FISICS_MOMENT_REFERENCE(id)` | `fisics::moment_reference(id)` | Torque reference point |
| `FISICS_APPLIED_AT(id)` | `fisics::applied_at(id)` | Force application point |
| `FISICS_BETWEEN_POINTS(from,to)` | `fisics::between_points(from,to)` | Directed displacement endpoints |

These annotations require v6 and existing dimensional/quantity declarations.
Framed scalar components require a valid frame axis. Framed vector carriers must
match frame extent and polar/axial meaning and carry no scalar axis annotation.
The existing 2D/3D float/double vector carriers remain available for declarations;
this does not admit 2D/float transforms. Optional component annotations must agree
with the enclosing frame, component order, and supplied references.

Reference annotations require the appropriate quantity role and the same context
as the frame. Positions must agree with the frame's explicit affine-origin link.
Position records use only the bounded 3D double polar family. Per-declarator isolation and
redeclaration inheritance/conflict checks include all seven new families.

A transform result declaration requires a known transform and an exact unary,
nonvariadic, by-value 3D double vector signature. Its parameter declares the source
frame; its result declares the target. Both match the policy kinds and storage.
Declared application/moment/end-point references must be preserved. The [rotation call path](physical_rotations.md) checks actual inputs and derives
the target frame; declaration validation alone does not prove the helper body.

Declaration contexts are interned in the compiler state with a 32-bit handle
(zero unknown), bounded to 65,536 entries. A fact occupies 64 bytes on the qualified
host, versus 56 bytes before this checkpoint; a context now occupies 36 bytes (32 before propagation flags), a frame
32 bytes, and a transform 48 bytes. C values retain their ordinary representation.

## Current acceptance boundary

`--dump-sema` shows the frame model and per-fact frame/axis/reference context,
with static compatibility separated from unassessed numeric geometry. The native
coverage API now qualifies the supported [same-frame subset](physical_frame_propagation.md).
CLI v6 [physical receipts and cross-TU comparison](physical_coverage.md#version-6-frame-evidence)
now retain exact frame facets, model fingerprints and directed helper identities. V1–v5 coverage receipts and strict behavior remain supported.

[Fixed-direction rotation qualification](physical_rotations.md) is available;
[position records and point maps](physical_points.md) are supported. Numeric geometry,
orthonormality, pose freshness, helper success-flow proof, mixed-toolchain numeric/ABI
parity, solver adoption and physical acceptance are not implied by static checks.

## Qualification

```sh
make physical-frame-contract-test
```

The permanent gate exercises 124 commands: canonical ordering, identity and
reference conflicts, malformed tables, production ceilings/lowered limits, atomic
failed replacements, source roles/components/declarator isolation, exact helper
signatures, Clang macro erasure, and preservation of refused receipt outputs.
The native contract sweeps all 92 loader-owned failure thresholds and the qualified
state-allocation thresholds before success, verifies the compact fact size, and
checks internal scope acceptance alongside empty-scope strict refusal. The scoped probe builder accepts
`--frames` with `--mode sanitized` or `--mode ledger`; it instruments new model and
semantic modules and uses the selected frontend archive for existing services.
Parser/JSON/unit-registry allocation internals and the complete compiler are not
claimed to be globally failure-injected or sanitizer-qualified by that probe.
