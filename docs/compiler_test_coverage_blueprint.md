# Compiler Behavior Coverage Map (Public)

This document is the public coverage map for `fisiCs`.

It shows which behavior areas are currently:

- **Fully validated**
- **Partially validated**
- **Not yet validated**

Use this to decide where to contribute next.

## Coverage Level Definitions

### Fully Validated

Behavior area has:

- Positive-path tests
- Negative/error-path tests
- Edge/stress cases
- Included in regular suite execution (`make test` and/or `make final` where applicable)
- Recent passing evidence without known open blockers

### Partially Validated

Behavior area has meaningful tests and passing coverage, but at least one of the following is still missing:

- complete negative/error coverage
- sufficient edge/stress coverage
- stable runtime/differential validation for key sub-areas
- unresolved known blockers in sub-features

### Not Yet Validated

Behavior area is missing structured tests or only has ad hoc/probe-level checks.

## Current Public Coverage Status

| Area | Status | Notes |
|---|---|---|
| Lexer/tokenization fundamentals | Partially validated | Strong baseline coverage exists; frontier edge cases and broad conformance depth are still expanding. |
| Preprocessor core behavior | Partially validated | Macro/include/conditional coverage is substantial, but full conformance depth remains in progress. |
| Parser and grammar breadth | Partially validated | Major statement/declaration/expression paths are tested; uncommon grammar frontiers still being expanded. |
| Semantic/type checking | Partially validated | Many semantic lanes are covered; there are still blocked/expanding regions in deeper edge behavior. |
| IR/codegen lowering | Partially validated | Active coverage is broad and improving; some complex lowering/perf-sensitive surfaces remain iterative. |
| Runtime behavior parity | Partially validated | Runtime lanes exist and are active; full parity confidence across all surfaces is still in progress. |
| Binary/external-program validation | Partially validated | `datalab` and `line_drawing` milestones are validated; broader external surface remains ongoing. |
| Torture/differential stress | Partially validated | Stress and differential lanes are active, but not yet complete across all intended stress classes. |
| Cross-platform target/ABI matrix | Not yet validated | No public claim of full cross-platform ABI validation matrix at this time. |
| Full C17 conformance claim | Not yet validated | Project is trending toward C17 but does not claim full conformance coverage yet. |

## Contributor Priorities

If you want high-impact contributions, prioritize:

1. Closing partial areas toward full validation (especially semantics, runtime, and differential lanes).
2. Adding negative and edge tests where only happy-path coverage exists.
3. Improving deterministic diagnostics and reproducibility for failing cases.
4. Expanding external-project compile/run validation with minimal, isolated repro tests.

## Minimum Standard For Marking A Behavior "Fully Validated"

Do not mark a behavior fully validated until all are true:

1. Positive, negative, and edge variants exist.
2. The behavior is covered by stable suite commands (not only one-off probes).
3. No known open blocker remains for that behavior.
4. Recent test evidence is included in PR/maintenance logs.

## How To Contribute Safely

- Keep PRs focused on one behavior cluster.
- Add tests before or with fixes.
- Prefer minimal repros.
- Include exact command evidence.
- Update this map when coverage status materially changes.

## Related Public Docs

- `docs/compiler_test_system_rearchitecture_context.md`
- `docs/compiler_test_workflow_guide.md`
- `docs/contributor_agent_quickstart.md`
