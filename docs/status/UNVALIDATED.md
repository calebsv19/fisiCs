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
  - bucket is now active (Wave2 promoted),
  - still missing deeper negative matrices for conversions/comparisons,
  - still missing diagnostic-json parity lanes for semantic rejects.
- Runtime policy:
  - explicit signed-overflow UB policy lane still listed as unstarted in the checklist.
- Torture infrastructure:
  - explicit high-volume fuzz-harness lane remains marked unstarted.

====== Recommended next execution order ======
1. Continue semantic bucket (`07`) with Wave3 conversion-negative and diagjson parity lanes.
2. Close lexer boundary leftovers (`02`) so token/literal policy is complete.
3. Add statement return diagnostics (`09`) with explicit warning/error policy.
4. Add torture harness infra lane (`15`) for bounded high-volume fuzz replay shards.

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

====== Diagnostics ======
- Consistent hint/note emission for common parse/semantic failures (beyond undeclared identifiers).

====== External project validations ======
- Broader compilation of external codebases with nested includes and macros.
- System header parsing with SDKs beyond macOS (if relevant).
