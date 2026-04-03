# SDL Binary Phase E Wave Plan

Date: 2026-04-03

## Scope

This plan defines the next SDL-focused binary test waves after E1 stabilization.
E1 (compile/link + real-header coverage) is complete and green.

Current status:

- E1 complete and promoted (`binary-sdl-wave1.json`).
- E2 complete and promoted (`binary-sdl-wave2.json`).
- E3 complete and promoted (`binary-sdl-wave3.json`).
- E4 differential shard complete and promoted (`binary-sdl-wave4.json`).
- E4 policy-skip shard complete and promoted (`binary-sdl-wave5.json`).
- Next active target: optional E5 expansion (renderer/window events under stricter gating).

Execution model remains probe-first:

1. Add tests in a temporary probe manifest.
2. Run targeted wave repeatedly until green.
3. Promote passing tests into the canonical SDL manifest.
4. Run `make test-binary-sdl`, then `make test-binary`.

## Wave E2: Headless Runtime Core (Completed)

Goal: prove deterministic runtime behavior with dummy drivers and no GPU/display dependency.

Implemented test IDs:

- `binary__runtime__sdl2_headless_init_quit`
- `binary__runtime__sdl2_headless_error_roundtrip`
- `binary__runtime__sdl2_ticks_monotonic_nonnegative`
- `binary__runtime__sdl2_event_push_poll_roundtrip`

Expected runtime env:

- `SDL_VIDEODRIVER=dummy`
- `SDL_AUDIODRIVER=dummy`

## Wave E3: Software Surface/Window Path (Completed)

Goal: validate bounded SDL functionality beyond init/quit while staying deterministic.

Implemented test IDs:

- `binary__runtime__sdl2_hidden_window_create_destroy`
- `binary__runtime__sdl2_surface_fill_readback`
- `binary__runtime__sdl2_surface_blit_checksum`
- `binary__runtime__sdl2_rwops_mem_roundtrip`

Notes:

- Prefer software-only APIs (`SDL_CreateRGBSurfaceWithFormat`, `SDL_BlitSurface`).
- Avoid renderer/GPU-specific assumptions in baseline tests.

## Wave E4: Differential + Policy Checks (Completed)

Goal: compare stable SDL runtime outputs against `clang` and harden skip behavior.

Implemented differential test IDs:

- `binary__runtime__diff_clang_sdl2_headless_init_quit`
- `binary__runtime__diff_clang_sdl2_ticks_monotonic_nonnegative`
- `binary__runtime__diff_clang_sdl2_surface_fill_readback`
- `binary__runtime__diff_clang_sdl2_event_push_poll_roundtrip`

Implemented policy checks:

- Skip-gated SDL tests when `pkg-config` is missing.
- Skip-gated SDL tests when `pkg-config sdl2` probe fails.
- Ensure skips are counted as `SKIP`, not `PASS`.

## Promotion Gates

A wave is promoted only when all are true:

- 3 consecutive targeted wave runs are green.
- No `harness_error`, `runtime_timeout`, or flaky output.
- `make test-binary-sdl` is green after promotion.
- `make test-binary` is green after promotion.

## Command Flow

Probe run:

`make test-binary-wave WAVE=<n> BINARY_WAVE_BUCKET=binary-sdl`

SDL lane check:

`make test-binary-sdl`

Global binary check:

`make test-binary`
