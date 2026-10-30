# Finite scalar norms and atan2

Version7 policies may explicitly declare canonical scalar rules for `sqrt`,
`hypot` and `atan2`. They reuse the existing operation schema, canonical kinds,
input-derived provenance and strict coverage. Existing policies retain their
fingerprints and do not acquire new operations implicitly.

Supported tuples are deliberately finite:

| Operation | Input kind(s) | Result kind |
| --- | --- | --- |
| sqrt | area | geometric_length |
| sqrt | speed_squared | velocity |
| sqrt | acceleration_squared | acceleration |
| hypot | displacement, displacement | geometric_length |
| hypot | velocity, velocity | velocity |
| hypot | acceleration, acceleration | acceleration |
| atan2 | geometric_length, geometric_length | angle |

Dimensions and linear/difference roles must match the canonical definitions.
Specific energy cannot replace squared speed. Absolute points, unknown values,
carriers, representations and framed/axis-tagged operands cannot enter these
scalar rules. Qualified geometry remains a separate contract; a scalar norm does
not prove vector shape, frame or a coordinate transform. The current scalar
velocity kind serves components and their speed norm in this bounded model.

Calls use captured standard math-header identity and an exact real floating
prototype. Existing compiler-owned generic wrappers retain their separate
identity checks. User definitions, arbitrary names and function-pointer calls do not
become recognized math operations. Standard-library identity supplies mathematical
interface semantics; it is not a formal proof of the library implementation.
`atan2` requires equal input dimensions and compatible concrete units and yields
a dimensionless angle. Its units strict receipt checks the transformed result.
Missing kinds never recover from a destination annotation.

Direction/clamp and diagnostic scalar rules include geometric length squared,
acceleration squared, displacement/separation and acceleration ratios, mass
weights, inverse length, angle/angle radian normalization, momentum times velocity,
and specific/full angular momentum. Each authored ordered tuple is checked
against the finite canonical table; these component products do not establish
vector geometry, an energy form, a gravity model or a physical process.

`tests/fixtures/physical_semantics/cosmic_planar` is the standalone qualification
policy/catalog. `make physical-cosmic-math-contract-test` is included in `make test`.
It exercises every ordered rule, callee/kind/frame/raw-input refusals, malformed
and duplicate models, canonical receipt round-trip/tamper controls and transactional
allocation failures. The direct script accepts `--old-compiler` for prior-version
refusal and `--out` for retained evidence. Scoped sanitizer qualification uses
`tests/integration/build_physical_policy_probe.py --operations --mode sanitized`;
it does not instrument the entire frontend archive.

Actual application adoption remains a separate source, numerical, ABI and
consumer qualification. Ordinary C and Dim8 storage are unchanged.

## Pair and adaptive encounter scalar model

The standalone `cosmic_encounter` fixture extends `cosmic_planar` without changing
an existing policy or granting operations from dimensions alone. It adds142 total
ordered rules. Additional math tuples are `sqrt(inverse_speed_squared)` to
`inverse_velocity`, `hypot(ratio, ratio)` to `ratio`, and
`atan2(ratio, ratio)` to `angle`. The same exact standard-library identity,
canonical dimension/role and unframed-input requirements apply.

Additional scalar products/quotients describe specific angular momentum divided
by gravitational parameter, inverse velocity times velocity or specific angular
momentum, gravitational parameter divided by specific energy, length divided by
gravitational parameter or speed, and length/speed normalization. Length times
speed produces specific angular momentum; length times inverse velocity produces
duration. Specific angular momentum remains distinct from mass-weighted orbital
angular momentum. Angles remain distinct from dimensionless eccentricity/ratios.

A canonical dimensionless `quantity.kinetic_specific_factor` explicitly interprets
squared speed as kinetic specific energy through the authored one-half coefficient.
A generic ratio does not relabel squared speed as specific energy. The compiler
checks the operation contract; the coefficient's value and the orbital formula
still require independent numerical references. This model has no new runtime
physical type fields, inferred energy forms or frame transforms.

`make physical-cosmic-encounter-contract-test` runs every ordered rule, model and
callee failures, transactional allocation, receipt compatibility and tamper checks.
It is part of `make test` and uses a fresh disposable output root. The reusable
script also accepts `--fixture cosmic_encounter`, `--old-compiler` and `--out`.
Application adoption must separately prove conic applicability/zero carriers,
periods, typed length versus radial-speed event boundaries, adaptive error
normalization, sampled event brackets, partial results and persistence semantics.
