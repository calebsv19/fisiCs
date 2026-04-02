# Link-Stage Diagnostics JSON Enablement Plan

Date: 2026-03-28  
Scope: Compiler driver + final harness behavior for link-time failures in
bucket `14` multi-TU negative tests.

Status (2026-03-28): Implemented through Wave89 (driver fallback + probe/final
promotion for link-failure `.diagjson` parity lanes).

## Goal

Enable `--emit-diags-json <path>` to produce a valid `.diagjson` artifact even
when failure occurs in the link stage (duplicate symbols, extern/type collisions,
etc.), so bucket-14 link-failure lanes can use structured JSON diagnostics
instead of text-only `.diag`.

This should be done with minimal code-path duplication and without regressing
current frontend diagnostic JSON behavior.

## Historical Baseline (Pre-Wave89)

- Frontend compile paths already emitted JSON via:
  `compiler_diagnostics_write_core_dataset_json(...)`.
- In link-driver mode (`src/main.c`), each `.c` input can emit per-input JSON,
  but linker failures only printed text (`Linker exited with status ...`) and
  returned non-zero.
- Existing link-conflict negative tests therefore relied on `.diag` text-only
  expectations (`capture_frontend_diag: true`, `allow_nonzero_exit: true`).
- `.diagjson` expectations failed for pure link-stage errors because no
  structured file was written on linker failure.

## Design Constraints

- Preserve existing JSON schema surface used by harness/tests:
  - `profile`
  - `schema_version`
  - `diag_count`
  - `error_count`
  - `warning_count`
  - `note_count`
  - `diagnostics[]` entries with `line/column/length/kind/code/...`
- Do not require the linker to provide machine-readable diagnostics.
- Keep path handling deterministic for single-input and multi-input flows.
- Avoid touching parser/sema/codegen internals for this feature.

## Architecture Decision

Use a **driver-level synthetic diagnostic JSON** fallback for link failures.

When linker exits non-zero and `--emit-diags-json` is active:
1. Build a tiny JSON payload in the driver.
2. Write it to the expected output path (`diagsJsonPath`).
3. Keep existing stderr behavior unchanged.

Rationale:
- Fastest path with lowest regression risk.
- No dependency on linker output parsing quality.
- Works for all link failure types consistently.

## Phased Implementation

## Phase 1: Driver JSON Fallback Primitive

Add a small helper in `src/main.c`:

- `write_link_stage_diag_json(const char* out_path, int exit_code, const char* linker_name, const char* output_name, size_t input_count)`

Behavior:
- Writes a single diagnostic entry (`kind=error`).
- Uses stable code (new linker code namespace, e.g. `4001`).
- Sets conservative location (`line=0`, `column=0`, `length=0`) if no source
  location is available.
- Includes deterministic metadata (linker name, exit code, input count) in the
  message/hint text.

Keep helper local to `main.c` initially to avoid broad API churn.

## Phase 2: Integrate In Link Failure Branch

In the link-driver path in `src/main.c`:

- After `waitpid` and exit-status evaluation, when final linker status is non-zero:
  - If `diagsJsonPath` is set, call `write_link_stage_diag_json(...)`.
  - Best-effort write: print warning if write fails, but do not mask original
    linker failure exit status.

Keep all existing stderr lines (`Linker exited with status ...`) intact.

## Phase 3: Harness Compatibility Check

No schema changes required in `tests/final/run_final.py`; it already:
- passes `--emit-diags-json` when `.diagjson` or `.pdiag` expectations are present,
- fails closed if JSON file is missing.

Required validation:
- Ensure new link-stage JSON is accepted by current `.diagjson` normalization.
- Ensure non-link tests are unaffected.

## Phase 4: Test Promotion Plan (Bucket 14)

1. Add probe-first `.diagjson` versions for link-failure lanes:
   - duplicate external definition
   - extern type mismatch
   - duplicate tentative/type conflict
   - duplicate function definition
2. Run targeted probe checks.
3. Promote to active wave shard only when:
   - `make final-wave WAVE=<new-wave>` passes,
   - `make final-runtime` remains green.

Until then, keep existing `.diag` lanes active as baseline.

## Phase 5: Cleanup And Policy Tightening

After `.diagjson` lanes pass reliably:
- Update bucket-14 docs to remove the “link-stage `.diagjson` unsupported” gap.
- Optionally keep `.diag` link lanes for redundancy, or trim duplicates.
- Mark link-stage JSON support as stable in workflow docs.

## Data Contract

Minimum link-stage JSON contract:

- `profile`: `"fisics_diagnostics_v1"`
- `schema_version`: integer
- `diag_count`: `>=1`
- `error_count`: `>=1`
- `diagnostics[0].kind`: error kind value
- `diagnostics[0].code`: linker-stage code (stable)

Do not overfit tests to full linker message text.
Assert category/code presence first; message substring second.

## Risk Register

1. Risk: Introducing duplicated JSON builders in `main.c`.
   Mitigation: Keep a compact local writer; if it grows, refactor into
   `Compiler/diagnostics.*` in a second pass.

2. Risk: Inconsistent code values breaking tests.
   Mitigation: define stable linker diagnostic code constant(s) in one place.

3. Risk: Multi-input path emits per-input JSON names while linker emits one.
   Mitigation: specify and document link-stage output path rule clearly.

4. Risk: Masking original linker failure status.
   Mitigation: JSON write must never alter non-zero return path.

## Rollout Gates

Gate A (implementation):
- Build succeeds.
- Existing final suite unchanged.

Gate B (targeted validation):
- New link-stage `.diagjson` probes pass.
- Existing `.diag` link tests still pass.

Gate C (bucket stability):
- `make final-wave WAVE=<link-diagjson-wave>` pass.
- `make final-runtime` => `0 failing`.

## Suggested Task Breakdown

1. Add `write_link_stage_diag_json` helper in `src/main.c`.
2. Call helper on linker non-zero exit in driver mode.
3. Add probe lanes for link-stage `.diagjson`.
4. Promote lanes + expectations in one wave shard.
5. Update docs/run log and close bucket-14 gap note.

## Out Of Scope

- Parsing linker stderr into precise file/line diagnostics.
- Changing frontend diagnostics schema.
- Reworking general diagnostics subsystem architecture.
