# DAW Compiler Error Triage

Date: 2026-02-23  
Project under test: `/Users/calebsv/Desktop/CodeWork/daw`

This document groups the current DAW diagnostics into likely root causes, identifies which ones are primary vs cascade, and defines the fix order in `fisiCs`.

## 1) Primary blocker: `_Atomic` type support is incomplete

### Symptoms
- `include/engine/ringbuf.h:11`, `:12`
  - `expected 'Expected ';' after struct/union field', got 'head'`
  - `expected 'Expected '}' to close struct/union body', got 'head'`
- `include/engine/ringbuf.h:19`, `:20` and `include/audio/audio_queue.h:19`, `:20`
  - `Invalid function parameter list`, `invalid declarator` around `*`
- `include/engine/engine_internal.h:91..131`
  - many typedef/declarator failures (`_Bool`, `char`, `signed`, `unsigned`, etc.)
  - `Expected ';' after typedef` for `atomic_*` aliases

### Likely cause
- Parser currently does not fully support C11 atomic type syntax:
  - qualifier form: `_Atomic size_t head;`
  - specifier form: `_Atomic(type)` (used by `stdatomic.h`, e.g. `typedef _Atomic(_Bool) atomic_bool;`)
- Once `ringbuf.h` fails, downstream prototypes and types misparse, creating many cascaded declarator errors.

### What needs to change
- Extend type parsing (`parseType`/declarator flow) to accept both `_Atomic` forms in declaration specifiers.
- Ensure `_Atomic(T)` is treated as a valid type specifier wrapping a type-name.
- Add focused regressions:
  - struct fields with `_Atomic size_t`
  - `typedef _Atomic(_Bool) atomic_bool;`
  - `atomic_uint_fast64_t` declarations in structs

## 2) Primary/secondary mix: conditional (`?:`) common type inference too strict

### Symptoms
- `src/ui/effects_panel/list_view.c:90`
- `src/ui/effects_panel/spec_panel.c:734`
- `src/ui/effects_panel/slot_view.c:143`, `:220`
- `src/ui/effects_panel/slot_layout.c:133`
- `src/ui/effects_panel/meter_detail_view.c:458`, `:560`
- `src/ui/effects_panel/panel.c:1031`, `:1403`
  - `Incompatible types in ternary expression`
  - plus argument mismatch follow-ons in function calls

### Likely cause
- Semantic type merge for `?:` is rejecting valid C pointer/array decay cases (common patterns in this codebase include:
  - pointer-or-literal branches
  - pointer-or-NULL branches
  - const qualifier merges).
- Some of these may be cascades from prior parse damage, but this category appears independently in UI code and should be treated as a real semantic gap.

### What needs to change
- Relax/correct conditional-expression common-type rules to C behavior:
  - array operands decay before merge
  - null pointer constant merges with object pointers
  - qualifiers merged to composite pointer type.
- Add regressions for representative DAW patterns.

## 3) Mostly cascade: “Conflicting types for function” in effects panel files

### Symptoms
- `src/ui/effects_panel/list_view.c:37`
- `src/ui/effects_panel/spec_panel.c:311`, `:376`, `:481`, `:700`, `:844`
- `src/ui/effects_panel/eq_detail_view.c:426`
- `src/ui/effects_panel/track_snapshot_view.c:235`
- `src/ui/effects_panel/meter_detail_view.c:330`
- `src/ui/effects_panel/panel.c:546`, `:566`, `:573`, `:734`, `:800`, `:868`, `:1006`, `:1386`

### Likely cause
- Most function definitions appear consistent with headers in normal C.
- These are likely downstream artifacts from earlier header/type parsing failures (especially `_Atomic`) and failed type inference in expressions.

### What needs to change
- Re-evaluate after fixing sections 1 and 2.
- Only if they remain, inspect symbol table canonicalization for:
  - `struct X*` vs typedef alias equivalence
  - duplicate declarations across includes.

## 4) Constant expression fallout in enum values

### Symptoms
- `include/engine/engine_internal.h:63..68`
  - `Enumerator value is not a constant expression`

### Likely cause
- Could be a cascade from earlier parse/type breakage in the same translation unit.
- If still present after `_Atomic` support fix, inspect enum constant evaluator for macro-expanded integer expressions in DAW headers.

## Fix order (execution plan)

1. Implement `_Atomic` parsing support (both forms) and add parser/semantic tests.  
2. Re-run DAW diagnostics.  
3. If ternary errors remain, fix `?:` type merge semantics and add tests.  
4. Re-run DAW diagnostics.  
5. Triage remaining “conflicting types” and enum constant-expression issues as residuals.

## Notes
- The `audio_queue.h` and `engine_internal.h` errors are strongly coupled to `ringbuf.h` atomic fields and `stdatomic.h` typedef forms.
- Expect major error-count drop after step 1; many current messages are likely cascades.

## Progress updates

### Completed
- `_Atomic` parser support is implemented (qualifier + `_Atomic(type)` forms) with regression tests.
- Function redeclaration compatibility now canonicalizes typedef aliases in semantic checks.
  - This removed false positives where headers use `struct AppState*` and definitions use `AppState*`.
  - Repro fixed: `void f(struct AppState*);` + `void f(AppState*) {}` no longer reports a conflict.
- Redeclaration matching now also tolerates equivalent struct/union tags when one parsed side lacks a retained type name.
  - This addresses additional false-positive `Conflicting types for function` cases seen in DAW input modules.
- Preprocessor now seeds atomic-order compiler macros used by `stdatomic.h`:
  - `__ATOMIC_RELAXED`, `__ATOMIC_CONSUME`, `__ATOMIC_ACQUIRE`, `__ATOMIC_RELEASE`, `__ATOMIC_ACQ_REL`, `__ATOMIC_SEQ_CST`
  - This fixes enum constant-expression failures from system `memory_order` definitions.
- Ternary pointer merge now decays array operands first and merges qualifiers on compatible pointer targets.
  - This fixes valid cases like `const char* s = cond ? display_name : char_array_field;`.
- Anonymous inline `struct/union` types are now marked complete in semantic type info.
  - This fixes false `sizeof applied to incomplete type` on `sizeof(*ptr_to_anon_struct_member)`.

### Current remaining cluster
- DAW input files still show incompatibilities around fortified string builtins in some runs:
  - `__builtin_object_size` / `__builtin___strncpy_chk` argument typing
  - Follow-on lvalue/index assignment diagnostics on nested aggregate members
- This appears to be a separate semantic gap from the typedef-conflict issue and is the next fix target.

### Additional fixes completed (2026-02-23)
- Lexer string scanning fixed for escaped backslashes before quotes.
  - Root cause: string termination logic treated `\\"`/`\\\\` patterns incorrectly and could consume past the closing quote.
  - Effect: eliminated parser desync cascades (including `session_io_write.c` switch/case parse breakage).
- Include resolver now supports macOS framework header lookup for system includes.
  - Added fallback resolution for `<Framework/Header.h>` to:
    - `$SDKROOT/System/Library/Frameworks/<Framework>.framework/Headers/<Header>`
    - `/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks/...`
    - `/System/Library/Frameworks/...` and `/Library/Frameworks/...`
  - Effect: unblocks `<AudioToolbox/AudioToolbox.h>` style includes used by `src/audio/media_clip.c`.
- Frontend include-path seeding improved for both CLI and frontend API paths.
  - Added common include roots:
    - `/opt/homebrew/include`, `/opt/homebrew/include/SDL2`
    - `/usr/local/include`, `/usr/local/include/SDL2`
    - existing CommandLineTools and `/usr/include` fallbacks remain.
  - API path now merges caller include paths with default/system roots instead of relying only on caller-provided paths.
- Parser now tolerates common interop calling-convention markers as declarator noise:
  - `VKAPI_ATTR`, `VKAPI_CALL`, `VKAPI_PTR`, `APIENTRY`, `WINAPI`, `CALLBACK`, `STDMETHODCALLTYPE`, `__stdcall`, `__cdecl`, `__fastcall`.
  - Effect: prevents declaration parsing from desyncing when these macros appear unresolved/expanded in Vulkan-style headers.
- Preprocessor token teardown now guards frees for non-heap token spellings on macOS.
  - Root cause: token pasting re-lex path can carry lexer-provided punctuation spellings (non-owned/static text), and unconditional `free(tok->value)` caused ASan aborts.
  - Effect: fixed deterministic crash while analyzing `daw/src/audio/media_clip.c`; full sanitized DAW sweep now completes (`160` `.c` files) without abort.
- Added fallback builtin symbols for additional macOS libc/atomic mappings used when system headers are partially unavailable:
  - `vsnprintf`, `__builtin___vsnprintf_chk`, `strcat`, `__builtin___strcat_chk`
  - `atomic_exchange_explicit`, `atomic_init`, `__c11_atomic_exchange`, `__c11_atomic_init`
  - Effect: reduces persistent `Undeclared identifier` clusters in DAW engine/session code paths that rely on fortified or `__c11_*` lowered names.
