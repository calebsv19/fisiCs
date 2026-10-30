# Selected dynamic energy contracts

The opt-in dynamic-energy source gate checks actual private arithmetic used by
the Structural motion owner. It preserves ordinary C storage and the eight
unit exponents. It does not select a default compiler or qualify a whole solver.

```sh
python3 -B tests/integration/run_dynamic_energy_contracts.py \
  --compiler /path/to/fisics \
  --core-root /path/to/shared/core/core_structural \
  --out /tmp/dynamic-energy-contracts-new
```

Run from this compiler checkout so its default extension headers match the tool.
Output must be fresh. The gate pins current source/policy/catalog and runs seven
physical helper bodies, four units-only cross-block helpers, raw-scope refusals,
quantity/model/operation controls, portable C, four runtime configurations and
six dimensionally legal arithmetic mutants.

The explicit v7 dynamic policy reuses current canonical mass, velocity, momentum,
angular rate, moment of inertia, duration and inverse duration. It adds scalar
`quantity.rotational_momentum` (kg m²/s), conjugate to the admitted rotational DOF,
with moment_of_inertia × angular_rate and rotational_momentum × angular_rate
contracts. These finite rules do not infer a vector frame, whole-body spin,
orbital angular momentum or an energy form. Existing policies are unchanged;
older tools reject policies requiring these new tuples.

Physical bodies check translational/rotational kinetic terms, midpoint damping
terms, force/displacement and moment/angle work, and the generic-energy balance.
Explicit half/quarter constants are literal assumptions. Generalized matrix and
DOF restoration remain untyped and must fail whole-function physical strict
checking. Cross-block inertia kg m contracts have dimensional coverage only.
Generic computed energy is not automatically classified kinetic/elastic/heat.
Fixture calls use declared interface permission; helper bodies are checked
separately without that permission.

Actual selected account is `R = I - I0 + K + D - W`. I0 is the accepted initial
internal work; it is not interchangeable with instantaneous elastic storage.
K=.5 vᵀMv, D uses midpoint velocity and passive mass-proportional damping, and
forced W uses trapezoidal total endpoint loads including held load. These are
runtime/model definitions verified independently, not compiler conservation
proof. The residual is not dissipation. Preserve exact physical clock and
accepted-prefix rollback, including when display samples are decimated.

Structural's v2 saved result carries I/I0 at each saved state and an explicit
account definition. Its reader checks finite terms, reference, clock/prefix and
roundoff-scaled reconstruction; v1 records remain readable without being
upgraded or relabelled. A coordinated rehashed rewrite is not authenticated by
these consistency checks; independent numerical oracles remain necessary.

Use the native all-tick probe and independent account observer from the owning
Structural tests with explicit matched original/adopted/base/units/physical
probe paths. Exact comparisons use matched O0 and FP contraction off. Optimized
Clang energy accumulations may differ in low bits; use unchanged independent
reference tolerances and report this separately from exact kinematic parity.
No arbitrary model calibration, nonlinear/plastic/contact or thermal-feedback
acceptance follows. Installed/canonical/release adoption remains separate.

## Cross-block physical checking candidate (Phase9)

The maintained owner baseline above remains unchanged. The optional
[cross-block candidate gate](physical_dynamic_coupling.md) adds explicit coefficient
roles and physical checking for four contained actual-source helper bodies. Raw
DOF/matrix restoration remains refused; no shared-owner adoption is implied.
