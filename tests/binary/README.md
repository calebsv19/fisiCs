# Binary Harness (Phases A-E)

This directory contains the safe binary-testing lane and its phased expansion.

## Current Phase Marker

- Current phase: **Phase E (E6 SDL + stdio/libc + math + linkage expansion lanes active)**
- Stable active scope: Levels 0-4 (`smoke`, `io`, `link`, `sdl`, `stdio`, `fortify`, `abi`)
- Added scope: Level 5 corpus lane through wave 9 (`compile_only` + `link_only` + bounded `runtime`)
- Added scope: differential lane (`level: diff`, waves 1-18, `fisics` vs `clang`)

## Current Snapshot

- `make test-binary-abi`, `make test-binary-corpus`, `make test-binary-wave`, `make test-binary-diff`, `make test-binary-link`, `make test-binary-math`, and `make test-binary` are green.
- Active inventory:
  - total tests: `244`
  - categories: `runtime=184`, `compile_only=19`, `compile_fail=15`, `link_fail=12`, `link_only=14`
  - levels: `smoke=17`, `io=4`, `link=8`, `sdl=34`, `stdio=11`, `math=4`, `fortify=3`, `abi=58`, `corpus=26`, `diff=79`

## Entrypoints

- `make test-binary-smoke`
- `make test-binary-io`
- `make test-binary-link`
- `make test-binary-sdl`
- `make test-binary-stdio`
- `make test-binary-math`
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
  - stdio/stdlib lane (`printf`, `fprintf`, `snprintf`, `vsnprintf`, `strtoul` endptr parsing)
  - fortify builtin lowering lane (`str*` + `mem*` wrappers through normal libc APIs)
- Phase C:
  - ABI stress runtime lane (`level: abi`)
  - ABI negative fail-closed lanes (`category: compile_fail`, `category: link_fail`)
  - compile-timeout expectation lane (`expect_compile_timeout`) for known hang sentinels
- Level 5 (initial):
  - corpus compile-only and link-only lanes (`level: corpus`)
  - bounded execute-safe runtime slice (`level: corpus`, waves 7-9)
- Differential lane (initial):
  - runtime parity checks against `clang` on UB-clean subsets (`level: diff`, waves 1-18)
- Phase E1 SDL lane:
  - compile/link-only SDL symbol-surface checks (`level: sdl`, wave 1)
  - direct SDL system-header coverage (`#include <SDL2/SDL.h>`)
  - stdinc inline/builtin-heavy coverage (`SDL_memcpy4`, `SDL_fabsf`)
  - tool-gated via `skip_if.missing_tools` + `skip_if.missing_pkg_config_modules`
- Phase E2 SDL lane:
  - deterministic headless runtime checks (`level: sdl`, wave 2)
  - dummy-driver env coverage (`SDL_VIDEODRIVER=dummy`, `SDL_AUDIODRIVER=dummy`)
  - event/timer/error runtime behavior checks
- Phase E3 SDL lane:
  - bounded runtime surface/window checks (`level: sdl`, wave 3)
  - hidden window create/destroy in dummy video mode
  - software surface fill/readback and blit verification
  - RWops memory roundtrip checks
- Phase E4 SDL lane:
  - `diff_clang` parity checks for SDL runtime shards (`level: sdl`, wave 4)
  - clang differential compile now respects `pkg_config_modules` for external link parity
  - policy-skip checks for missing tool/module paths (`level: sdl`, wave 5)
  - validated behavior: policy tests report `SKIP` (not `PASS`)
- Phase E5 SDL lane:
  - renderer-gated runtime checks (`level: sdl`, wave 6, opt-in via `ENABLE_SDL_RENDERER_TESTS`)
  - expanded SDL differential parity checks (`level: sdl`, wave 7)
  - SDL negative compile/link checks (`level: sdl`, wave 8)
  - dummy-audio runtime check (`level: sdl`, wave 9)
- Phase E6 SDL/stdio lane:
  - deterministic SDL runtime expansion (`level: sdl`, wave 10)
  - deterministic SDL differential expansion (`level: sdl`, wave 11)
  - stdio/libc parse+format matrix expansion (`level: stdio`, wave 5)
  - corresponding diff-clang parity expansion (`level: diff`, wave 16)
- Phase E7 SDL hardening lane:
  - Darwin-header regression guard on SDL stdinc inline compile path (`level: sdl`, wave 14)
  - renderer drawline+clip readback runtime coverage (`level: sdl`, wave 14)
  - corresponding diff-clang parity coverage for drawline+clip (`level: sdl`, wave 14)
- Phase E6 math lane:
  - deterministic math runtime matrix (`level: math`, wave 1)
  - corresponding diff-clang parity expansion (`level: diff`, wave 17)
- Phase E6 linkage lane:
  - multi-TU linkage runtime + link-fail stress (`level: link`, `binary-linkage-wave1`)
  - corresponding diff-clang parity expansion (`level: diff`, wave 18)

## Safety Controls Enabled

- per-test artifact isolation under `build/tests/binary/<test_id>/`
- runtime `cwd` forced to isolated per-test directory
- environment allowlist for compile/run subprocesses
- timeout per test phase (`compile_timeout_sec` / `run_timeout_sec` / `timeout_sec`)
- basic resource limits by `resource_profile` (`smoke|default|heavy`)
- fail-closed expectation checks (no implicit pass on missing/mismatched output)
- failure reporting now emits canonical taxonomy labels on failing paths:
  - `failure_kind=<...>`
  - `severity=<...>`
  - `source_lane=binary`
  - `trust_layer=<...>`
  - `owner_lane=<binary-compile|binary-link|binary-runtime>`
  - `raw_status=<existing binary status code>`

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
- `skip_if.missing_env` (opt-in gating for environment-sensitive runtime lanes)
- `pkg_config_modules` (resolved via `pkg-config --cflags --libs`)
- `expect_output_contains` (for `compile_fail` and `link_fail`)
- `compile_output_must_not_contain` (for any category; fail-closed on forbidden compile-output fragments)
- `expect_compile_timeout` (optional compile timeout expectation)
- `expected_files` (for runtime file assertions inside isolated run dir)
- `ub` (optional policy tag; skips differential comparison when `true`)
- `impl_defined` (optional policy tag; skips differential comparison when `true`)
- `differential_with` (optional; currently `clang` for runtime parity checks)
- `differential_compiler`, `clang_args`, `clang_env`, `clang_run_env` (optional overrides)
