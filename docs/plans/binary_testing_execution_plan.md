# Binary Testing Execution Plan

Date: 2026-04-02

## Goal

Introduce a safe, deterministic binary-testing lane that starts small and
expands in controlled levels, so we can validate real executable behavior
without risking filesystem damage or unstable harness behavior.

## Current State (Grounded Snapshot)

- `make test` and `make final` are currently green.
- Binary-adjacent checks already exist:
  - `tests/integration/compile_only.sh`
  - `tests/integration/compile_and_link.sh`
  - runtime execution in `tests/final/run_final.py` for `run: true` tests
- Current runtime harness has two safety gaps for larger binary campaigns:
  - no per-test runtime timeout
  - runtime binaries inherit broad environment and run in shared working dir

## Safety Model (Fail-Closed)

Absolute zero-risk cannot be guaranteed for arbitrary native executables, but
impact can be tightly contained.

Required controls before broad binary expansion:

- Run each binary in an isolated per-test run directory under `build/tests/`.
- Force per-test `cwd` to that run directory.
- Use a strict env allowlist (`PATH`, locale vars, explicit test vars only).
- Set per-test timeouts for compile, link, and runtime phases.
- Set resource limits (`ulimit`) for CPU time, file size, and open files.
- Capture and compare `stdout`, `stderr`, and exit code.
- Kill and fail on timeout/hang.
- Keep all generated artifacts out of repo source tree.
- Default mode never rewrites expectations.

Optional hardening (phase-later):

- OS sandbox profile for runtime process (filesystem/network restrictions).
- Sanitizer lane for harness/helper binaries.

## Test Levels (Progressive Rollout)

## Level 0: Compile-Only Binary Surface

- Compile single-file and multi-file programs to object or executable.
- No runtime execution.
- Purpose: driver/link pipeline confidence with minimal risk.

## Level 1: Runtime Smoke (Pure Deterministic)

- Run tiny deterministic binaries (no filesystem writes).
- Compare exact `stdout` + exit code.
- Short timeout and strict resource limits.

## Level 2: Runtime I/O Surface (Controlled)

- Add controlled `stdin`, argv, and explicit env input tests.
- Keep writes restricted to per-test temp dir.
- Validate `stdout`, `stderr`, exit code.

## Level 3: Multi-TU and Linkage Matrix

- Multi-translation-unit binaries with extern/static/tentative definitions.
- Link-error expectation lane (negative link tests).
- Ensure precise compile-vs-link failure classification.

## Level 4: ABI and Calling Convention Frontier

- Many-arg calls, struct returns, variadics, function pointers.
- Architecture-sensitive cases tagged `abi_sensitive`.
- Differential runtime checks only where behavior is stable.

## Level 5: External Mini-Corpus (Compile First)

- Curated small real-world fragments.
- Start compile-only, then selective execute-safe binaries.
- No mass execution of unknown binaries until earlier levels are stable.

## Harness Architecture

Proposed new harness:

- `tests/binary/run_binary.py` (manifest-driven, fail-closed)
- Inputs:
  - `tests/binary/meta/*.json`
  - `tests/binary/meta/index.json` registry
- Sources:
  - `tests/binary/cases/`
- Expectations:
  - `tests/binary/expect/`
- Artifacts:
  - `build/tests/binary/<test_id>/`

Per-test metadata minimum:

- `id`
- `category`: `compile_only | link_only | runtime | differential`
- `inputs` (one or more source files)
- `args` (compiler args)
- `run` (bool)
- `expect_exit`
- `expected_stdout`
- `expected_stderr`
- `timeout_sec`
- `resource_profile` (`smoke | default | heavy`)
- `ub` / `impl_defined` tags
- `skip_if` rules

## Make Entrypoints (Planned)

- `make test-binary` (all binary levels currently enabled)
- `make test-binary-smoke` (Level 0-1)
- `make test-binary-io` (Level 2)
- `make test-binary-link` (Level 3)
- `make test-binary-stdio` (stdio formatting lane)
- `make test-binary-fortify` (fortified builtin lowering lane)
- `make test-binary-abi` (Level 4)
- `make test-binary-corpus` (Level 5 compile-focused)
- `make test-binary-id ID=<test_id>`
- `make test-binary-wave WAVE=<n>`

Expectation update path (explicit only):

- `make binary-regen TEST=<id> CONFIRM=YES`
- No default/global binary golden rewrite path.

## Failure Taxonomy (Required Reporting)

Each failure must be classified as one of:

- compile_fail_unexpected
- compile_succeeded_unexpected
- link_fail_unexpected
- runtime_exit_mismatch
- runtime_stdout_mismatch
- runtime_stderr_mismatch
- runtime_timeout
- runtime_resource_limit
- differential_mismatch
- harness_error

## Promotion Gates

Advance levels only when all are true:

- level slice is green for 3 consecutive runs
- no unresolved `harness_error` or `runtime_timeout`
- no expectation drift without explicit reviewed update
- docs run-log updated with promoted tests and blocked IDs

## First Implementation Slice

Phase A (small, safe):

- Add `tests/binary/run_binary.py` with timeout + cwd isolation + env allowlist.
- Add Level 0-1 manifests (10-20 tests).
- Add `make test-binary-smoke`.
- Keep execution limited to deterministic no-write smoke binaries.

Phase B:

- Add Level 2-3 manifests.
- Add link-failure classification and stronger reporting.

Phase C:

- Add Level 4 ABI frontier lane and selective Level 5 corpus compile-only lane.

## Progress Snapshot

- Phase A implemented:
  - `tests/binary/run_binary.py`
  - `make test-binary-smoke`, `make test-binary-id`, `make binary-regen`
  - initial smoke manifest and cases
- Phase B in progress:
  - Level 2 (`io`) manifest scaffold
  - Level 3 (`link`) manifest scaffold
  - harness classification for `link_fail` category
  - stdio format lane added (`printf`/`fprintf`/`snprintf`/`vsnprintf`)
  - `fprintf(stderr, ...)` crash resolved by extern-global codegen fix for stdio globals
  - fortified builtin lowering expanded for `__builtin___*chk` wrappers:
    - `snprintf` / `vsnprintf` / `sprintf`
    - `memcpy` / `memmove` / `memset`
    - `strcpy` / `strcat` / `strncpy` / `strncat`
  - added `fortify` binary lane and manifest (`binary-fortify-wave5.json`)
  - hardened binary harness text decoding (`errors=replace`) to fail with diffs instead of crashing on non-UTF8 output
