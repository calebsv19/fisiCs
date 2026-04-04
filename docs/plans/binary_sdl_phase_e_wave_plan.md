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
- E5 gated runtime expansion shards complete and promoted (`binary-sdl-wave6.json` through `binary-sdl-wave9.json`).
- E6 deterministic runtime expansion shards complete and promoted (`binary-sdl-wave10.json`, `binary-sdl-wave11.json`, `binary-sdl-wave12.json`, `binary-sdl-wave13.json`).
- Next active target: optional E7 expansion (deeper renderer/input/audio stress under stricter gating).

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

## Wave E5: Gated Expansion (Completed First Pass)

Goal: broaden SDL runtime surface while preserving deterministic CI behavior via explicit gating.

Implemented test IDs:

- `binary__runtime__sdl2_renderer_hidden_software_clear`
- `binary__runtime__sdl2_renderer_texture_lock_cycle`
- `binary__runtime__diff_clang_sdl2_hidden_window_create_destroy`
- `binary__runtime__diff_clang_sdl2_surface_blit_checksum`
- `binary__runtime__diff_clang_sdl2_rwops_mem_roundtrip`
- `binary__compile_fail__sdl2_init_too_few_args`
- `binary__link_fail__sdl2_missing_symbol`
- `binary__runtime__sdl2_audio_dummy_open_close`

Gating/Policy:

- Renderer runtime shards are gated with `skip_if.missing_env: [ENABLE_SDL_RENDERER_TESTS]`.
- Default suite behavior remains deterministic: gated renderer tests skip unless explicitly enabled.

## Wave E6: Deterministic Runtime + Diff Expansion (Completed)

Goal: deepen non-flaky SDL runtime semantics and close differential parity for the added lanes.

Implemented runtime test IDs:

- `binary__runtime__sdl2_register_events_roundtrip`
- `binary__runtime__sdl2_surface_lock_write_readback`
- `binary__runtime__sdl2_perfcounter_monotonic`

Implemented differential test IDs:

- `binary__runtime__diff_clang_sdl2_register_events_roundtrip`
- `binary__runtime__diff_clang_sdl2_surface_lock_write_readback`
- `binary__runtime__diff_clang_sdl2_perfcounter_monotonic`

Notes:

- All E6 lanes run under dummy drivers to keep behavior deterministic.
- E6 wave10 and wave11 pass independently and under full `make test-binary-sdl`.

## Wave E6b: Deterministic Runtime + Diff Expansion (Completed)

Goal: extend deterministic non-renderer SDL surface with stable geometry/stdinc lanes.

Implemented runtime test IDs:

- `binary__runtime__sdl2_rect_ops_matrix`
- `binary__runtime__sdl2_stdinc_string_helpers`
- `binary__runtime__sdl2_rect_line_clip_matrix`
- `binary__runtime__sdl2_pixel_format_roundtrip`

Implemented differential test IDs:

- `binary__runtime__diff_clang_sdl2_rect_ops_matrix`
- `binary__runtime__diff_clang_sdl2_stdinc_string_helpers`
- `binary__runtime__diff_clang_sdl2_rect_line_clip_matrix`
- `binary__runtime__diff_clang_sdl2_pixel_format_roundtrip`

Notes:

- E6b wave12 and wave13 pass independently and under full `make test-binary-sdl`.
- Fixed lane detail: `&identifier` codegen now resolves lvalues before falling back to global-function designators, which resolved the `y1` shadowing issue exposed by the rect-line clip test.

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
