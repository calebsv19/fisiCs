# Physical compiler-flow qualification

This finite campaign checks the usability of optional units/physical semantics
for authored physical information across C paths. It does not prove arbitrary
alias/data flow, solver conservation, model accuracy or physical acceptance.

From a matching compiler checkout, use isolated tools and fresh result roots:

```sh
python3 -B tests/integration/run_flow_campaign.py \
  --compiler /path/to/fisics \
  --matrix tests/fixtures/solver_physics/flow_campaign/matrix.json \
  --frame-probe /path/to/build/tests/physical_frame_flow_contract \
  --energy-probe /path/to/build/tests/physical_energy_contract \
  --out /tmp/flow-campaign-new
python3 -B tests/integration/run_flow_repair_guards.py \
  --compiler /path/to/fisics --out /tmp/flow-guards-new
```

The existing Make rules build the two native probes against the selected build's
frontend library. See [the isolated build walkthrough](physics_authoring_path.md).
The runners do not change a default compiler, install an app or adopt solver code.
They create only the requested fresh result directories and temporary test outputs.

## Matrix and evidence

The manifest selects168 scenarios:52 positive,56 negative,32 unknown,16 explicit
trust and12 ordinary controls. Scenario IDs are distinct from command counts,
repeated timings and per-function obligation counts. Native frame, energy and
cross-TU suites exercise additional controls; their extra commands do not inflate
the168 authored scenario count.

B0 covers ordinary C and basic dimensional families. B1 covers scalar formulas,
explicit conversion, electrical/pressure quantities, kinds and references.
B2 exercises declaration/interface relationships and implementation trust.
B3 exercises fields/arrays, casts, aliases, branches and storage information loss.
B4 selects represented frames and energy qualifiers. B5 checks cross-TU receipts,
conflicts, metadata removal and explicit trust. Unsupported paths remain visible
refusals. Declared source compatibility is not callee body proof.

The owning suites assert source diagnostics and required/checked/trusted/blocked
accounting, both receipt input orders, source/policy freshness and intended
unknown/refusal outcomes. Ordinary controls compile and run under Clang and
ordinary/units/physical fisiCs. The manifest stores expectations and source hashes;
it is not generated from successful observations.

## Two bounded coverage repairs

A valid explicit unit conversion now retains its input provenance in selected
units coverage. It keeps four obligation rows: the conversion operation, source
argument, target argument and return. The validated concrete-unit string target
is labeled `explicit_unit_target_literal` and remains visible as an ordinary C
configuration argument, not checked physical arithmetic. The other three rows
can be checked when the source proof is complete. Unknown sources, invalid or
nonliteral targets and incompatible dimensions still fail. This adds no automatic
physical-kind or energy-form restoration to a conversion.

Physical strict coverage now honors the already admitted exact scalar
`*(&identifier)` reference. Its address row is a checked C reference relationship,
not a quantity-valued pointer; all three required address/read/return rows remain.
The directly referenced scalar must have known physical meaning and no represented
carrier. The pair must use the exact same AST address expression. Standalone
addresses, raw sources and pointer aliases remain refused; no path-sensitive
alias proof or arbitrary pointer annotation was added. Upstream trust remains
trust, and earlier unchecked writes remain obligations.

The focused guard runner verifies these boundaries, including that unit conversion
does not manufacture physical-kind meaning. The diagnostic probe harness also
contains single-source codegen output in its own temporary directory; it does
not suppress codegen to obtain a passing dump.

## Authoring and costs

Use source locations, expected/actual facts, first loss and explicit trust to guide
a repair. Some tasks legitimately end with an unsupported-interface refusal.
Removing metadata is not a repair. The campaign's six deterministic feedback
traces are not an AI-model success-rate benchmark.

Report/strict analysis is opt-in and measured separately from ordinary compilation.
Use [confidence tiers](compiler_test_confidence_tiers.md) after changes. Existing
matched cost tools require source/tool/policy identity and serial repetitions.
A current-versus-candidate ordinary compilation comparison measures regression
from the current change, not all historical extension overhead. No comparable
pre-extension build was established by this campaign. Whole-program lightweight
or zero-runtime-overhead claims require their own evidence; unchanged C storage
and a successful compiler invocation alone are insufficient.
