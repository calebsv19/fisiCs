# Frontend API (fisics_frontend)

This API exposes the compiler frontend (lex/parse/sema) as a reusable library for IDEs.

The versioned boundary and compatibility policy are defined in:

- `docs/compiler_ide_data_contract.md`

## Usage

```c
#include "fisics_frontend.h"

const char* src = "int main(){ return missing; }";
FisicsAnalysisResult res = {0};
FisicsFrontendOptions opts = {
    .include_paths = NULL, // or an array of -I paths
    .include_path_count = 0,
    .macro_defines = NULL, // e.g., (const char*[]){"DEBUG=1","PLATFORM_MAC"}
    .macro_define_count = 0,
    .overlay_features = FISICS_OVERLAY_NONE, // or FISICS_OVERLAY_PHYSICS_UNITS
};
if (fisics_analyze_buffer("inline.c", src, strlen(src), &opts, &res)) {
    // inspect res.diagnostics / res.tokens / res.symbols
}
fisics_free_analysis_result(&res);
```

- The call allocates a fresh `CompilerContext`, runs the full frontend on the in-memory buffer, and copies results into the `FisicsAnalysisResult`.
- Call `fisics_free_analysis_result` to free owned arrays/strings.

## Result contents
- `diagnostics`: errors/warnings/notes with file/line/column, length, code, and optional hint. Additive taxonomy fields are also provided in current v1 lanes: `severity_id`, `category_id`, and `code_id`. Code values follow the stable v1 diagnostic code lane in `Compiler/diagnostics.h`.
- `tokens`: preprocessed token spans with coarse kinds (identifier, keyword, number, string/char, operator, punct, comment).
- `symbols`: top-level and nested symbol metadata with start/end ranges, ownership, deterministic `stable_id` values, and optional ownership link identity via `parent_stable_id` when available.
- `includes`: include usage edges with resolved/unresolved origin info.
- `contract`: metadata describing contract id/version, producer, mode (`strict`/`lenient`), partial/fatal flags, trust fields, and additive capability flags (`capabilities`) for optional consumer lanes.
- `units_attachments`: optional declaration/symbol units metadata exported only when the producer advertises `FISICS_CONTRACT_CAP_EXTENSION_UNITS_ATTACHMENTS`.

## Overlay Options

- `overlay_features` uses the same feature family as the CLI `--overlay=` flag.
- Current public flags include:
  - `FISICS_OVERLAY_NONE`
  - `FISICS_OVERLAY_IDE_METADATA`
  - `FISICS_OVERLAY_PHYSICS_UNITS`
- Overlay behavior is opt-in; callers that leave this field at `FISICS_OVERLAY_NONE` stay on the core-C analysis path.

For the current overlay model and `[[fisics::dim(...)]]` syntax, see:

- `docs/extension_overlays.md`

## Ownership and lifetime
- All pointers inside `FisicsAnalysisResult` are owned by the caller after a successful call; free them via `fisics_free_analysis_result`.
- Input `source` is not modified or retained.

## Linking
- Build provides `libfisics_frontend.a` and `fisics_frontend.h`. Link with `-L<repo> -lfisics_frontend` and include headers from `src/`.
- IDE integration note:
  - Some consumers (for example `../ide`) link profile-specific archives:
    - `libfisics_frontend_unsanitized.a`
    - `libfisics_frontend_sanitized.a`
  - Run `make frontend-rebuild` in `fisiCs` to refresh both archives and update `libfisics_frontend.a` alias.
