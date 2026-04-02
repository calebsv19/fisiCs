# Semantic Bucket 07 Execution Plan

Date: 2026-04-01

This plan tracks wave-based expansion for `tests/final` bucket `07`
(`types-conversions`) with probe-first promotion.

## Current Baseline

- Active manifests:
  - `tests/final/meta/07-types-conversions.json`
  - `tests/final/meta/07-types-conversions-wave2-semantic.json`
- Active tests: `16`
- Current validation:
  - `make final-prefix PREFIX=07__` -> all pass
  - `make final-wave WAVE=2 WAVE_BUCKET=07-types-conversions` -> all pass
  - `PROBE_FILTER=07__probe_* python3 tests/final/probes/run_probes.py`
    -> `resolved=7`, `blocked=0`, `skipped=0`

## Wave 2 (Completed)

Promoted tests:

- `07__assign_struct_to_int_reject`
- `07__agg__member_access`
- `07__agg__offsets`
- `07__agg__invalid_member_reject`
- `07__constexpr__array_size_reject`
- `07__constexpr__case_label_reject`
- `07__constexpr__static_init_reject`

Probe-first notes:

- Initial probe blocker was a strict static-initializer diagnostic substring
  mismatch; matcher was adjusted to expected emitted wording
  (`not a constant expression`).
- Re-run after matcher fix: no blocked probes.

## Next Wave Targets (Wave 3 Queue)

1. Conversion negative matrix:
   - reject mixed non-scalar arithmetic in conversion contexts,
   - reject invalid pointer/integer conversion assignments where required.
2. Signed/unsigned comparison depth:
   - add boundary matrix with wider integer rank mixes.
3. Semantic diagnostic JSON parity:
   - add `07__diagjson__*` parity lanes for representative rejects:
     invalid member access, case-label constexpr reject, static-init constexpr reject.
4. Aggregate offset follow-up:
   - extend offset checks across nested aggregate member paths.

## Execution Rules

1. Add probe lanes first (`07__probe_*`).
2. Run full bucket probe filter before promotion:
   `PROBE_FILTER=07__probe_* python3 tests/final/probes/run_probes.py`
3. Promote only resolved probes into `07-types-conversions-waveN-*.json`.
4. Validate with:
   - `make final-manifest MANIFEST=<wave-manifest>.json`
   - `make final-wave WAVE=<n> WAVE_BUCKET=07-types-conversions`
   - `make final-prefix PREFIX=07__`
