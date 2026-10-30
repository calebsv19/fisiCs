# Physical representation contracts (policy v5)

P6.2 contract: opt-in compile-time storage bindings alongside unchanged SI Dim8,
quantity kind and reference role. This version adds `representations` and
`shape_helpers` to the exact v4 policy root. Policies and receipts v1–v4 retain
their existing schemas and fingerprints. The v5 operation submodel stays v4's
version 3; representation submodel version 1 is separately fingerprinted.

## Storage and meaning

A representation has exact fields `id`, `storage` (`record` or `array`), `scalar`
(`float` or `double`), `rank` (1 or 2), `extent` (2 or 3), `fields`, `parity`.
Rank 1 uses polar or 3D axial interpretation; rank 2 uses `tensor`, meaning only
homogeneous row-major square storage, with no tensor contraction rules.

Record vectors enumerate every scalar field in declaration order. Rank2 records
have exactly one array field, named by `fields`, with the exact square extents.
Array descriptors have an empty field list and bind only direct fixed storage.
Array parameters, pointers, VLA, flexible arrays, unions, nested/strided storage,
opaque aliases and higher ranks are unsupported. Resolved record typedefs and
const-qualified values use the same checks. Existing ordinary C type compatibility
continues to govern assignment/calls; an adapter is required between different
C storage types even when physical shapes match.

Use `FISICS_REPRESENTATION("rep.id")` alongside `FISICS_QUANTITY_KIND` and
`FISICS_DIM` on each declaration/parameter/result slot. It requires a v5 model
and a linear/difference kind; absolute point vectors require the separate [v6 point contract](physical_points.md). A physical
kind is not inferred from a C type name. The same storage can carry different
explicit kinds. Scalar/planar torque under old policies remains valid.

## Helpers

A helper entry has `id`, `operation`, `operand_kinds` (two ordered canonical IDs),
`operand_representations` (two IDs or null for scalar), `result_kind`,
`result_representation` (ID or null), `preconditions`, `provenance`.
Allowed operations are same-kind/same-representation add/subtract, vector × ratio
scale, force dot displacement → scalar energy, and 3D displacement cross force →
axial vector torque. No 2D cross, point-vector arithmetic, generic tensor products
or stiffness multiplication is inferred. Dot supports 2D/3D polar vectors.
Precision must agree throughout a helper. Helpers use records by value.

Bind `FISICS_PHYSICAL_OPERATION("helper.id")` to the function result declaration.
Resolution uses the captured C callee identity and validates its complete ordered
signature; spelling or destination annotations cannot select an operation.
The exact precondition is `common_cartesian_basis_right_handed`; provenance is
`declared_implementation_and_basis`. The compiler checks input storage/kinds and
signature but trusts the implementation and authored basis assumptions. Frames
are explicitly **unassessed** until P6.3. Helper result trust requires the existing
strict declared-boundary permission; a trust-only scope still cannot pass.

## Evidence and limits

The interned model is bounded to 256 representations and 256 helper contracts,
within existing 1 MiB policy and allocation limits. Its deterministic fingerprint
includes IDs, precision, storage, ordered fields, rank/extents/parity and helper
kinds/reference roles, representations and preconditions. Loads are atomic.
Runtime C values have no descriptor payload or wrapper.

V5 receipts add a representation model and explicit per-fact representation ID,
fingerprint, component depth and unassessed frame status. Cross-TU checks accept
only admitted direct carrier signatures, preserving unsupported-pointer/callback
refusals. These are freshness and declaration-consistency receipts, not signatures
or callee-body proofs.

## Source inventory and pilot boundary

`core_math` uses float x/y and x/y/z records, by-value add/sub/scale/dot/cross,
and pointer-based normalization. Only by-value routes are candidates here.
Cosmic OrbitBody mixes IDs, labels, physical double components, radius/mass/energy
and controls; it is never one homogeneous vector. Its component groups require
explicit copied adapters; absolute positions use the separate [v6 affine family](physical_points.md).
Structural translation[4]/rotation[2] index node DOFs, not geometric vectors;
its stiffness blocks carry different force/angle/length roles. Packed stresses
and mixed records are not admitted by size alone. The homogeneous 3×3 `axes`
member of CoreStructuralSpatialBasis can be copied into an explicitly bound
storage adapter; this establishes no orthonormality or frame transformation.

Qualification must pair valid copies/component routes with wrong order/type/
extent/parity/kind, removal, raw-cast, DOF and heterogeneous controls; compare
real helper numerics and orientation mutants independently; preserve old scalar
acceptance; check C layout/calling ABI and unchanged compile/RSS budgets.

## Minimal binding example

The checked fixture model is `tests/fixtures/physical_semantics/representations/`:

```c
#include <fisics/extensions.h>
typedef struct Force3 { float x, y, z; } Force3;
#define FORCE3 FISICS_DIM(force) FISICS_QUANTITY_KIND("quantity.force") \
    FISICS_REPRESENTATION("rep.record.float.r1.n3.polar")
FORCE3 Force3 copy(Force3 input FORCE3) {
    Force3 result FORCE3 = input;
    return result;
}
```

The example establishes two checked copy/return obligations under the fixture
v5 policy. It requires no helper or trust permission. Native attributes and the
public macros have identical physical meaning; the macros erase under Clang.

`physical-representation-contract-test` covers the finite model and source
routes. Source pilot, sanitizer, allocation-ledger and cost runners retain
separate evidence; passing the fast target alone does not establish all of them.
On the qualified arm64 compiler, a PhysicalFact is56 bytes (previous scalar fact
layout48); an interned representation is56 and helper48 bytes plus owned strings.
These are compiler structures only, not the values or the public frontend ABI.

## Frame declaration staging

[Policy v6](physical_frames.md) now validates authored frame and reference
annotations on admitted vector carriers. [P6.3.3 propagation](physical_frame_propagation.md)
now checks static frame/axis compatibility; [P6.3.4 rotations](physical_rotations.md)
check exact directed calls while retaining helper body and basis trust. V5 facts still report frames unassessed. [V6 receipts](physical_coverage.md#version-6-frame-evidence) now carry exact context
facets. The current compiler fact is64 bytes, with optional36-byte interned
contexts; the56-byte figure above describes the original v5 checkpoint.
[Position records](physical_points.md) use the bounded3D double affine family.

## Cosmic 3D double extension

[V7 Cosmic spatial contracts](physical_cosmic_spatial_contracts.md) add finite
unary norm and velocity/angular products on exact 3D double records. A `norm`
model entry has one operand slot; existing binary entries retain two. Existing
policies/fingerprints and force/work/torque reference requirements are unchanged.
Implementation and numeric basis trust remain explicit.
