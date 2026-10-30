# Units coverage-state contract

The foundational layer is an internal, caller-selected coverage assessment
interface. It adds no public frontend ABI fields or new semantic acceptance rules.
The [reporting layer](units_coverage_reporting.md) now supplies an opt-in selected-
function collector and CLI output. Strict enforcement remains unimplemented.
The foundational implementation is
`src/Extensions/Units/units_coverage_model.{h,c}`.

## Evidence inventory

| Existing table | Evidence retained | Evidence it does not establish |
| --- | --- | --- |
| Declaration annotations | owner plus declarator index, dimension/unit resolution, duplicate counts | validation of the initializer or implementation |
| Expression results | resolved dimension and optional concrete unit by expression node | input origins, complete dependency edges, successful check execution |
| Identifier bindings | declaration annotation plus copied C type | general alias/dataflow provenance |
| Access-type snapshots | actual resolved field/element type, isolated from name collisions | original field declaration AST/source origin |
| Call contracts and callee bindings | effective argument/return contracts, resolved callee when retained | checked implementation or externally observed values |
| Recognized math-call table | trusted operator identity for supported helpers | complete operand/check provenance |
| Diagnostics | emitted contradictions and source locations | absence of unknown paths or completeness of checking |

No coverage projection may infer successful checking merely from annotation
presence, a resolved expression, an empty diagnostic list, or a function name.

## Two independent axes

A fact's origin is **declared**, **inferred**, or **unknown**. Its dimension and
unit each have a presence bit; a zero exponent vector without a presence bit is
unknown, not dimensionless. A declaration may be known even if its producing
expression is unknown. Facts carry their use location and, where available, the
origin node/location. Borrowed nodes, paths and registry references are valid
only until that frontend context is destroyed. No persistent identity is claimed.

A selected compatibility obligation has one assessment:

| Assessment | Meaning |
| --- | --- |
| checked | this assessment compared compatible known dimensions/required units, with available provenance and no declared trust dependency |
| trusted-boundary | compatible known facts depend on a declared interface or explicitly assumed representation; implementation/storage meaning is unproven |
| unchecked | necessary metadata, concrete unit, or provenance is missing; a precise reason is retained |
| contradictory | known dimensions or concrete units conflict, even if a trust dependency exists |

Declared/inferred origins remain on both inputs when an assessment is checked,
trusted or contradictory. The internal assessment performs its own selected
boundary comparison; it is not a record asserting that an earlier compiler pass
ran that comparison. Existing diagnostics remain authoritative for compilation.
A compatible declared fact is not a proof that its numerical producer is correct.

## Projection and conservative limits

`fisics_coverage_declaration` selects the exact declaration/index annotation.
`fisics_coverage_expression` observes real identifier bindings, captured access
types, callable result signatures and local expression-result entries. Neither
function changes compiler tables or emits diagnostics. Invalid or duplicate unit
annotations retain an unresolved-annotation reason; they cannot count as checked.

Direct identifier uses can retain their actual declaration origin. Access type
snapshots retain dimensions but have no original field AST, so their provenance
is explicitly incomplete. Existing expression entries lack dependency edges and
can include declaration-adopted literal dimensions; they remain inferred with an
untracked-provenance reason. This is deliberately conservative, including for
simple expressions. P3.2 must retain real origins/check edges before promoting
such observations into complete coverage claims.

A callable result signature is a declared interface dependency. It remains trusted
even when a local body exists; the current tables do not prove that body's check
chain. A result derived from that call does not silently become fully checked:
the local expression projection still reports incomplete provenance.

Assumed representations, such as dense stiffness restoration, require the caller
to explicitly mark a fact trusted with `ASSUMED_REPRESENTATION`. The model never
recognizes trust from helper spelling, raw matrix indices or numerical values.
This flag represents an assertion by the caller, not a new source attribute or
proof of an implementation. Trusted rows are reported separately from checked.

Reasons distinguish overlay disabled, missing annotation, unannotated storage,
uncontracted call, unresolved annotation, absent local inference, incomplete
provenance, declared interface, assumed representation, dimension mismatch,
concrete-unit mismatch and a requested but missing concrete unit. The loss node
identifies the observed unresolved site; it is not a fabricated upstream origin.
Expected and actual fact snapshots remain attached to every assessment.

## Finite denominator

The P3.1 denominator is the number of **unique explicitly selected compatibility
obligations** in one caller-provided batch. A key is `(AST site, boundary kind,
slot)`. Supported kinds are initializer, assignment, call argument and return;
slots distinguish multiple declarators/arguments at one site. Summary rejects
duplicate keys, absent sites and invalid kinds atomically. It recomputes verdicts
from obligations rather than trusting caller-edited assessment counters.

`total = checked + trusted + unchecked + contradictory`. These are counts, not a
whole-function/TU percentage. Empty selection means no observations, not 100%.
Known concrete-unit conflicts are contradictory even in dimension-focused checks;
if concrete units are explicitly required, absence on either side is unchecked.
Otherwise known dimensions can be compared without claiming concrete-unit coverage.

This denominator does not count declarations, resolved expressions, compiler test
cases, source lines, mathematical validity, runtime conservation, or whole-solver
physical realism. It covers selected boundaries only, not every arithmetic step.
A later collector must enumerate all intended obligations from the selected AST
scope, including unresolved ones; enumerating only the successful result table
would hide the very gaps the report must expose. Scope identity, enumeration
completeness, arithmetic/check-edge coverage and any percentage presentation must
be specified and qualified before making a larger coverage claim.

## Qualification and unchanged boundaries

Run `make units-coverage-contract-test` for the internal contract. It compiles real
frontend inputs without executing their deliberately uninitialized raw state,
checks provenance/assessment/denominator controls, and inspects existing raw and
typed translation fixtures plus the heterogeneous structural matrix fixture.
The full `make frontend-contract-test` also owns this test.

The original `translation_raw_field` and `structure_matrix_slot` gaps remain
unchecked. Typed fields expose known dimensions with the current origin limit;
annotating a destination cannot prove a raw source expression. Existing strict
solver qualification continues to fail for its two known gaps. No public units
metadata schema, plain-C behavior, runtime representation, diagnostic policy,
physical quantity kind or downstream solver source changes in P3.1.


## Reporting integration

[Selected-function reports](units_coverage_reporting.md) now collect explicit AST
obligations, reconstruct supported local dependency/check evidence and publish a
separately versioned artifact. The P3.1 observation helpers remain conservative;
the collector retains stronger evidence where actual declaration origins and
supported operand rules are available. The internal boundary enum also includes
`OPERATION`; the caller-selected P3.1 compatibility contract is otherwise intact.
Strict enforcement is not enabled by reporting alone. The separate opt-in
[strict policy](units_coverage_strict.md) defines required rows, controls and trust.
