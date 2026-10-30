# Selected-function strict units policy

`--units-coverage-strict` is an opt-in frontend gate using the existing
[selected-function coverage report](units_coverage_reporting.md). It requires the
physics-units overlay, one exact source-defined function in one C translation unit,
and at least one JSON/text report path. It does not generate or execute code.
Use it as a prerequisite to a normal build:

```sh
./fisics --overlay=physics-units --units-coverage-strict \
  --units-coverage-function travel \
  --emit-units-coverage-json /tmp/travel-strict.json \
  --emit-units-coverage-text /tmp/travel-strict.txt \
  tests/fixtures/solver_physics/coverage_reporting.c
```

Without the strict flag the report remains advisory. Ordinary compilation and
existing dimensional diagnostics retain their prior behavior. This policy does
not make a whole program or a callee implementation strictly checked by implication.

## Required obligations

Policy `selected_function_v1` requires every collected initializer, assignment,
argument, value-return, arithmetic/merge/call-result operation and unsupported
sentinel in the selected function, except the narrow loop controls below. Both
branches, including unreachable source branches, remain selected. Selection is
syntactic, not an estimate of runtime reachability or physics relevance by name.
Removing metadata cannot shrink the report denominator or automatically waive a row.

Acceptance requires all of the following:

- Successful frontend analysis with no translation-unit semantic errors, even if
  the error is outside the selected function.
- Complete enumeration, successful report publication and at least one required
  obligation assessed as checked. Empty, controls-only and trust-only scopes fail.
- Every required obligation is checked or belongs to an explicitly permitted
  trusted category with complete expected/actual declaration provenance.
- No required unchecked physical obligation or contradictory row. Eligible C interfaces
  are separately classified as described below. Permission never overrides a known
  dimension/unit mismatch, missing input metadata or an unsupported operation.

The verdict is calculated from the in-memory obligations, reassessing their facts;
it is not imported from report files or inferred from annotation counts.
Known concrete units must agree. Dimension-only contracts remain valid; this
policy does not add a requirement that every dimension also name a concrete unit.

## Ordinary scalar controls

A narrow ordinary-control exception covers a single local unannotated primitive
`int` counter declared in a `for` initializer, with no pointer/array/volatile or
static/extern storage. Its initializer must consist of recognized integer
literal/counter expressions. Header comparisons and addition/subtraction may use
that exact declaration and integer literals; `++`/`--` updates are recognized.
Symbol identity, rather than the spelling `i` or a naming convention, ties uses
to the counter. Physical or unresolved annotations prevent the exception.

Only matching header rows are excluded. All remain visible with their original
unchecked/checked states; none becomes checked physical coverage. The loop body
is required as usual. For example, `for(int i=0;i<2;i++) { force=force; }` can pass,
while `force=force*i` remains unknown and fails. A raw parameter used as a loop
bound, an unrelated scalar calculation in the body, other update forms or an
unrecognized counter type remain required and may fail. There is no blanket
integer or floating-point exemption. This initial policy intentionally rejects
unclassified controls rather than guessing that unknown values are dimensionless.

An actual dimensionless physical coefficient can declare `FISICS_DIM(1)`.
Counters do not need to be mislabeled as physical quantities to use the supported
ordinary-control exception. Broader control recognition needs its own bounded
proof before extending this policy.

## Trusted boundaries

Trust is rejected by default. Two independent options permit known categories
within the selected scope:

| Option | Permitted category | What it does not establish |
| --- | --- | --- |
| `--units-coverage-allow-declared-boundaries` | Declared function-result interfaces, including trust propagated through supported operations | That the called implementation obeys its declared physical contract |
| `--units-coverage-allow-literal-assumptions` | Literal-only initialization or plain assignment adopting an explicitly annotated representation | Measured or independently inferred units of a numeric constant |

These are scope-wide category permissions, not per-call approvals, and must be
reviewed using the listed rows. Both require `--units-coverage-strict`; duplicate
flags and permissions without strict mode are errors. Uncontracted physical values, raw
storage restoration, incomplete origins, unsupported operations and contradictions
are not permitted by either option. The narrow nonquantity C-interface role below
requires its own eligibility proof and the declared-boundary permission. Trusted counts remain separate from checked
counts, even when permitted. Reporting a caller does not validate a callee body.

## Report and exit contract

Schema `fisiCs.units_coverage`, version 1, keeps the original observation denominator
and row states. Strict mode adds:

- `strict_enforcement:true`.
- `strict_policy`: contract name, the two permission booleans, `accepted`, `required`,
  `ordinary_controls`, `checked`, `permitted_trust` and `blocked` counts.
- Each obligation's `strict_required` boolean. False identifies a proven ordinary
  loop-header control. Text also labels every row required or ordinary control.

`required + ordinary_controls == denominator.total` and
`checked + permitted_trust + blocked == required`. Global failures such as
incomplete enumeration, no checked work or translation-unit errors can reject
acceptance even when blocked rows are zero. The original denominator does not
turn permitted trust or controls into checked physics.

Successful publication with rejected strict coverage still writes the diagnostic
reports and exits 1. The stderr summary identifies acceptance and counts; the text
report supplies row source/loss locations, expected/actual facts and origins.
Parser, invalid-selection and publication failures retain the existing preservation
rules. JSON/text publication is individually atomic, not a two-file transaction;
consume artifacts only after reconciling the producing command's exit.

Run `make integration-units-coverage-strict-contracts` for positive/negative strict
controls, and `make integration-units-coverage-contracts` for unchanged advisory
reporting. The tests cover stripped metadata, unknown scales, required mismatches,
ordinary controls entering physical arithmetic, distinct trust permissions, empty
and trust-only scopes, TU errors, incomplete matrix enumeration, the raw translation
storage gap, deterministic output, write failures and Clang source legality.
This establishes compiler/source-test behavior, not complete solver qualification.

## Joined acceptance qualification

`make integration-units-coverage-acceptance` runs the fixed
[acceptance matrix](../tests/fixtures/solver_physics/coverage_acceptance.json),
then the advisory reporting and strict-policy suites. The stable owning runner is
[run_units_coverage_acceptance.py](../tests/integration/run_units_coverage_acceptance.py).
Its 19 matrix cases qualify:

- A complete typed stored-state update: 11 required checked observations and three
  separately classified loop controls, with zero trust.
- Eight independent removals of timestep, coefficient, stored position/velocity/
  acceleration, local delta/next-value, and return metadata. Each retains the
  observation/control denominator and fails strict acceptance with visible loss.
- Opaque function results, unsupported size expressions and aggregate boundaries.
  Explicit trust permissions cannot waive these gaps.
- Known concrete-unit mismatch, alongside its valid unit control.
- Full source-shaped structural stiffness construction and application functions,
  using the existing typed blocks. They pass with explicit literal-representation
  permission and fail without it. Construction has 54 required observations
  (52 checked, two permitted trust) and 15 controls; application has 12 required
  observations (10 checked, two permitted trust) and 18 controls.

The joined prerequisite suites also qualify default versus permitted trust,
nonvacuous scope acceptance, physical versus ordinary scalar uses, original raw
storage/matrix gap refusals, report determinism and protected output behavior.
These are finite selected-function compiler acceptance results. They do not
establish arbitrary-C coverage, callee-body correctness, runtime physical truth,
automatic whole-TU enforcement, or application-wide solver adoption. The raw
translation fixture still needs a typed timestep interface in addition to typed
state; the original heterogeneous raw matrix remains unproved. Typed alternatives
must be qualified at their own boundaries rather than relabeling raw slots.

## Real-C controls and source-defined carrier interfaces

Recognized pointer truth tests and their logical negation/conjunction/disjunction
in `if` conditions, plus literal 0/1 returns from a
resolved C boolean function are ordinary C controls. Boolean typedefs are resolved
from the actual AST declarations; no type is guessed from its spelling. Physical
comparisons inside a logical predicate remain required observations. Bare physical
truth tests or a physical quantity returned as status are not exempted. These rows
retain their observation states and carry strict_basis `c_predicate` or
`c_boolean_status`; they are excluded from required physics and do not inflate
checked counts. The checked/nonempty acceptance requirement still applies.

A direct call to a source-defined, nonvariadic void function may be classified as a
`declared_c_interface` when every argument is a single-level pointer to the same
resolved physical record as its parameter. A physical record must actually contain
annotated fields. Scalar pointers, unresolved records, variadic calls, externally
only-declared functions and physical return values are not eligible.

This classification does not assign a dimension to a record pointer or void value.
The original quantity observation may remain unchecked because no whole-record
quantity contract exists. The strict role is an explicitly trusted C interface,
permitted only with `--units-coverage-allow-declared-boundaries`. These rows remain
required and count as permitted trust, never checked physics. Known contradictory
facts override the role. The strict-only `interface_callee` names the direct callee.
A caller report does not qualify that implementation: an application gate must
review the helper inventory and qualify the relevant callee bodies independently.
Unknown physical arguments/results cannot be admitted through this role.

## Literal writes into declared storage

Plain `=` writes of numeric/character literal-only expressions may adopt the
resolved lvalue dimension/unit, consistently with authored literal initialization.
This covers fields and array elements as well as scalar variables. It does not
apply to identifiers, calls or compound assignments. A dimensionless variable is
still dimensionless; assigning it to length is rejected. Concrete-unit mismatches
on variable values still require explicit conversion.

Strict reports label these writes `trusted_boundary` / `assumed_representation`.
They require `--units-coverage-allow-literal-assumptions`, remain separate from
checked arithmetic, and do not establish measured or independently inferred units
for constants. Dimensioned comparisons/conditional values continue to require
compatible facts; use typed zeros/constants in shared source.

Run `make integration-units-storage-contracts` for the literal-write, predicate,
boolean-status and C-interface valid/invalid cases. The frontend contract suite
also checks that contradictory facts and absent permissions override interface
trust. Existing selected-function acceptance remains a separate gate.

## Actual-source and cost gates

See [actual solver source qualification](solver_source_qualification.md) for the
hash-bound external structural candidate, its finite strict/numerical boundary,
and the separate matched overlay/strict-report cost runner. These gates do not
automatically adopt external source changes or qualify raw matrix provenance.
