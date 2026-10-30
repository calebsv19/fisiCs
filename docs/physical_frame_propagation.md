# Static frame and axis propagation (P6.3.3)

Policy v6 now propagates and checks the [canonical frame model](physical_frames.md)
through admitted physical carriers. Dim8 remains authoritative; ordinary C layout
and runtime values are unchanged. V1–v5 retain their established behavior.
This is compile-time compatibility checking of authored frame/observer/state
premises. Numeric basis validity, pose freshness and physical geometry remain
unassessed.

## Supported flow

The bounded carrier subset is the existing 2D/3D float/double rank-1 record and
fixed-array storage, including polar force/displacement and axial torque. Exact
C storage/precision/parity validation remains in force. Unframed rank-2
layout/copy keeps its prior behavior; no tensor transform inference is added.

Frame and axis facts survive supported copies, initialization, component
reads/writes, scalar casts/signs, same-frame scalar operations, conditional
merges and direct declared function boundaries. Ordered validated record fields
map to axes; fixed array components require literal integer indices within the
extent. A dynamic index has unknown axis provenance, so a later required axis
boundary refuses it. Aliased/punned/raw pointer storage remains unsupported.

An annotation on the destination cannot supply missing source context. Framed
scalar components cannot pass through an unannotated or kind-only scalar slot.
The compiler reports that erasure at the boundary. Rebuilding a vector from
such scalars does not restore their former frames or axes.

A plain unframed scalar magnitude or ordinary C loop/control remains possible.
It does not become a vector component merely because its dimension/kind matches.
V6 rank-1 carrier copy with missing frame metadata is incomplete, including when
both sides lost their annotations. AST coverage obligations remain present when
metadata is removed. Missing frames also block enumerated geometric call-result
obligations when the result is discarded. A bare identifier statement creates no
P3 physical obligation; that existing enumeration boundary remains unchanged.

## Operations and references

- Copy, argument, return and initialization boundaries require matching frames,
  component axes and any references required by the destination.
- Component addition/subtraction/comparison requires the same known frame/axis.
  Wrong known identities are contradictory; missing facts remain unknown.
- A finite ratio rule preserves frame and axis, including either multiplication
  order and vector-component divided by ratio. Scaling clears directed displacement
  endpoints; this checkpoint does not prove identity scaling. Arbitrary products
  of framed components cannot silently become a dot or cross product.
- Force addition preserves a common application point. Different application
  points produce a free resultant with unknown application point; point-qualified
  destination metadata cannot invent one. Torque sums require compatible supplied
  moment references. General displacement sums clear fixed endpoint relationships.
- Resolved `physical_operation` helpers retain the P6.2 finite add/subtract/scale/
  dot/cross model. V6 checks each declared input contract and derives the output
  context from the actual inputs before checking the declared output.
- Dot requires a common frame and produces invariant scalar energy, retaining
  input compatibility evidence in the helper event. The result has no coordinate
  frame requirement. Cross preserves the frame and axial output; a referenced
  moment requires displacement `between_points(P,A)` and force `applied_at(A)`,
  yielding `moment_reference(P)`. Missing or mismatched relationships refuse.

Validated generic carrier components already carry the enclosing quantity's
Dim8 contract even when individual C fields have no annotations. V6 finite ratio
and recognized standard-helper rules may consume that proven component contract.
The policy loader has already validated their operand/result dimensions. Field
spelling alone never creates a dimension or physical fact, and the units engine
and units export schema are unchanged. `fabs`/`fmax` preserve/check component
contexts through their existing trusted standard-function identity path.

## Function and trust boundaries

Admitted direct by-value C quantity contracts now support same-frame parameter
and return checks. A declared call result retains implementation trust; source
checking does not authenticate or prove the callee body. Shape helper results
also retain implementation/basis trust even when frame compatibility is checked.
Literal-only construction can adopt the destination's authored frame under the
existing literal-assumption permission. That is authored representation trust,
not inferred geometry.

Ordinary geometric function declarations cannot relabel a frame, scalar axis or
applicable same-kind point reference, even under declared-boundary permission.
Missing required frame/axis/reference facts and contradictory facts cannot be
waived by either literal or declared-boundary permission. The v6 coverage collector
no longer substitutes the broad legacy interface-trust shortcut for physical
facts. Supported known non-trust work is still required for acceptance.

Context erasure or missing required context uses diagnostic 4319 and unknown
facts; known incompatible frames/axes/references use 4317 and contradictory facts.
`--dump-sema` exposes per-fact frame, axis, checked/unassessed compatibility,
invariance and supplied point references. An interned context occupies 36 bytes
on the qualified host; PhysicalFact remains 64 bytes. State-owned contexts retain
the 65,536-entry bound and existing allocation-failure seam.

## Staged boundaries

The internal native coverage API can qualify supported same-frame scopes. Exact [v6 source receipts and cross-TU checks](physical_coverage.md#version-6-frame-evidence)
now expose checked compatibility separately from authored premises. V1–v5 receipts and strict paths remain available.

Explicit [directed rotation calls](physical_rotations.md) now have a separate
bounded gate with retained implementation/basis trust. [Bounded point records, maps and affine arithmetic](physical_points.md) are
qualified separately; a scalar position calculation alone is not record proof. No
moving-observer effects, moment rebasing, generic dynamic frame parameters,
reflections, arbitrary tensor transformations, pointer/callback admission or
solver adoption is added.

Existing unclassified expression/control obligations remain conservative. A
plain ordinary call argument can leave a selected strict scope blocked even
when its physical result context propagates. Comma-return lowering is outside
this qualified slice; an existing frontend reproducer is retained in private
checkpoint evidence. No general control-flow, aliasing, initialization-history,
helper-success-flow or runtime physics proof is claimed.

## Permanent qualification

```sh
make physical-frame-flow-contract-test
make physical-frame-contract-test
```

The propagation gate checks 78 source/native-coverage cases and 162 commands,
including positive copies/construction, wrong frame/member/axis refusals, mixed
components, missing metadata, scalar erasure/reconstruction, ratio and shape
rules, reference preservation/loss, ordinary scalar/control behavior, standard
math helpers, all four trust-permission combinations on unknown required facts,
unchanged AST obligation counts after frame removal, callback/raw
refusals, a declared directed rotation and successful v6 receipt publication. Its native probe sweeps
state-owned allocation failures and requires PhysicalFact at most 64 bytes.

`build_physical_policy_probe.py --frame-flow` supports scoped ASan/UBSan and
owned-allocation-ledger builds against the selected frontend archive. Those
checks instrument the new model/semantic modules; they are not a whole-compiler
sanitizer or global parser/JSON/unit-registry allocation-injection claim.
