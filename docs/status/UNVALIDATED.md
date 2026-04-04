# Unvalidated Behavior (needs targeted `make run` checks)

These items are not yet explicitly validated via `include/test.txt`/`make run` and should be exercised with focused snippets.

====== Final suite bucket focus ======
- Bucket `14` runtime-surface is currently stable:
  `make final-runtime` => `0 failing, 22 skipped`,
  and `PROBE_FILTER=14__probe_*` => `resolved=234, blocked=0, skipped=0`.
- Bucket `15` torture/differential wave campaign is complete at current baseline.
- Bucket `15` baseline:
  `145` active tests (`81` runtime differential lanes, `35` diagnostics lanes,
  `27` diagjson lanes), and
  `PROBE_FILTER=15__probe_*` => `resolved=136, blocked=0, skipped=0`.
- Full final snapshot (2026-04-01):
  `make final` => `0 failing, 32 skipped`.
- Plan reference for next work:
  `docs/plans/torture_bucket_15_execution_plan.md`.
- Bucket-15 closure waves through Wave40 are now promoted.
- Next planned expansion set: none (Wave40 campaign complete).
- Bucket-15 unresolved stress areas (by design, not blockers):
  - tri-reference gcc coverage remains a subset of total runtime differential lanes,
  - optional future expansion can add additional pinned external-corpus fragments beyond wave40.

====== Binary lane focus ======
- Binary lane is stable through ABI wave 19, corpus wave 9, diff wave 19, SDL wave 13, math wave 1, and linkage wave 1:
  - `make test-binary-abi` => pass
  - `make test-binary-corpus` => pass
  - `make test-binary-link` => pass
  - `make test-binary-diff` => pass
  - `make test-binary-math` => pass
  - `make test-binary-sdl` => pass
  - `make test-binary-wave WAVE=19 BINARY_WAVE_BUCKET=binary-abi` => pass
  - `make test-binary-wave WAVE=15 BINARY_WAVE_BUCKET=binary-diff` => pass
  - `make test-binary-wave WAVE=16 BINARY_WAVE_BUCKET=binary-diff` => pass
  - `make test-binary-wave WAVE=17 BINARY_WAVE_BUCKET=binary-diff` => pass
  - `make test-binary-wave WAVE=18 BINARY_WAVE_BUCKET=binary-diff` => pass
  - `make test-binary-wave WAVE=1 BINARY_WAVE_BUCKET=binary-sdl` => pass
  - `make test-binary-wave WAVE=2 BINARY_WAVE_BUCKET=binary-sdl` => pass
  - `make test-binary-wave WAVE=3 BINARY_WAVE_BUCKET=binary-sdl` => pass
  - `make test-binary-wave WAVE=4 BINARY_WAVE_BUCKET=binary-sdl` => pass
  - `make test-binary-wave WAVE=5 BINARY_WAVE_BUCKET=binary-sdl` => pass (`0 failing, 2 skipped`)
  - `make test-binary-wave WAVE=6 BINARY_WAVE_BUCKET=binary-sdl` => pass (`0 failing, 2 skipped` by default; opt-in run validated with `ENABLE_SDL_RENDERER_TESTS=1`)
  - `make test-binary-wave WAVE=7 BINARY_WAVE_BUCKET=binary-sdl` => pass
  - `make test-binary-wave WAVE=8 BINARY_WAVE_BUCKET=binary-sdl` => pass
  - `make test-binary-wave WAVE=9 BINARY_WAVE_BUCKET=binary-sdl` => pass
  - `make test-binary-wave WAVE=10 BINARY_WAVE_BUCKET=binary-sdl` => pass
  - `make test-binary-wave WAVE=11 BINARY_WAVE_BUCKET=binary-sdl` => pass
  - `make test-binary-wave WAVE=12 BINARY_WAVE_BUCKET=binary-sdl` => pass
  - `make test-binary-wave WAVE=13 BINARY_WAVE_BUCKET=binary-sdl` => pass
  - `make test-binary-wave WAVE=1 BINARY_WAVE_BUCKET=binary-math` => pass
  - `make test-binary-wave WAVE=1 BINARY_WAVE_BUCKET=binary-linkage` => pass
  - `make test-binary` => pass
  - active inventory: `244` tests (`runtime=184`, `compile_only=19`, `compile_fail=15`, `link_fail=12`, `link_only=14`)
  - differential parity audit:
    - baseline runtime set `68`, effective diff-mapped coverage `68`, remaining `0`
    - legacy naming drift closed (`abi_fnptr_dispatch` diff ID aligned)
  - SDL toolchain readiness:
    - `sdl2-config` and `pkg-config` are present locally (`SDL2 2.32.10`)
    - E1 compile/link SDL lane is active (`level: sdl`, wave 1)
    - E2 headless runtime SDL lane is active (`level: sdl`, wave 2)
    - E3 bounded runtime SDL lane is active (`level: sdl`, wave 3)
    - E4 differential runtime SDL lane is active (`level: sdl`, wave 4)
    - E4 policy-skip SDL lane is active (`level: sdl`, wave 5)
    - E5 renderer-gated SDL lane is active (`level: sdl`, wave 6; env-gated)
    - E5 expanded SDL differential lane is active (`level: sdl`, wave 7)
    - E5 SDL negative compile/link lane is active (`level: sdl`, wave 8)
    - E5 SDL dummy-audio lane is active (`level: sdl`, wave 9)
    - E6 SDL deterministic runtime expansion is active (`level: sdl`, wave 10)
    - E6 SDL deterministic diff-clang expansion is active (`level: sdl`, wave 11)
    - E6b SDL deterministic runtime expansion is active (`level: sdl`, wave 12)
    - E6b SDL deterministic diff-clang expansion is active (`level: sdl`, wave 13)
    - direct SDL system-header parsing is active in wave 1 (`binary__link_only__sdl2_header_real_smoke`, `binary__link_only__sdl2_stdinc_inline_smoke`)
- Current phase marker:
  - Phase E6/E6b in progress (Levels 0-4 stable, corpus wave-9 active, diff wave-19 full parity, SDL waves 1-13 active, math wave-1 active, linkage wave-1 active, with wave-6 env-gated renderer checks).
- Recent binary fix:
  - `ptr_to_agg` call-shape compile hang is resolved.
  - `binary__compile_fail__abi_ptr_to_agg_timeout` now runs as a normal
    compile-fail diagnostic expectation (no timeout sentinel behavior).

====== Explicit checklist gaps (remaining) ======
- Lexer literal boundaries:
  - large integer boundary constants,
  - float precision-boundary matrix,
  - multi-character constant coverage,
  - escape overflow diagnostics.
- Statements/control diagnostics:
  - missing-return policy lane,
  - wrong-return-type statement lane.
- Semantic phase (`07`) coverage:
  - bucket is active through Wave17 (`100` tests total),
  - all current 07 probe lanes are resolved (`resolved=91, blocked=0`),
  - remaining work is optional depth extension (not a blocker for current baseline).
- Runtime policy:
  - explicit signed-overflow UB policy lane still listed as unstarted in the checklist.
- Torture infrastructure:
  - explicit high-volume fuzz-harness lane remains marked unstarted.

====== Recommended next execution order ======
1. Close lexer boundary leftovers (`02`) so token/literal policy is complete.
2. Add statement return diagnostics (`09`) with explicit warning/error policy.
3. Add torture harness infra lane (`15`) for bounded high-volume fuzz replay shards.
4. Optionally extend semantic bucket (`07`) with Wave18+ depth-only lanes.

====== Preprocessor ======
- Macro self-expansion suppression and re-scan behavior.
- Empty `__VA_ARGS__` in variadic macros without GNU extension reliance.
- `#pragma once` behavior if supported.
- `__FILE__`, `__LINE__`, `__DATE__`, `__TIME__` expansions.
- `#include` of macro-generated `<...>` vs `"..."` forms.

====== Declarations, types, and qualifiers ======
- `restrict` semantics and propagation (if diagnostics are planned).
- Bitfield signedness edge cases (beyond width checks).

====== Initializers and object layout ======
- Struct/union padding/offset checks against target layout.

====== Scoping, linkage, and redeclarations ======
- Tentative definitions across multiple translation units (if tracked).
- `static` vs `extern` mismatches across separate declarations in headers.

====== Functions and calls ======
- Variadic function argument type checks vs default promotions.

====== Codegen and lowering ======
- VLA stack allocation/deallocation strategy and lifetime (beyond simple `alloca`).

====== Runtime surface and builtins ======
- Coverage for additional system headers (e.g., `stdarg.h`, `stddef.h` edge cases).
- Builtin stubs for platform headers beyond current secure-check intrinsics.
- Manual SDL crash lane (tracked for compiler/runtime hardening):
  - Automated guard is now present in binary SDL wave 14 (`binary__runtime__sdl2_renderer_drawline_clip_readback` + clang diff twin), but high-density visual playground stress remains an open manual lane for macOS-specific renderer edge behavior.

====== Diagnostics ======
- Consistent hint/note emission for common parse/semantic failures (beyond undeclared identifiers).

====== External project validations ======
- Broader compilation of external codebases with nested includes and macros.
- System header parsing with SDKs beyond macOS (if relevant).
