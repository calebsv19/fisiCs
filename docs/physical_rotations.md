# Explicit directed vector rotations

Policy v6 admits resolved unary `frame_transform` calls for 3D double `x/y/z`
records: polar force/displacement and axial torque. The [frame model](physical_frames.md)
provides explicit directed transform IDs. Dim8, quantity kind and actual C storage
remain authoritative. This adds no runtime payload to C values.

## Exact helper contract

A helper result declares `FISICS_FRAME_TRANSFORM(id)` and the target frame. Its
single by-value parameter declares the source frame. Both declare exactly the
policy kind and representation. Nonvariadic, by-value 3D double storage is
required. The resolved C callee chooses the contract; names, destinations,
function pointers, raw casts, runtime direction flags and generic mutable-basis
parameters cannot select or manufacture one. Two-dimensional, float, tensor and
point transforms remain unsupported at this checkpoint.

For the repository's force transform fixture, the corresponding declaration is:

```c
#include <fisics/extensions.h>
typedef struct Vec3d { double x, y, z; } Vec3d;

Vec3d member_a_force_to_world(
    Vec3d value FISICS_DIM(force) FISICS_QUANTITY_KIND("quantity.force")
        FISICS_REPRESENTATION("rep.record.double.r1.n3.polar")
        FISICS_FRAME("frame.member_a"))
    FISICS_DIM(force) FISICS_QUANTITY_KIND("quantity.force")
    FISICS_REPRESENTATION("rep.record.double.r1.n3.polar")
    FISICS_FRAME("frame.world")
    FISICS_FRAME_TRANSFORM("transform.member_a_to_world.force");
```

The actual argument must already have the source frame, kind and representation,
and any parameter-required physical references. Unknown facts remain blocked even
with both trust permissions. A successful call changes only the coordinate frame;
application points, displacement endpoints and torque moment points survive,
including optional references supplied by the actual argument. Re-expression of
torque never rebases its moment point. Referenced torque is qualified only when
that point was supplied. Ordinary geometric calls still cannot relabel frames,
axes or required physical point references.

Reverse and identity directions require separate explicit model entries. Nested
explicit calls are checked individually; no inverse or path is inferred.

## Trust and failure boundary

The result retains declared implementation/basis trust and named transform
operation provenance. Static input/output compatibility is checked; numerical
geometry, right-handed orthonormality, observer/state freshness and the body or
successful runtime path are not proved. Strict native coverage requires declared-
boundary permission, nonzero checked work and no unresolved obligations. It does
not waive unknown required facets. Exact [v6 CLI receipts and cross-TU checks](physical_coverage.md#version-6-frame-evidence)
are now supported; legacy v1–v5 receipts are unchanged.

The bounded copied structural pilot uses eight fixed wrappers, force/displacement/
torque in both directions plus two point maps, for one named member/state and a private immutable
basis. Each uses literal direction, local exact-sized arrays, checks the copied
utility's status and exits without returning a typed value on failure. This is
separately tested implementation trust, not a compiler proof that an arbitrary
annotated body calls a validator or exits. A broken wrapper returning typed zero
on failure fails the runtime oracle. Raw status/out-pointer APIs gain no success-
flow or initialization-history inference.

Axes in the selected utility are stored as local-axis/world-component rows;
local-to-world uses the transpose. Independent labelled quarter-turn answers,
identity controls, round-trip, norm/dot invariance and cross covariance qualify
this specimen. Round-trip alone cannot detect a mutually wrong inverse pair.
Reflections and nonorthonormal/nonfinite bases are rejected at runtime.

## Qualification and limits

```sh
make physical-rotation-contract-test
```

The static gate checks 42 source/native cases and 85 commands, including allocation
failure qualification. `build_physical_policy_probe.py --frame-flow` also supports
scoped sanitizer and owned-ledger runs. These instrument the physical model and
semantic modules, not the complete compiler or parser/JSON/unit allocators.

For the optional copied owner pilot, select a fresh output root and matching
compiler/native probe, then run:

```sh
python3 tests/integration/run_physical_rotation_pilot.py \
  --compiler /path/to/fisics --probe /path/to/physical_frame_flow_contract \
  --structural-root /path/to/core_structural --out /fresh/pilot-output
```

Owner source pins are checked before and after copying; owner files are unchanged.
The raw array/status wrapper bodies compile as ordinary C. A separate annotated
caller qualifies the declared source interface. Each compiler compiles its own
caller and wrappers for the bounded numerical run.

The qualified arm64 host now preserves homogeneous float/double aggregates of
one through four members in the native floating-point ABI, including the
three-double records used here. `make aggregate-hfa-contract-test` independently
checks Clang/fisiCs callers/callees in all four combinations, with homogeneous
and larger/nonhomogeneous controls. The copied pilot also checks both mixed
calling directions. This fixes the earlier retained three-double record ABI
mismatch; it is not a claim of complete support for every target or aggregate ABI.

[Point translation/arithmetic and same-point wrench pilots](physical_points.md),
exact v6 receipts/cross-TU checks and bounded ABI/cost qualification are supported.
Broader solver adoption remains separate. General reflection/moving-observer dynamics, dynamic frame instances,
moment rebasing and tensor transforms are outside this family. PhysicalFact stays
64 bytes and an interned context 36 bytes on the qualified host.

## V7 Cosmic kinds

The finite [Cosmic spatial extension](physical_cosmic_spatial_contracts.md) adds
velocity, acceleration, angular rate and specific/full orbital angular momentum
to exact 3D double directed transforms. It retains the existing signature,
precision/parity, context, affine-origin and implementation/basis trust checks.
No dynamic-frame or quaternion dynamics inference is added.
