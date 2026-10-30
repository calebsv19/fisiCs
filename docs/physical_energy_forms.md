# Energy forms, aggregation and declared transfers

The opt-in `physics-semantics` overlay supports energy qualifiers with a version 7
physical policy. Dimensions remain the existing eight signed exponents; the kind
remains `quantity.energy`. Forms describe contributions to an energy account.
They are independent metadata and introduce no runtime payload or ABI change.

## Source and policy

Use `<fisics/extensions.h>` for source shared with Clang:

```c
#define ENERGY FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")

double energy_total(
    double thermal ENERGY FISICS_ENERGY_FORM("energy.thermal"),
    double kinetic ENERGY FISICS_ENERGY_FORM("energy.kinetic"))
    ENERGY FISICS_ENERGY_FORM("energy.mixed")
{
    return thermal + kinetic;
}

double release_heat(double chemical ENERGY FISICS_ENERGY_FORM("energy.chemical"))
    ENERGY FISICS_ENERGY_FORM("energy.thermal")
    FISICS_ENERGY_TRANSFER("transfer.chemical_to_thermal");
```

Native fixtures can use `[[fisics::energy_form("energy.thermal")]]` and
`[[fisics::energy_transfer("transfer.chemical_to_thermal")]]`. Each attribute
occupies its own bracket pair. Declaration inheritance and per-declarator
isolation apply. The macros erase for Clang; they do not make Clang validate the
physical metadata.

A v7 policy retains the fourteen v6 root fields and requires two additional arrays:

```json
"energy_forms": ["energy.chemical", "energy.kinetic", "energy.mixed", "energy.thermal"],
"energy_transfers": [
  {"id": "transfer.chemical_to_thermal", "source_form": "energy.chemical",
   "target_form": "energy.thermal", "process": "process.combustion"}
]
```

See the complete [example policy](../tests/fixtures/physical_semantics/energy/policy.json).
The catalog must define linear `quantity.energy` with SI energy Dim8. Every
identity must be canonical. The reserved `energy.mixed` identity is mandatory.
There can be at most 128 forms and 256 transfers. Transfer endpoints must be
known, distinct concrete forms; `mixed` cannot be a transfer endpoint. Duplicate
IDs, extra/missing fields and unknown references fail. Canonical sorting makes
fingerprints independent of input order; process and direction participate in
the fingerprint. Failed loads leave the live model unchanged.

## Propagation and boundaries

| Operation | Result |
| --- | --- |
| Same-form addition/subtraction | Same form |
| Different known forms added/subtracted | `energy.mixed` |
| Conditional selection or admitted `fmax` of different known forms | `energy.mixed`, a conservative join rather than a conversion |
| Any energy operand has no form | Result form remains unknown |
| Unary sign or admitted `fabs` | Preserve the form |
| An admitted energy × ratio, ratio × energy or energy ÷ ratio rule | Preserve the energy operand's form |
| Other derived operations | Do not infer an energy form |
| Specific energy assigned to `mixed` or unqualified energy | Safe widening |
| `mixed`, unqualified or different-form energy assigned to a specific form | Refuse narrowing/restoration/relabeling |

The operation rules must still be explicitly admitted by the policy. Addition of
energy contributions does not prove an energy conversion or physical process.
A C cast preserves physical identity. An unqualified local loses its declared
form; a destination annotation cannot recover it. Literal interpretation remains
explicit literal trust. Missing forms can remain usable as general energy; strict
checking of a required specific form cannot be waived by a trust permission.

This checkpoint supports scalar `float` and `double` energy forms. Represented
vectors, array-content forms, integers and arbitrary pointers are outside this
qualifier contract. Energy-form metadata does not extend power, time integration,
or the operation registry's admitted dimensional/quantity rules. Same-form spatial
transfers, component identity, sign conventions and accounting intervals belong
to a separately declared solver interface.

## Directed helpers and trust

An energy transfer selects the actual resolved C function declaration, not its
spelling. It requires exactly one by-value `double` parameter and a `double`
result, with exact source/target forms from the model. It cannot be variadic or
use pointer/callback restoration. Ordinary energy helpers cannot change the form
of an energy input merely through a different return annotation.

A successful call retains `declared-trust`, its operation ID and its source loss
location. Selected strict acceptance needs
`--physical-semantics-allow-declared-boundaries`; the checked input compatibility
is separate from trusted output implementation. The annotation does not exempt
a function body from ordinary form checks. Implementations using raw numeric
adapters must be qualified separately. The compiler does not prove efficiency,
nonnegativity, energy conservation, component existence or correct physical law.

## Evidence and validation

Version 7 physical receipts add `energy_model` (18 exact root fields), a nullable
canonical `energy_form` on every fact, and nullable `energy_transfer` on external
declarations. The model includes named processes, directed endpoints,
`implementation: declared` and `conservation: unassessed`. Expression operation
IDs retain named transfers. Readers validate the current policy/catalog/source
receipts, exact model, supported signatures and shared declaration consistency.
They are unsigned local evidence, not authentication of bodies or protection
against a coordinated forgery. Versions 1–6 retain their established schemas.

```bash
make physical-energy-contract-test
```

The gate checks accepted, rejected and unknown source cases; model/state failure
controls; cross-TU and stale/tampered receipts; exact numerical bookkeeping and all
four Clang/fisiCs calling combinations. A deliberately incorrect transfer body
passes ordinary compilation but is detected by the independent runtime oracle.
Frame coexistence has a separate gate:

```bash
python3 tests/integration/run_physical_frame_completion_contracts.py \
  --compiler /absolute/path/to/fisics --probe /absolute/path/to/physical_frame_flow_contract \
  --energy --out /fresh/output/path
```
