# Unvalidated Behavior (needs targeted `make run` checks)

These items are not yet explicitly validated via `include/test.txt`/`make run` and should be exercised with focused snippets.

====== Final suite (bucket 14) next validation targets ======
- Previous ABI/call probe blockers were fixed and promoted:
  `14__runtime_float_cast_roundtrip`,
  `14__runtime_variadic_promotions_matrix`,
  `14__runtime_struct_with_array_pass_return`,
  `14__runtime_union_payload_roundtrip`,
  `14__runtime_fnptr_dispatch_table_mixed`.
- VLA stride/index runtime behavior and deeper alignment stress.
- Nested switch/fallthrough loop control-flow stress.
- Plan reference: `docs/plans/runtime_bucket_14_execution_plan.md`.

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
