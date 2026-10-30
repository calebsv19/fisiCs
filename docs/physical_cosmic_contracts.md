# Finite Cosmic scalar contracts

The optional v7 physical-semantics model admits explicit scalar primitive tuples
for gravitational constant/parameter and pair strength, mass, squared speed,
specific energy, force/acceleration, momentum, orbital angular momentum, inertia,
angular rate, area, separation length and inverse length. The canonical kind and
Dim8 vector and each linear/difference role must match. Only authored ordered
rules are enabled; multiplication's reverse order needs its own policy entry.
No arbitrary same-dimension tuple, absolute-point product or implicit ratio is
admitted. Existing v1–v6 models and old v7 policy fingerprints are preserved.

The reusable fixture is `tests/fixtures/physical_semantics/cosmic/`: catalog,
v7 policy, five portable scalar prototype kernels and the primitive row inventory.
`COSMIC_PHYSICAL` enables dimensions and quantity kinds in these prototypes;
`COSMIC_UNITS` enables dimensions; ordinary C stores the same doubles.
The prototypes cover 2D kinetic energy, gravitational parameter, pair force,
force-to-acceleration and unsoftened pair potential, using the admitted-state
arithmetic grouping. They do not replace the actual solver's admission or domain
guards, softened/cutoff law, callback ownership, state storage or persistence.

Shape, frame and energy form are separate contracts. A scalar product row does
not prove a vector norm/cross product, rank, handedness, coordinate transform,
physical process or closed-system conservation. Specific energy is distinct from
squared speed despite equal dimensions. Torque is distinct from energy, and
orbital angular momentum is distinct from torque. The prototype frame contexts
are authored test premises, not runtime frame discovery. No automatic kinetic→
gravitational or loss→thermal transfer is defined.

[Finite scalar norm/atan2 contracts](physical_cosmic_math_contracts.md) are now
qualified with a separate v7 opt-in policy. This original prototype policy still
has no sqrt rule and must refuse its unsupported sqrt control. Trigonometric and
framed geometric helpers remain separate; a destination never supplies a missing
kind. Ordinary units now also capture exact atan2 identity and like-input checks.

Run `make physical-cosmic-contract-test` with the normal isolated BIN/BUILD_DIR/
LIB_FRONTEND overrides. Successful Make-owned scratch is removed; a failed root
is reported for diagnosis. For retained evidence, run
`python3 -B tests/integration/run_physical_cosmic_contracts.py --compiler <binary>
--probe <matching-build/tests/physical_operations_contract> --out <fresh-root>`.
Add `--old-compiler <prior-qualified-binary>` to prove that older compilers refuse
the newly declared tuples. The gate checks every ordered scalar rule, strict
prototypes, mixed/unknown frames, wrong quantity/form/origin, raw/cast failures,
model duplicates/unknown references/canonical dimensions, model allocation
failures and four numerical/layout configurations. This is compiler prerequisite
qualification, not actual whole-solver semantic adoption or physical acceptance.
