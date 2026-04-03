# Binary Harness (Phase A)

This directory contains the initial safe binary-testing lane.

## Entrypoints

- `make test-binary-smoke`
- `make test-binary-io`
- `make test-binary-link`
- `make test-binary-stdio`
- `make test-binary-fortify`
- `make test-binary`
- `make test-binary-id ID=<test_id>`
- `make binary-regen TEST=<test_id> CONFIRM=YES`

## Layout

- `cases/` source inputs
- `expect/` runtime expectations (`.stdout` / `.stderr`)
- `meta/` manifest shards and `index.json`
- `run_binary.py` harness

## Current Scope

Current coverage:

- Phase A:
  - compile-only smoke checks
  - deterministic runtime smoke checks
  - single test selection and explicit expectation regen
- Phase B:
  - controlled runtime I/O checks (file roundtrip, append/readback, stdin->file)
  - link-failure classification lane (`category: link_fail`)
  - stdio formatting lane (`printf`, `fprintf`, `snprintf`, `vsnprintf`)
  - fortify builtin lowering lane (`str*` + `mem*` wrappers through normal libc APIs)

## Safety Controls Enabled

- per-test artifact isolation under `build/tests/binary/<test_id>/`
- runtime `cwd` forced to isolated per-test directory
- environment allowlist for compile/run subprocesses
- timeout per test phase (`compile_timeout_sec` / `run_timeout_sec` / `timeout_sec`)
- basic resource limits by `resource_profile` (`smoke|default|heavy`)
- fail-closed expectation checks (no implicit pass on missing/mismatched output)

## Metadata Fields (Phase A)

- `id`
- `level`
- `category`: `compile_only` or `runtime`
- `inputs` (or `input`)
- `args` (compiler args)
- `expect_exit` (runtime, defaults to `0`)
- `expected_stdout`, `expected_stderr` (runtime optional; omitted means expected empty)
- `run_args`, `run_stdin`, `run_env` (runtime optional)
- `timeout_sec`, `compile_timeout_sec`, `run_timeout_sec`
- `resource_profile`
- `env` (compile env overrides)
- `skip_if.missing_tools`
- `expect_output_contains` (for `link_fail`)
- `expected_files` (for runtime file assertions inside isolated run dir)
