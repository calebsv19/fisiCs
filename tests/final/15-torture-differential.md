# Torture / Differential

This bucket is the stress and pressure lane for the canonical `tests/final/`
suite.

It complements the narrower behavior buckets by keeping hard, high-pressure
coverage in one stable place once those cases have a deterministic oracle.

## Scope

Use bucket `15` for:

- pathological parser and semantic surfaces
- malformed-input no-crash coverage
- differential runtime stress cases
- multi-translation-unit torture cases
- external-corpus and pinned high-pressure repros that are stable enough for
  routine validation
- policy-tagged UB or implementation-defined lanes that should remain visible
  but explicitly labeled

## Current Use

- focused bucket run: `make final-bucket BUCKET=torture-differential`
- exact id: `make final-id ID=<15__test_id>`
- broader checkpoint: `make final`

Probe-only or still-unstable high-pressure repros should stay in
`tests/final/probes/` until the oracle is deterministic enough for this stable
bucket.

## Acceptance Checklist

- pathological but supported inputs either compile and run correctly or fail
  closed with deterministic diagnostics
- malformed-input lanes never crash the compiler
- runtime differential lanes remain stable against the chosen trusted oracle
- multi-TU torture cases remain deterministic
- policy-tagged UB and implementation-defined lanes are clearly labeled
- promoted corpus fragments remain reviewable and reproducible

## Representative Coverage Areas

- long or deeply nested control-flow paths
- pathological declarators and type graphs
- high local/global declaration density
- large aggregate, layout, and array-pressure paths
- malformed token-stream and preprocessing robustness
- multi-TU stress around callbacks, state replay, and linkage pressure
- corpus-derived compile, diagnostic, and runtime smoke anchors
- reducer- and replay-style differential pressure surfaces

## Related Lanes

- `tests/final/probes/`: unstable or not-yet-promotable torture repros
- bucket `12`: diagnostic recovery when the primary question is front-end
  recovery quality rather than stress depth
- bucket `14`: runtime-surface truth when the main value is supported runtime
  behavior instead of torture pressure
- `tests/real_projects/`: whole-program canary pressure once a stable reduced
  repro already exists

## Public Note

This file is intentionally a stable public bucket reference.

Wave-by-wave promotion history, active expansion batching, and maintainer-only
triage sequencing belong in private maintainer docs instead of this public
summary.
