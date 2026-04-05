# Line Drawing Binary Execution Plan

Date: 2026-04-03

## Completion Record (2026-04-04)

- `line_drawing` is now validated as functional with `fisics`-compiled objects.
- Manual GUI verification passed:
  - window creation/lifecycle is stable
  - controls and keybind/input handling respond
  - text and UI rendering are visually correct for the validated run
- Full compiler validation remained green during closeout:
  - `make final` => `0 failing, 36 skipped`

Implementation notes from this lane:

- Compiler-side ABI routing was hardened for external call boundaries involving
  small by-value aggregates in SDL/TTF paths.
- Shared `vk_renderer` code is intentionally **not** part of this completion
  change set; milestone closure is scoped to compiler behavior and test lanes.

## Goal

Run `line_drawing` through the same proven binary-validation path used for
`datalab`, with explicit phase gates and blocker isolation rules.

## Why This Target Is Next

- It expands coverage (SDL2 + SDL2_ttf + Vulkan + shared libs) without jumping
  straight to DAW-scale complexity.
- It has deterministic test lanes already (`make test`, `make test-stable`,
  `make run-headless-smoke`).
- It is medium-sized enough for targeted one-TU isolation when failures appear.

## Datalab Lessons To Preserve

1. Always build fresh probe binaries in `/tmp/<program>_probe/<stamp>/`.
2. Compare against a clang baseline first; do not debug `fisics` in isolation.
3. Keep runtime checks deterministic first (headless/default fixtures).
4. On any mismatch, run one-TU mix matrix (one `fisics` TU at a time, rest
   clang) to localize the failing translation unit quickly.
5. Only run GUI/manual validation after deterministic and differential lanes
   are green.
6. Do not commit runtime artifacts; keep repo state clean while probing.

## Phase Plan

### LD0: Preflight

- Confirm dependencies and tools:
  - `sdl2-config`, `pkg-config`, Vulkan headers/libs, SDL2_ttf
- Confirm baseline commands:
  - `make -C ../line_drawing test`
  - `make -C ../line_drawing test-stable`
  - `make -C ../line_drawing run-headless-smoke`

Gate: all baseline lanes pass with clang toolchain.

### LD1: Full Compile Lane (fisics compile + clang link)

- Compile every C TU with `./fisics -c`.
- Link resulting objects with clang.
- Keep build output in `/tmp/line_drawing_probe/<stamp>/`.

Gate: full build links successfully with no unresolved symbols.

### LD2: Deterministic Runtime Lane

- Run non-interactive deterministic lanes first:
  - project test binaries
  - headless smoke path
- Capture stdout/stderr/exit codes for both clang and fisics binaries.
- Diff outputs exactly where deterministic.

Gate: `rc=0` and deterministic output parity for all selected lanes.

### LD3: Soak Stability Lane

- Repeat deterministic runs (minimum 20-30 iterations per lane).
- Hash output logs and assert single-hash stability per lane.

Gate: zero runtime failures and stable output hashes across repetitions.

### LD4: Mix-Matrix Isolation Lane (Only If Needed)

- One-TU-at-a-time matrix:
  - compile one TU with `fisics`
  - compile remaining TUs with clang
  - link + run deterministic lanes
- Record failing TU IDs and failure type (`compile`, `link`, `runtime`,
  `diff`).

Gate: every blocker mapped to specific TU/function family before fix work.

### LD5: Fix and Regression Lane

- Apply minimal compiler fixes for mapped blockers.
- Add/extend binary regression tests in `tests/binary` where behavior belongs.
- Re-run:
  - focused reproducer
  - lane-specific binary tests
  - full `make test-binary`

Gate: blocker repro passes and no regression in existing binary lanes.

### LD6: Manual GUI Validation Lane

- Run interactive SDL/Vulkan app manually after deterministic lanes are green.
- Verify:
  - window creation and lifecycle
  - input loop responsiveness
  - no immediate crash on startup/close

Gate: interactive smoke success plus deterministic lanes still green.

## Blocker Classification

Use these labels while logging failures:

- `compile_fail_unexpected`
- `link_fail_unexpected`
- `runtime_crash`
- `runtime_exit_mismatch`
- `runtime_output_mismatch`
- `nondeterministic_output`
- `toolchain_env_issue`
- `harness_issue`

## Promotion Criteria

`line_drawing` is considered stable for binary rollout when:

- clang baseline is green
- full fisics-compiled binary links and passes deterministic lanes
- soak stability is green
- any discovered blockers are fixed or explicitly documented with a scoped
  follow-up plan
- full `make test-binary` in `fisiCs` remains green after fixes

## Final Gate Status

- `LD0`: complete
- `LD1`: complete
- `LD2`: complete
- `LD3`: complete
- `LD4`: complete (used selectively for blocker isolation)
- `LD5`: complete
- `LD6`: complete

## Next External Program Target

- Move to `physics_sim` as the next large external-program binary validation
  lane.
