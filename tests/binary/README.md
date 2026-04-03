# Binary Harness (Phases A-E)

This directory contains the safe binary-testing lane and its phased expansion.

## Current Phase Marker

- Current phase: **Phase E (E1 SDL compile/link lane active)**
- Stable active scope: Levels 0-4 (`smoke`, `io`, `link`, `sdl`, `stdio`, `fortify`, `abi`)
- Added scope: Level 5 corpus lane through wave 9 (`compile_only` + `link_only` + bounded `runtime`)
- Added scope: differential lane (`level: diff`, waves 1-15, `fisics` vs `clang`)

## Current Snapshot

- `make test-binary-abi`, `make test-binary-corpus`, `make test-binary-wave`, `make test-binary-diff`, and `make test-binary` are green.
- Active inventory:
  - total tests: `190`
  - categories: `runtime=136`, `compile_only=19`, `compile_fail=14`, `link_fail=9`, `link_only=12`
  - levels: `smoke=17`, `io=4`, `link=3`, `sdl=6`, `stdio=5`, `fortify=3`, `abi=58`, `corpus=26`, `diff=68`

## Entrypoints

- `make test-binary-smoke`
- `make test-binary-io`
- `make test-binary-link`
- `make test-binary-sdl`
- `make test-binary-stdio`
- `make test-binary-fortify`
- `make test-binary-abi`
- `make test-binary-corpus`
- `make test-binary-diff`
- `make test-binary-wave WAVE=<n> [BINARY_WAVE_BUCKET=<bucket-prefix>]`
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
- Phase C:
  - ABI stress runtime lane (`level: abi`)
  - ABI negative fail-closed lanes (`category: compile_fail`, `category: link_fail`)
  - compile-timeout expectation lane (`expect_compile_timeout`) for known hang sentinels
- Level 5 (initial):
  - corpus compile-only and link-only lanes (`level: corpus`)
  - bounded execute-safe runtime slice (`level: corpus`, waves 7-9)
- Differential lane (initial):
  - runtime parity checks against `clang` on UB-clean subsets (`level: diff`, waves 1-15)
- Phase E1 SDL lane:
  - compile/link-only SDL symbol-surface checks (`level: sdl`, wave 1)
  - direct SDL system-header coverage (`#include <SDL2/SDL.h>`)
  - stdinc inline/builtin-heavy coverage (`SDL_memcpy4`, `SDL_fabsf`)
  - tool-gated via `skip_if.missing_tools` + `skip_if.missing_pkg_config_modules`

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
- `category`: `compile_only`, `compile_fail`, `runtime`, `link_fail`, or `link_only`
- `inputs` (or `input`)
- `args` (compiler args)
- `expect_exit` (runtime, defaults to `0`)
- `expected_stdout`, `expected_stderr` (runtime optional; omitted means expected empty)
- `run_args`, `run_stdin`, `run_env` (runtime optional)
- `timeout_sec`, `compile_timeout_sec`, `run_timeout_sec`
- `resource_profile`
- `env` (compile env overrides)
- `skip_if.missing_tools`
- `skip_if.missing_pkg_config_modules`
- `pkg_config_modules` (resolved via `pkg-config --cflags --libs`)
- `expect_output_contains` (for `compile_fail` and `link_fail`)
- `expect_compile_timeout` (optional compile timeout expectation)
- `expected_files` (for runtime file assertions inside isolated run dir)
- `differential_with` (optional; currently `clang` for runtime parity checks)
- `differential_compiler`, `clang_args`, `clang_env`, `clang_run_env` (optional overrides)
