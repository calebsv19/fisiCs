# IDE Error Summary

Date: 2026-02-26  
Source dump: `/Users/calebsv/Desktop/CodeWork/ide/errors.md`

## Progress Update (Applied in `fisiCs`)

- Recursive include handling in lenient mode now skips cycles instead of emitting hard errors.
- Recursive include skips in lenient mode are now silent (no warning spam).
- Fixed modifiable-lvalue classification for member assignments like:
  - `opts.include_paths = ...`
  - `opts.macro_defines = ...`
  by correcting top-level pointer `const` handling for member access.

## Snapshot

- Total diagnostics: `107`
- Recursive include diagnostics: `63`
- Diagnostics under `vk_renderer_ref_backup`: `41` lines in dump (multiple files)
- Remaining non-recursive/non-backup diagnostics are concentrated in:
  - `src/core/Terminal/terminal_backend.c`
  - `src/core/Analysis/project_scan.c`
  - `src/core/Analysis/library_index_build.c`
  - `src/ide/Panes/Terminal/terminal.c`

## Root-Cause Clusters

## 1) Header recursion/cycle handling (dominant)

### Symptoms
- `detected recursive include of '/Users/.../ide/Panes/Editor/editor_view.h'`
- Appears across editor, panes, input, watcher, app, and layout files.

### Likely root cause
- True include cycle exists in project headers:
  - `src/ide/Panes/Editor/editor_view.h:6` includes `editor.h`
  - `src/ide/Panes/Editor/editor.h:10` includes `editor_view.h`
- Current preprocessor reports recursion as hard error instead of tolerating guarded re-include behavior after include guards/pragma-once.

### Impact
- This single issue causes most IDE files to fail before semantic analysis.

## 2) Legacy Vulkan backup subtree (large, isolated)

### Symptoms
- Files under:
  - `src/engine/Render/vk_renderer_ref_backup/src/vk_renderer*.c`
- Errors are mostly pointer/argument type incompatibilities and assignment incompatibilities.

### Likely root cause
- Same stale Vulkan backup code pattern seen in other projects (`daw`, etc.) that does not match current API typedef expectations.

### Impact
- Large error volume, but isolated to backup subtree.

## 3) Terminal backend parse/semantic edge

### Symptoms
- `src/core/Terminal/terminal_backend.c`
  - `Bitfield width exceeds type width` (line `0:0`, likely from system include expansion)
  - several `Undeclared identifier` around lines `66-68`

### Likely root cause
- A system-header expansion path is still tripping strict semantic checks (bitfield width rule) and then cascading local identifier errors.
- Could also be affected by missing/mis-seeded builtin macros/functions in this include path.

## 4) Dot-access assignment lvalue regression

### Symptoms
- `src/core/Analysis/project_scan.c:253,255`
- `src/core/Analysis/library_index_build.c:185,187`
- Error: `Left operand of '=' must be a modifiable lvalue`

### Context pattern
- Assignments like:
  - `opts.include_paths = ...`
  - `opts.macro_defines = ...`
  where `opts` is a local struct variable.

### Likely root cause
- Semantic lvalue classification regression for `.` member assignment in specific typed contexts.

## 5) Single residual undeclared in terminal pane

### Symptoms
- `src/ide/Panes/Terminal/terminal.c:30:25` `Undeclared identifier`

### Likely root cause
- Secondary/cascade from prior parser/semantic state corruption in this TU; revisit after clusters 1/3/4 are fixed.

## Recommended Fix Order (One-Wave Plan)

1. Fix recursive-include handling and/or avoid false positives with guarded headers.
2. Decide policy for `vk_renderer_ref_backup`:
   - exclude from IDE analysis/build set, or
   - keep and patch all Vulkan type mismatches.
3. Fix terminal backend system-header semantic path (`bitfield width` + undeclared cascade).
4. Fix struct member lvalue assignment classification (`opts.field = ...`).
5. Re-run IDE scan and handle any true residuals (`terminal.c` etc.).

## Immediate Project-side Header Cleanup (optional but high leverage)

To reduce recursion regardless of compiler behavior:

- Break direct cycle:
  - Remove `#include "ide/Panes/Editor/editor_view.h"` from `editor.h`
  - Keep forward declarations in `editor.h` where possible.
- Keep heavy include graph in `.c` files instead of top-level headers when feasible.
