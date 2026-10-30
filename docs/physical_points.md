# Bounded point and same-point wrench contracts

Policy v6 extends the existing position/displacement affine pair to by-value
3D double polar `x/y/z` records. A position must declare the physical origin
linked to its frame. Float, 2D, array and general tensor position carriers remain
outside this family. Scalar temperature and other existing affine behavior are
unchanged. See [frame declarations](physical_frames.md) and
[directed rotations](physical_rotations.md).

## Point operations

C has no built-in record addition/subtraction. Explicit `physical_operation`
helpers use the existing shape-helper schema and these finite v6 contracts:

| Operation | Operand kinds | Result |
| --- | --- | --- |
| subtract | position, position | paired displacement |
| add / subtract | position, paired displacement | position |
| add | paired displacement, position | position |

All three helper slots have the same admitted 3D double polar representation.
Concrete parameter/result origins are declared on the C signature. Actual frames
and component axes must match; point subtraction also requires compatible affine
origins. Dimensions and quantity pairing retain their existing checks. A
position-plus-position helper is rejected by the model loader.

Subtracting named points `B-A` derives `between_points(A,B)` only when both
physical point subjects are known. Otherwise endpoints stay unknown. Shifting a
point preserves its frame/origin and clears its fixed subject. Destination
annotations cannot invent the subject or missing endpoints. Force application
points, coordinate origins and moment references remain separate identities.

`map_point` transforms use the same resolved unary contract path as vector
rotations. Their parameter/result declare the directed source/target frames and
corresponding position origins. Mapping updates both frame and affine origin,
while preserving the optional physical point subject. A point map includes
translation; a vector rotation does not. Missing or incompatible facts, raw
storage and scalar laundering cannot restore a point or frame.

## Same-point wrench boundary

The copied structural pilot passes force and axial torque as separate records,
with a declared common physical reference. Both rotate in the same direction and
the torque's moment reference is preserved. The actual utility's heterogeneous
six-slot force/moment buffer is used only for an independent numerical comparison;
it is never annotated as one vector. Missing moments and moment-point relabeling
refuse. Moving the moment point requires a separate lever-arm cross-force law
and remains excluded.

The pilot has eight fixed wrappers: three vector kinds in both directions, plus
two point maps. They bridge through local exact-sized arrays to unchanged pinned
owner utilities, with private constant basis, literal direction and checked
status. Failure exits without returning a typed value. Independent translated-
origin, vector invariance, work/dot, cross covariance, same-point wrench and
atomic-output controls are separate from static compatibility. Wrapper bodies,
numeric basis and observer/state validity remain declared trust.

## Verification

```sh
make physical-frame-completion-contract-test
make physical-rotation-contract-test
make aggregate-hfa-contract-test
```

The completion gate owns point/reference and exact v6 receipt/cross-TU checks.
The copied pilot invocation is documented in [rotations](physical_rotations.md).
No inline structural assembly, mixed DOF/stiffness contraction, generic mutable
frame, moving observer, reflection, moment rebasing, or solver-wide adoption is
established by these tests.
