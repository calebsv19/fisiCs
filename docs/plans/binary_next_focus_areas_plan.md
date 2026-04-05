# Binary Next Focus Areas Plan

Date: 2026-04-03

## Goal

Drive the next binary expansion in a controlled order, one focus area at a
time, with explicit promotion gates between areas.

## Ordered Focus Areas

1. **Area 1 (Completed): stdio/libc parse+format matrix**
   - `strtol` base/autodetect + endptr coverage
   - `strtod` scientific/decimal + endptr coverage
   - `printf/snprintf` width/precision/sign formatting coverage
   - runtime parity with `clang` for each added lane
2. **Area 2 (Completed): deterministic math runtime lane**
   - `isnan` / `isinf` / signed zero handling
   - bounded floating edge behavior with deterministic assertions
   - strict diff-clang parity where UB/impl-defined behavior is excluded
3. **Area 3 (Completed): multi-TU linkage stress lane**
   - cross-TU `extern/static` interactions
   - function-pointer table wiring across translation units
   - additional compile/link negative diagnostics

## One-Area-At-A-Time Execution Rules

For each area:

1. Add new tests in a dedicated wave manifest.
2. Run the wave until green.
3. Add/verify differential wave (when applicable).
4. Re-run bucket lane (`make test-binary-<bucket>`).
5. Re-run full binary lane (`make test-binary`).
6. Update status docs with added waves and current phase marker.
7. Only then start the next area.

## Promotion Gates

A focus area is promoted only when all conditions are met:

- `0 failing` in its direct wave run(s)
- `0 failing` in corresponding diff wave run(s) where required
- `make test-binary` remains green
- no flaky/timeout-only pass behavior

## Current Execution Marker

- Portability follow-up note:
  - `strtoul` on no-digit input (`\"xyz\"`) has libc-specific `errno` behavior.
  - Implemented explicit implementation-defined policy lanes:
    - `binary__runtime__policy_impldef_strtoul_no_digits_errno`
    - `binary__runtime__diff_clang_policy_impldef_strtoul_no_digits_errno`
  - Differential harness now respects `impl_defined: true` and skips
    tri-reference comparison by policy.
- Math builtin follow-up note:
  - Direct `math.h` macro builtin paths are now supported for this lane:
    `__builtin_huge_valf`, `__builtin_huge_val`, `__builtin_huge_vall`,
    `__builtin_nanf`, `__builtin_nan`, `__builtin_nanl`.
  - Added regression coverage:
    - `binary__runtime__math_macro_nan_infinity_builtin_path`
    - `binary__runtime__diff_clang_math_macro_nan_infinity_builtin_path`

- **Area 1 complete**:
  - runtime wave: `binary-stdio-wave5.json`
  - differential wave: `binary-diff-wave16.json`
  - verification:
    - `make test-binary-wave WAVE=5 BINARY_WAVE_BUCKET=binary-stdio` => pass
    - `make test-binary-wave WAVE=16 BINARY_WAVE_BUCKET=binary-diff` => pass
    - `make test-binary-stdio` => pass
    - `make test-binary-diff` => pass
    - `make test-binary` => pass
- **Area 2 complete**:
  - runtime wave: `binary-math-wave1.json`
  - differential wave: `binary-diff-wave17.json`
  - verification:
    - `make test-binary-wave WAVE=1 BINARY_WAVE_BUCKET=binary-math` => pass
    - `make test-binary-wave WAVE=17 BINARY_WAVE_BUCKET=binary-diff` => pass
    - `make test-binary-math` => pass
    - `make test-binary-diff` => pass
    - `make test-binary` => pass
- **Area 3 complete**:
  - runtime/link wave: `binary-linkage-wave1.json`
  - differential wave: `binary-diff-wave18.json`
  - verification:
    - `make test-binary-wave WAVE=1 BINARY_WAVE_BUCKET=binary-linkage` => pass
    - `make test-binary-wave WAVE=18 BINARY_WAVE_BUCKET=binary-diff` => pass
    - `make test-binary-link` => pass
    - `make test-binary-diff` => pass
    - `make test-binary` => pass
- **All three current focus areas are complete**.
- **Post-area fix lanes completed**:
  - builtin macro exposure (`math.h` `NAN`/`INFINITY` path)
  - implementation-defined `strto*` no-digit `errno` policy tagging
- **External target progression update (2026-04-04)**:
  - `line_drawing` binary campaign is complete and marked stable.
  - `line_drawing` now closes with full interactive validation (render + input)
    and green compiler suite (`make final` => `0 failing, 36 skipped`).
  - next target is `physics_sim`.

## Next Program Progression (Post-Datalab)

After confirming `datalab` full compile+run stability, the next external
program target order is:

1. `physics_sim` (next active target)
2. `map_forge`
3. `daw`

Rationale:

- `line_drawing` expands binary surface area (SDL2_ttf + Vulkan + shared libs)
  with manageable complexity and existing deterministic gates.
- `physics_sim` and `map_forge` are larger and carry more subsystem coupling.
- `daw` is the largest/most coupled lane and should remain last in this run
  order.

Execution contract for each program:

1. clang baseline pass
2. full `fisics` compile + clang link pass
3. deterministic headless/runtime parity pass
4. soak stability pass
5. one-TU mix matrix only when a blocker appears
6. targeted fix + regression + revalidation

See `docs/plans/binary_line_drawing_execution_plan.md` for the concrete next
program plan and phase gates.
