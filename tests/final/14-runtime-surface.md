# Runtime / Library Surface

This bucket is the stable runtime-truth lane inside `tests/final/`.

It exists to catch the class of compiler bugs that are most dangerous in normal
use: code that compiles but behaves incorrectly at runtime.

## Scope

Use bucket `14` for:

- runtime execution correctness
- ABI-sensitive call, return, and layout behavior
- minimal header and builtin surface needed for practical C program work
- library-facing runtime behavior that belongs in the canonical bucket suite
- multi-translation-unit runtime cases when the main truth signal is produced
  program behavior rather than only link success

## Current Use

- focused runtime bucket run: `make final-bucket BUCKET=runtime-surface`
- broader runtime slice: `make final-runtime`
- exact id: `make final-id ID=<14__test_id>`
- broader checkpoint: `make final`

Use `tests/binary/` when compile/link/runtime artifact handling is the clearest
oracle. Use `tests/real_projects/` when validating practical readiness on full
programs. Use `tests/final/probes/` only for blocked or not-yet-promotable
runtime repros.

## Acceptance Checklist

- produced executables run and match expected stdout, stderr, and exit status
- wrong-code is treated as a top-priority failure
- ABI-sensitive call and return behavior remains stable
- pointer, aggregate, and layout-dependent runtime behavior stays correct
- supported minimal header surfaces compile and behave as expected
- multi-TU runtime cases stay deterministic where this bucket owns them
- UB and implementation-defined policy lanes are tagged explicitly rather than
  silently treated as ordinary differential checks

## Representative Coverage Areas

- minimal header surface such as `stdbool.h`, `stddef.h`, `stdint.h`, and
  `limits.h`
- `setjmp.h` / `longjmp` control-transfer behavior for bounded direct and
  helper-mediated runtime paths
- `stdarg.h` variadic argument handling for default promotions and `va_copy`
  forwarding across deterministic `va_list` consumers
- `iso646.h` alternative-token behavior for boolean, bitwise, and compound
  assignment operators
- `limits.h` signed/unsigned range and implementation-width runtime contracts
- `stdint.h` exact-width, least/fast-width, pointer-width, and max-width
  integer runtime contracts
- `stdbool.h` bool macro, conversion, and aggregate-storage runtime contracts
- `stddef.h` `NULL`, `size_t`, `ptrdiff_t`, and nested `offsetof` runtime
  contracts
- `string.h` memory copy/compare and search/span runtime contracts
- `ctype.h` classification and case-conversion runtime contracts
- `stdio.h` formatting/scanning, width/precision, scan length modifiers, stream
  state, and file-position runtime contracts
- `stdlib.h` allocation, numeric-conversion, `strto*` base/end-pointer, and
  `qsort` / `bsearch` runtime contracts
- `errno.h` macro/lvalue and libc error-state runtime contracts
- `assert.h` enabled-assert and `NDEBUG` disabled-assert runtime contracts
- `float.h` precision and range macro runtime contracts
- `math.h` finite-classification, relation-macro, rounding, `nextafter`, and
  `fma` runtime contracts
- `complex.h` absolute-value, argument, projection, conjugation, finite
  transcendental, power, and root runtime contracts
- `locale.h` / `time.h` C-locale formatting and calendar normalization
  runtime contracts
- `signal.h` handler sequencing and `sig_atomic_t` state runtime contracts
- `inttypes.h` base formatting/scanning runtime contracts
- `wchar.h` / `wctype.h` wide string/span, C-locale multibyte conversion, and
  descriptor/classification runtime contracts
- `fenv.h` environment save/update, roundtrip, exception-flag sequencing, and
  exception-flag restore runtime contracts
- `stdatomic.h` `atomic_flag` sequence/array-state, bool/char/uintptr storage,
  weak compare-exchange/fence, compare-exchange, and fetch read-modify-write
  runtime contracts
- `tgmath.h` real type-generic math dispatch and bounded complex `sqrt` /
  `fabs` dispatch runtime contracts
- `offsetof`, fixed-width typedefs, and key macro contracts
- direct calls, recursion, function pointers, and variadic call surfaces
- pointer arithmetic, pointer-difference, and array/aggregate access
- struct/union layout, by-value passing, and return behavior
- switch/loop/runtime control-flow correctness
- multi-TU runtime coordination, callback, relocation, and state-sharing cases
- policy-tagged undefined-behavior and implementation-defined runtime lanes

## Related Lanes

- `tests/binary/`: compile/link/runtime artifact truth and broader differential
  binary checks
- `tests/real_projects/`: canary validation on full programs
- bucket `10`: linkage ownership when the main question is symbol/link behavior
- bucket `13`: lowering/IR shape when IR is the clearest oracle
- bucket `15`: stress and torture surfaces when the main value is pressure,
  pathology, or differential breadth

## Public Note

This file is intentionally a stable public bucket reference.

Wave-by-wave expansion history, probe counts over time, and active expansion
sequencing are maintainer material and are intentionally kept out of this public
summary.
