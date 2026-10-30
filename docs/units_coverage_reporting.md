# Selected-function units coverage reports

Coverage reporting is opt-in and requires the physics-units overlay. Select exactly
one function definition in one C translation unit and at least one filesystem
report destination:

```sh
./fisics --overlay=physics-units \
  --units-coverage-function travel \
  --emit-units-coverage-json /tmp/travel-coverage.json \
  --emit-units-coverage-text /tmp/travel-coverage.txt \
  tests/fixtures/solver_physics/coverage_reporting.c
```

This is a frontend-only invocation. It does not emit objects, invoke a linker or
run code. Without `--units-coverage-strict`, unknown coverage does not change compilation acceptance. Existing
parser/semantic errors retain exit 1; successfully collected semantic contradictions
can still be reported with that exit. Parser failure or a missing/ambiguous function
preserves existing report files. Scope/function options and each output option may
occur once. `-`, multiple C/object inputs, ordinary `-o`/link outputs, other artifact
modes, build manifests and external/hybrid preprocessing are refused. Explicit
`-I`, `-D`, forced includes and dialect options retain ordinary frontend meaning.

## Reading the report

The text report shows counts and source-located obligations with operation, state,
reason, observed loss location, expected/actual dimension/unit facts and authored
origins. It distinguishes **checked**, **trusted-boundary**, **unchecked**, and
**contradictory**. Resolved metadata alone is not proof of a checked operation.
The separate [internal state contract](units_coverage_contract.md) describes fact
origins and the conservative assessment interface.

The JSON artifact has schema `fisiCs.units_coverage`, schema_version `1`, and:

- `source`, captured frontend-input FNV-1a hash/length, and ordered explicit analysis
  configuration: dialect, compatibility flags, macro definitions, include paths
  and forced includes. The hash uses the existing compiler source-contract capture;
  forced includes can produce a wrapped frontend input. It is not a cryptographic
  release identity or a digest of the complete header closure.
- `scope`: selected function name and definition location, `enumeration_complete`
  and unsupported-node count. Locations follow preprocessing logical source ranges;
  real included-file origins and macro source filenames remain valid through context
  destruction. The report does not claim physical execution or frozen-project freshness.
- `compilation_errors` and `strict_enforcement` (false by default). Error count is TU-wide even
  when reporting one function. A scope can have no contradictions while another
  function causes compilation to fail.
- `denominator`: contract `selected_ast_obligations_v1`, plus total, checked,
  trusted, unchecked and contradictory counts. Their sum equals total.
- `expressions`: deterministic traversal IDs (`e0`, ...), facts, observed loss
  locations and dependency references. Facts distinguish declared, inferred and
  unknown origins, nullable dimensions/units, provenance completeness and trust.
  Dependency roles are `value`, `argument`, `storage_base`, `index` or `condition`.
  Navigation/control edges are not interchangeable with value-inference evidence.
- `obligations`: deterministic IDs (`r0`, ...), string boundary kind
  (`initializer`, `assignment`, `argument`, `return`, `operation`), slot/index,
  operation, state/reason, comparison flags, expected/actual facts, source/loss
  locations and optional result-expression ID. Unsupported sentinel slots are null.

IDs are deterministic within this source/scope/configuration traversal, not
persistent cross-revision symbol IDs. References never expose pointer values.
Repeated identical invocations produce byte-identical artifacts. Input changes
may alter traversal IDs; consumers must retain scope and source/configuration context.

## Denominator and supported projection

Collection walks the selected AST, including unresolved operations, both branches,
loop initializer/condition/increment/body, cases and statement sequences. It counts
explicit initializer, assignment, argument and value-return boundaries, plus
encountered binary/unary/cast/merge operations and call-result rules. Empty selection
is zero observations. There is no coverage percentage or whole-solver success flag.

Supported local arithmetic rules include multiplication/division exponent algebra,
addition/subtraction/comparison compatibility, sign/cast preservation and ternary
branch compatibility. Known dimension/unit mismatches stay contradictory. Supported
compound assignments compare the resulting dimensions rather than treating every
right operand as the stored result. Unsupported rules stay unchecked.

Declarations and exact resolved field/array origins provide type evidence. Local
inference links operands and verifies supported result dimensions instead of
promoting the legacy result table into a complete proof. Recognized standard
`sqrt`, `fabs`, `hypot` and `fmax` calls use the math-operation identity captured
by core semantic analysis, not the function name. Their supported dimensional
rules can be checked; unsupported result rules remain unchecked alongside the
compiler diagnostic. Header/local function signatures remain trusted result
interfaces even when a body is present; reporting a caller does not qualify that
body. Direct-call parameter origins are retained when the real declaration exists;
missing indirect-contract origins remain incomplete.

Literal-only initialization of an explicitly annotated quantity is a declared
representation assumption and is counted as trusted, not inferred numeric proof.
An annotated destination cannot establish the units of a raw source. General
alias/dataflow histories and cross-function implementation proofs are not inferred.
Type assertions/casts remain visible as operations and their dependencies; a typed
access is evidence of its declared type, not proof of the referenced object's values.

The denominator deliberately includes unannotated controls and unknown calls in
the selected body. The optional [strict policy](units_coverage_strict.md) selects
required obligations without changing this reporting denominator or its states.
Ordinary C outside the selected invocation does not require physics annotations.

## Incomplete enumeration and loss reporting

Record/array compound initializers and statement expressions retain their observed
children and an unchecked unsupported sentinel, but mark enumeration incomplete.
Other unsupported AST forms (including currently unhandled size/alignment forms
and inline assembly) also produce a sentinel rather than a full-coverage claim.
The collector limits recursion to 192 and expression/obligation tables to 16,384;
resource-limit/allocation failure prevents publication. This version supports one
selected function definition, not an automatic application-wide sweep.

Missing metadata remains unknown, never dimensionless. The raw acceleration-field
and heterogeneous 6x6 stiffness-slot fixtures remain gaps. Typed carriers improve
their declared checking surface, while assumed raw restoration and opaque helpers
cannot establish checked implementation semantics merely by relabeling values.
Loss locations identify observed unknown/incomplete inputs, not fabricated origins.

## Output and verification contract

Each output is written to a sibling temporary file, flushed and atomically renamed.
Input source and resolved included files are protected from direct, symlink and
hardlink destination aliases; JSON/text destinations must be distinct. Both paths
are preflighted before publication. The two files are individually atomic, not a
cross-file transaction: an I/O failure on the second file may leave the first newly
published. A nonzero publication exit must be reconciled before consuming a pair.
Existing valid reports survive injected write failures, and temporary files are
removed on failed publication. Old units metadata JSON and the public frontend
ABI remain unchanged.

Run `make integration-units-coverage-contracts` for scope, exact denominator,
annotation-removal, raw/typed storage, header/macro origin, argument, trusted math,
contradiction, deterministic output and publication controls. The gate also checks
C legality with Clang. Clang does not validate the physics overlay. Existing solver
numerical gates and strict qualification inventory remain independent evidence.

## Optional strict policy

Add `--units-coverage-strict` to make required unknowns, contradictions, incomplete
scope enumeration and unpermitted trust fail this frontend gate. The separate
[strict-policy contract](units_coverage_strict.md) defines ordinary loop controls,
explicit trust permissions, nonvacuous acceptance and additional JSON fields.
Report-only artifacts retain their existing schema and bytes when strict mode is off.

The joined `make integration-units-coverage-acceptance` gate qualifies this report
contract together with strict selection and targeted metadata-removal controls;
see [joined acceptance qualification](units_coverage_strict.md#joined-acceptance-qualification).

Strict-only `strict_basis` identifies proven C predicates/status controls and
eligible declared C interfaces; `interface_callee` identifies a direct trusted
carrier helper. Original fact/observation states remain distinct from strict role
selection. See [the real-C policy](units_coverage_strict.md#real-c-controls-and-source-defined-carrier-interfaces).
Literal writes into declared storage now use the same explicit representation
assumption as literal initialization; nonliteral values retain normal checks.
