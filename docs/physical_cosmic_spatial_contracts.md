# Finite Cosmic spatial contracts

Policy v7 can explicitly declare a bounded family of 3D binary64 operations on
by-value records with exactly `double x, y, z` fields in that order. The existing
Dim8 core and runtime C storage do not change. Older policies and fingerprints
remain intact; these operations require authored model entries.

## Vector operations

The `shape_helpers` schema retains the existing fields. A `norm` entry has one
operand kind and one operand representation; the existing binary operations have
two. Norms return a scalar with no representation or coordinate frame. Supported
unary tuples are displacement to geometric length, and velocity, acceleration,
angular rate, specific angular momentum, full orbital angular momentum or ratio
to the same scalar kind. All input frames must be known. A destination annotation
cannot restore an absent frame.

| Operation | Ordered inputs | Result |
| --- | --- | --- |
| dot | velocity, velocity | speed_squared scalar |
| dot | ratio direction, ratio direction | ratio scalar |
| cross | displacement, velocity | axial specific_angular_momentum |
| cross | displacement, momentum | axial orbital_angular_momentum |
| cross | velocity, specific_angular_momentum | polar eccentricity_numerator |

`quantity.eccentricity_numerator` has dimensions L³/T² but is distinct from a
gravitational parameter. Kinds, canonical dimensions/roles, exact field order,
precision and parity are validated. These new norm/product contracts do not admit
float, 2D, array/pointer parameters, tensors, arbitrary same-dimension products or
unclassified pseudoscalars. Existing v5/v6 force/displacement work and torque
contracts retain their prior behavior.

Use `FISICS_PHYSICAL_OPERATION` on the resolved exact by-value signature. Calls
check input storage, kind and frame compatibility. Dot/norm results are invariant;
free-vector crosses preserve the common frame and clear point references. They do
not infer an applied force, directed lever arm or moment reference. The earlier
force-moment cross contract still requires its declared point relationships.
Bodies, numeric basis and runtime domains retain declared implementation/basis
trust. A nonzero checked scope plus explicit declared-boundary permission is
required for strict acceptance; trust does not waive missing frames or axes.

## Fixed directed transforms

V7 extends the existing 3D double transform family to velocity, acceleration,
angular rate, specific angular momentum and orbital angular momentum. Polar/axial
parity and canonical dimensions/roles remain exact. Displacement rotations and
absolute point maps retain their existing contracts. Each direction must have an
explicit model entry and `FISICS_FRAME_TRANSFORM` signature. Position maps also
require matching source/target affine origins. Context or scalar casts cannot
relabel a frame, and a reflection is not a qualified proper rotation.

The Cosmic implementation reuses the existing `core_space` Y-up to Z-up proper
transform through a Clang aggregate/status bridge. It maps `(x,y,z)` to `(x,-z,y)`;
embedding a planar `(x,y)` gives `(x,0,y)`, and the old planar angular normal maps
to −Y. This fixed basis/origin convention does not prove dynamic pose, observer
freshness, moving-frame dynamics or quaternion integration.

## Qualification and source adoption

`make physical-cosmic-spatial-contract-test` runs the exact model/source contract
and scalar-model checks, including unary norm receipt round trips and tamper
refusal, wrong frame/axis/parity/kind, scalar laundering, missing implementation
trust, malformed models and transactional allocation failures. It is part of
`make test`. The direct runner accepts `--compiler`, `--probe`, `--out` and an
optional `--old-compiler`. Its fixture is
`tests/fixtures/physical_semantics/cosmic_spatial/`.

The receipt model retains the existing supported schema and deterministic
fingerprints; old readers refuse unsupported new norm models. The source checker
and receipt reader both validate the added framed kinds and their parity.

Actual solver adoption must separately qualify helper implementations, labelled
basis answers, cross covariance, numerical parity, ABI, independent orbital
refinement, transactional failures and exact continuation. Constant world-spin
quaternion kinematics does not imply torque-driven attitude or rotational energy.
Pointer arrays, aliasing, raw publication and general geometric/trigonometric
controllers remain explicit boundaries unless separately qualified. Compile/RSS
cost and whole-program adoption are separate acceptance claims.

The related [population/external-field scalar contract](physical_cosmic_population_contracts.md) adds separate specific-potential and mass-weighted energy identities without extending vector or frame inference.
