# Solver physical qualification fixtures

These bounded reductions exercise physical calculations and the C representation
paths used by real solver code. They do not qualify whole applications or prove
model accuracy. Expected values are analytical, with relative tolerance 1e-10
and absolute tolerance 1e-9; non-finite values fail.

| Fixture | Source pattern | Analytical oracle |
| --- | --- | --- |
| translation.c | gravity_orbit_sim, orbit_body_integrate | v=1+3*0.5=2.5; x=0+2.5*0.5=1.25 |
| energy.c | gravity_orbit_sim, orbit_body_refresh_energy | E=0.5*2*(3*3+4*4)=25 |
| structure.c | core_structural_frame_element and normal_extrema recovery | EA/L=120000; F=120; N/A+M/Z=106000 |
| interface.c, helper.c | extracted velocity-times-timestep helper boundary | distance=3*2=6 |

Translation retains pointer-to-body access and component arrays; the source's
separate x/y fields are consolidated into two-component arrays. TYPED_STATE is an
additional annotation experiment, not an assertion that the customer is annotated
that way. The anchored branch and other axes are outside this reduction.
Energy retains its annotated local bridge and concrete units; the original
separate velocity fields are consolidated into an array. The sqrt misuse is an
added math-boundary probe, not a literal excerpt of refresh_energy.
Structure retains the heterogeneous 6x6 matrix, selected coefficients and
force/stress recovery. It reduces the source's full frame assembly and extrema
search to one known coefficient and section point. A single homogeneous dimension
cannot describe every stiffness slot. The helper interface is a reduction of the
selected update's velocity-times-dt operation split into a header and two TUs;
it is not a claim that the customer exports a function named travel.
Source revisions/content hashes and extraction mapping belong to the qualification
report; fixtures remain self-contained for repeatable execution.

From the repository root, select the compiler and a new output directory:

```sh
python3 tests/integration/run_solver_physics_qualification.py \
  --compiler ./fisics --out /tmp/solver-physics-new-run
```

Equivalent Make entry point:

```sh
make integration-solver-physics SOLVER_PHYSICS_OUT=/tmp/solver-physics-new-run
```

The default strict mode returns 1 for any checking gap or execution failure.
For a baseline inventory, add `--baseline` (or set
`SOLVER_PHYSICS_ARGS=--baseline`). Inventory mode returns 0 only when all cases
executed and were classified; `qualification_passed` remains false if gaps exist.
Never interpret that inventory exit code as successful physical qualification.
No snapshot of today's gaps is accepted as a semantic success expectation.

The output includes exact compile/run commands, exit codes, semantic dumps,
fixture/compiler hashes, per-case states, and results.json. Each output root must
be new. Source-order cases run forward/reverse with identical expectations.
Clang validates ordinary C, including mutant legality; it does not validate the
physical overlay. Invalid-case runtime output has no valid-physics oracle.

The runner checks its own validators against wrong exits, wrong numeric output,
missing executables, unrelated diagnostics and a stale executable after rejection.
A compile timeout or unsupported parser failure is an execution failure, never a
physical rejection pass. Retain useful logs and reproducers, then remove owned
executables once no consumer needs them.

## Typed unreleased local frame boundary

`frame_local.h` is a portable compiler fixture, not a production structural API.
It expresses the complete unreleased local Euler-Bernoulli element using four
annotated stiffness blocks and separate coordinates and loads. The original
coordinate order is `[ux_a,uy_a,rz_a,ux_b,uy_b,rz_b]`; translations map to
`[0,1,3,4]` and rotations to `[2,5]`.

| Field | Shape | Dimension |
| --- | --- | --- |
| stiffness.tt | 4x4 | force/length |
| stiffness.tr | 4x2 | force |
| stiffness.rt | 2x4 | force |
| stiffness.rr | 2x2 | force*length |
| coordinates.translation | 4 | length, meter |
| coordinates.rotation | 2 | scalar, interpreted as radians |
| loads.force | 4 | force |
| loads.moment | 2 | force*length |

`frame_local_build` explicitly initializes all 36 coefficients, including typed
zeroes. `frame_local_apply` accumulates each output from homogeneous block loops.
The inputs require finite positive E/A/I/L in an SI numerical basis; the fixture
does not implement model admission or general unit normalization. Product
metadata currently proves dimensions, not a concrete-unit scale for every term.

Run `make integration-units-frame-local-contracts`. The gate checks three section
sets, all 108 coefficients, six basis inputs, three rigid modes, a mixed input
and an axial extension for each section (33 applications / 198 output components).
The C reference retains the original solver's unreleased row-major matrix formula;
the independent Python reference integrates axial/cubic-Hermite curvature
operators using exact rational arithmetic. Clang controls and explicit diagnostics
separate ordinary C legality from physical enforcement. A dimensionally legal
sign mutation must fail numerical parity. Oracle controls reject wrong
coefficients, wrong loads and nonfinite output.

Dynamic indices within a declared homogeneous block are supported. The two
coupling blocks share dimensions, so dimension checking does not prove their
physical roles. Unannotated raw matrices, unknown inputs and arbitrary pointer
flows remain checking boundaries. The original `structure.c` / `BAD_MATRIX_SLOT`
control is retained and still counted as a checking gap. No releases, coordinate
transforms, production dense adapter, global solve, spatial/plate/shell adoption,
physical-kind semantics or runtime performance claim is implied by this fixture.

## Local releases, basis transforms, and dense exchange

`frame_boundary.h` builds on the typed local fixture without changing it. It
supports no release, A-only, B-only and both-end rotational releases; the one-end
case uses typed Schur-complement blocks, and the both-end case retains only axial
stiffness, matching the owning 2D frame source. The input is an admitted local
Euler-Bernoulli matrix with a positive rotational pivot. This is not a generic
singular-matrix solver or a new model-admission policy.

`frame_local_transform` evaluates the block form of `K_global = T^T K_local T`.
Translations rotate through a normalized finite cosine/sine pair; planar rotations
are unchanged. Matching coordinate and load transforms allow covariance checks.
Inputs and outputs of releases, stiffness transforms, coordinate transforms and
load transforms may alias; source snapshots make those operations safe in place.
No physical frame identifiers or normalization checks are inferred by the compiler.

`frame_dense_pack` explicitly maps all blocks to row-major
`[ux_a,uy_a,rz_a,ux_b,uy_b,rz_b]` storage. Its `FrameDense6` elements are deliberately
unannotated. `frame_dense_restore_assumed_si` explicitly assumes that order and SI
representation on import. Restored fields receive declared physical contracts;
this is trusted annotation, not evidence that the compiler checked raw provenance,
slot identity, units or arbitrary bulk/pointer writes. Subsequent known physical
misuses remain checked. These fixture adapters are not installed in the shared
structural solver.

Use `make integration-units-frame-boundary-contracts`. It covers 3 sections x 4
release states x 5 normalized bases, including oblique and negative orientations.
For each compiler, 2,160 coefficient positions are checked independently through
transformed typed values, packed dense values and restoration of a separately
constructed reference (6,480 comparisons). Eleven input vectors per case cover
basis vectors, combined motion, rigid modes and global-x extension. Four
application paths (direct, coordinate/load covariance, restored, in-place) produce
15,840 checked output values. Additional checks preserve inputs, test in-place
stiffness operations, clear released rows and overwrite sentinel destinations.
The independent rational oracle reuses exact Hermite integration, then computes a
simultaneous Schur complement (including the 2x2 both-release block) and exact
basis congruence. C source-shaped parity and Clang execution remain separate
controls. Tolerances are absolute 1e-9 plus relative 1e-10, with load cancellation
scaled by the sum of absolute term magnitudes; nonfinite outputs fail.

The gate requires 21 exact physical rejections and 12 ordinary-C mutation controls.
Seven dimensionally legal mutations must fail numerical parity: wrong pack/restore
slots, transposed pack/restore indices, wrong load-transform sign, wrong release
end and raw-value corruption. An additional 72 exact tagged-value comparisons
prove pack/import permutations on nonsymmetric data, which beam symmetry could
otherwise hide.
The original unannotated matrix gap remains an explicit unchecked control.
No distributed loads, joint springs, recovered member fields, global solve,
factorization, spatial/plate/shell migration or semantic frame-kind checking is
included.

## Typed local loads, joint springs, recovery and work

The next fixture layer is split by role:

- `frame_loading.h`: density/gravity and applied uniform loads, equivalent nodal
  force/moment terms, released-load condensation, load addition and nodal work.
- `frame_joint.h`: coincident translational and rotational elastic springs,
  three-bit active-coordinate masks, paired stiffness, response and strain energy.
- `frame_recovery.h`: eliminated member rotations, end forces/moments, normalized
  axial/transverse displacement polynomials, force/moment/shear polynomials,
  normal-stress extrema, rectangular shear convention, and elastic/distributed work.

The input convention is finite admitted SI data with positive E/A/I/L and section
modulus; spring coefficients are nonnegative. Normalized xi keeps each polynomial
array homogeneous. Scalar loop coefficients and integration powers are explicitly
annotated so arithmetic does not silently lose dimensions through generic helpers.
The fixture keeps independently dimensioned quantities in separate declarations
for readability. The separate `integration-units-declarator-contracts` gate now
qualifies distinct physical suffix annotations within one comma-separated
declaration; it leaves unannotated neighbors unknown.

The source-shaped spring convention is force_on_a = k*(q_b-q_a). The conjugate
external nodal loads Kq have the opposite sign at A. Member end forces are Kq-f_eq.
Release recovery uses the original unreleased operator, not an already condensed
matrix. The section normal-stress envelope evaluates both sides at endpoints and
any stationary point of the quadratic stress polynomial. Shear uses the source's
1.5*max(abs(V))/area convention; no strength or general-section qualification is
implied. Deflection extrema and authored utilization/limit policy are outside this
fixture.

`make integration-units-frame-recovery-contracts` covers 36 load setups, 72 member
states and all 8 joint masks for two displacement patterns (16 joint states). It
checks 4,192 values per compiler: 936 load values, 2,448 member values, 224 joint
response/work values, 576 joint stiffness coefficients, and eight analytical
anchors. A rational oracle builds Hermite interpolation plus uniform-load
particular solutions, solves eliminated rotations, differentiates fields,
integrates strain energy and external work, and evaluates exact stationary stress
extrema. Clang and fisiCs are each checked independently. Absolute tolerance is
1e-8 plus relative 1e-9; nonfinite values fail. Header-only unused static functions
are suppressed in the strict Clang control, while other warnings remain errors.

Work is paired with matching coordinate bases. Arbitrary global nodal work is
reported separately from equilibrium endpoint work. For consistent recovered
member fields, 2U = endpoint_work + integrated_distributed_work; it is not assumed
for arbitrary unequilibrated nodal loads. Springs independently check 2U = nodal
work. The original 106000 tensile-stress oracle and a 1000 N cantilever with
0.1 J elastic energy provide analytical anchors.

The focused gate requires 28 exact physical rejections, 19 ordinary-C mutation
controls and eight dimensionally legal numerical failures. It checks load factors,
spring signs/energy, recovery signs, interior stress extrema and work factors.
Three corrupted-output controls test the rational oracle itself. Existing raw
matrix/state gaps, assumed-SI dense restoration, frame-ID semantics and
same-dimension energy-versus-moment distinctions remain separate boundaries.
No global solve, shared-solver migration, generic root engine, nonlinear behavior,
thermal eigenstrain, spatial/plate/shell adoption or performance claim is included.
