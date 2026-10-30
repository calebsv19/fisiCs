# Finite discrete interaction contracts

The opt-in v7 interaction fixture declares164 ordered scalar rules, including
canonical translational stiffness (M/T²), translational damping (M/T) and
`quantity.dissipative_power` (ML²/T³). These complement existing force,
displacement, duration, energy, inertia and angular-momentum kinds.

The added finite tuples are stiffness × displacement → force, damping × velocity
→ force, force × displacement → energy, force × velocity → dissipative power,
dissipative power × duration → energy and inertia / inertia → ratio. Multiplication
admits both authored operand orders. Canonical names, Dim8 and reference roles
must match. These are scalar component contracts; they imply no vector frame,
normal orientation, general work inference, contact model or system conservation.
Exact fmax entries clip force and dissipative power; explicit ratio scaling retains
the scaled kind. Existing policies and schema versions remain unchanged.

```sh
make physical-cosmic-interaction-contract-test
```

The maintained gate checks every declared scalar rule, metadata/receipt loading,
allocation failure and compatibility. Negative controls refuse implicit storage to
loss relabeling, restoration of an absent energy form, raw function-pointer
laundering and loss to thermal relabeling.

## Storage and process ledgers

The fixture adds `energy.elastic`, `energy.contact_loss`,
`energy.preparation_loss` and `energy.damage_loss`. These are contributions or
accounting categories, not interchangeable physical stores. Elastic storage is
not a loss ledger. Contact integration classifies a dissipative-power integral.
Preparation removal starts as a removed kinetic contribution. Two exact unary
double process signatures explicitly record elastic → damage_loss and kinetic →
preparation_loss. No loss → thermal transfer is declared.

A selected wrapper can require declared implementation/process trust even when
its arithmetic callee is checked separately. The body and conservation of an
energy-transfer interface are not proven by its signature. Qualify its numerical
implementation independently and report that trust separately. Disallowing
boundary trust must reject these wrappers.

The actual Cosmic adoption selects11 arithmetic helpers with41 checked
obligations/zero trust, and four storage/process wrappers with seven checked/eight
explicit implementation/process-trust obligations. No selected literal assumptions
are needed. Underlying raw classification/identity bodies are ordinary C and are
covered by parity, ABI and numerical mutation tests.

## Inertia and angular momentum

An inertia tensor has dimensions ML²; its angular-momentum RHS has ML²/T.
After dividing by the same inertia scale, matrix coefficients are dimensionless
and the RHS has1/T. Keep these in separate typed carriers. Swapping a coefficient
into the RHS must fail units checking. An augmented array cannot truthfully carry
one dimension for all four columns.

The compiler does not infer matrix rank, positive definiteness, pivot choice or
conditioning. Cosmic preserves the existing pivot comparison, threshold, swap,
normalization and elimination order and tests against its frozen augmented
algorithm. Pointer/array solver controllers continue refusing whole-function
physical-strict acceptance. This distinction also applies to swept contact,
relaxation exp, callback dispatch, history and atomic publication.
