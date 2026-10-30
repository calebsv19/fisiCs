# Immutable local physical storage snapshots

The optional `physics-semantics` overlay can retain individual physical facts
through a finite local snapshot using ordinary C syntax. The qualified path uses
an explicit v7 physical policy; it introduces no attribute or runtime metadata.

```c
#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define T FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
E double select_energy(double energy E, double moment T) {
    const double snapshot[2] = {energy, moment};
    return snapshot[0];
}
```

The compiler checks the resolved declaration identity, array shape, exact index
and selected initializer expression. Reading slot 1 into this energy result is a
same-dimension wrong-kind error. Names such as `energy` or `snapshot` have no role
in selecting the rule. A storage aggregate/reference is a structural relationship,
not a homogeneous quantity or a quantity-valued pointer.

The finite contract supports local non-static, non-volatile `const double`
arrays of rank one or two, at most 64 elements, with explicit positive fixed
extents and complete nested positional initializers. Reads use unsuffixed decimal
literal indices in range. Copies are explicit element expressions in another
qualified initializer; general array/memory copies are not inferred. Every use
of the selected array in the function must be an exact scalar access. An escape,
address-taking, partial-row use, write/cast route or unsupported index invalidates
the root for this proof, including when it occurs after an earlier read. Distinct
shadowed objects use their captured C identities.

Every initializer leaf needs its own physical provenance. An authored scalar
contract is an input assumption, not validation of the physical producer.
Declared call-result trust remains visible and requires the existing explicit
permission. A wholly trusted function still fails the existing nonvacuous checked
obligation gate. Missing metadata stays unknown even with both trust permissions.
Literal array entries do not acquire neighboring quantity meanings.

Concrete unit scales are not carried through this mixed-storage mechanism.
If any expression in the function has a resolved concrete unit, this finite
snapshot proof is refused, even for equal units. This conservative restriction
prevents a known gram/kilogram distinction from being erased by raw storage.
Keep such boundaries on the existing explicitly checked units path.

The required coverage denominator is unchanged. Qualified aggregate construction
and initialization rows remain required and checked as structural relationships;
they do not acquire a fabricated quantity kind. Scalar reads preserve the
selected initializer origin, trust and first loss. Validated physical kinds carry
their policy-checked dimensions into supported finite scalar operations. This is
a physical-overlay rule; units-only raw mixed storage does not gain this proof.

The rule refuses dynamic indices even under bounds/modulo guards, VLA or inferred
extents, sparse/designated/flattened aggregate initializers, rank three, volatile,
static/global/pointer storage and unknown alias/callback routes. Label/goto,
switch and unsupported statement forms are not admitted by this finite pass.
Functions beyond its analysis limit remain unproved. Unused raw storage is not
promoted merely because another array was checked. No general mutable write
analysis or interprocedural alias analysis is provided.

For a mixed generalized inertia matrix, each TT/TR/RT/RR entry needs its own
explicit physical input role. A constant block selection can preserve that role;
a typed destination checks a row/column swap. This does not prove the original
matrix assembly, physical basis, model admission, symmetry, definiteness,
conservation, calibration or solver stability. Dynamic raw matrix dispatchers
retain their existing refusal boundary. Wrapping raw coefficients in annotated
accessors does not satisfy this contract.

Run the owning stable gate from a source build:

```sh
python3 -B tests/integration/run_physical_storage_snapshots.py \
  --compiler ./fisics --out /tmp/fisics-storage-snapshots-check
```

The output directory must be fresh. The gate checks valid, contradictory,
unknown and explicit-trust paths plus ordinary C execution with overlays off/on
and Clang. Numerical solver and actual-source qualification are separate.

Multi-digit integer tokens beginning with zero are refused by this finite proof: C interprets them as octal. Decimal provenance must not select a different slot from the runtime C index.
