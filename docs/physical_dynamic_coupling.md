# Bounded cross-block dynamic energy checking

The opt-in v7 physical policy can admit scalar generalized-inertia coupling
contracts for the existing planar dynamic energy helpers. This adds no runtime
type, numerical law, ABI, schema version or default compiler selection.

Two explicit linear kinds have kg·m dimensions:

- `quantity.inertia_translation_rotation`: the translation-row/rotation-column
  coefficient in an authored generalized inertia contraction.
- `quantity.inertia_rotation_translation`: the reverse row/column role.

These roles remain distinct at assignments and function interfaces even when an
admitted symmetric model stores equal numerical coefficients. A kind annotation
is an authored role, not proof that a raw array element has that role.

An explicit rule may multiply either coefficient kind by velocity to produce
`quantity.rotational_momentum`, or by angular rate to produce `quantity.momentum`.
Both scalar operand orders are supported. The existing admitted final contractions
produce generic energy. Each involved catalog dimension and linear reference role
must match exactly. Only named finite tuples under v7 are admitted; no arbitrary
same-dimension kind, model law, spatial frame or energy form is inferred. A policy
must list each needed operation. Older compilers refuse the new tuples.

## Actual-source candidate gate

```sh
python3 -B tests/integration/run_dynamic_coupling_contracts.py \
  --compiler /path/to/new/fisics --baseline /path/to/old/fisics \
  --core-root /path/to/shared/core/core_structural --out /tmp/coupling-contracts-new
```

Run from the matching compiler checkout with a new output directory. The baseline
argument is optional and demonstrates refusal of the new policy. The runner reads
the owner without modifying it. It copies the actual integration TU byte-for-byte,
changes only four private cross-block helper declarations/bodies in a contained
header candidate, and constructs an explicit policy/catalog from current owner
inputs. Half and quarter become named typed constants; their existing arithmetic
order is preserved. Unexpected source shape refuses rather than applying a broad
source rewrite. The copied header's older dimensional-only comment describes the
owner baseline; the four changed bodies in this candidate have physical checking.

Four bodies pass physical strict checking: kinetic TR/RT and midpoint damping
TR/RT. They establish24 checked obligations and four separately permitted literal
assumptions. The owner baseline has24 physical blocked obligations in those four
scopes and24 checked dimensional obligations. The candidate adds four literal
initializer rows; comparing raw percentages would therefore be misleading.

The raw `cs_de_kinetic`, `cs_de_damping` and whole `integrate` scopes remain strict
refusals. The tests also reject swapped coefficient roles, angular-rate versus
decay-rate confusion, untyped values, dimension-only restoration, raw aliases,
casts used to retag values, wrong catalog dimensions/IDs and missing operations.
Trust flags cannot waive those unknowns or contradictions. Body acceptance does
not prove caller selection, matrix construction, symmetry or positive definiteness.

## Independent runtime and ABI gate

First build the native core in fresh output, preserving its existing defaults:

```sh
make -C /path/to/shared/core/core_structural \
  BUILD_DIR=/tmp/coupling-native-core-new build
python3 -B tests/integration/run_dynamic_coupling_runtime.py \
  --compiler /path/to/new/fisics \
  --core-root /path/to/shared/core/core_structural \
  --core-build /tmp/coupling-native-core-new \
  --structural-root /path/to/structural-source \
  --contracts /tmp/coupling-contracts-new --out /tmp/coupling-runtime-new
```

The core build, contracts and sources must belong to the same current input set.
The runtime runner verifies compatibility with the captured contract inputs and
requires the expected native objects. The independent fixture expands a nonzero
cross-coupled SPD3x3 quadratic directly, rather than reusing the helper's nested
sum. It checks729 velocity states, including signs/zeros, exact kinetic/damping
values and nonnegative totals:2,916 checks per combination. Four helper modes and
two caller compilers give eight exact combinations. Three cross-only arithmetic
mutants must fail the oracle. Raw restoration in the thin ABI wrapper remains
unproved. It builds actual selected integration archives/probes; full owner-native
workflows are separate acceptance gates described in
[dynamic energy contracts](physical_dynamic_energy_contracts.md) and
[solver qualification](solver_source_qualification.md).

## Adoption and remaining gap

This is a compiler-side source candidate, not a maintained shared-source change.
The existing owner retains its header/policy. Live adoption requires its coordinated
writer boundary and new qualification. Arbitrary matrix/DOF provenance, alias and
control-flow inference remain future compiler gaps. Material internal work versus
instantaneous storage, damping law, basis and accepted-state ownership retain their
existing model/runtime boundaries. Energy residual is not dissipation. No new
physics, installed or canonical adoption follows.
