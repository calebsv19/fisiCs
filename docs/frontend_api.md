# Frontend API (fisics_frontend)

This API exposes the compiler frontend (lex/parse/sema) as a reusable library for IDEs.

## Usage

```c
#include "fisics_frontend.h"

const char* src = "int main(){ return missing; }";
FisicsAnalysisResult res = {0};
if (fisics_analyze_buffer("inline.c", src, strlen(src), &res)) {
    // inspect res.diagnostics / res.tokens / res.symbols
}
fisics_free_analysis_result(&res);
```

- The call allocates a fresh `CompilerContext`, runs the full frontend on the in-memory buffer, and copies results into the `FisicsAnalysisResult`.
- Call `fisics_free_analysis_result` to free owned arrays/strings.

## Result contents
- `diagnostics`: errors/warnings/notes with file/line/column, length, code, and optional hint. Codes are internal and may evolve.
- `tokens`: preprocessed token spans with coarse kinds (identifier, keyword, number, string/char, operator, punct, comment).
- `symbols`: simple top-level symbols (functions/structs/unions/enums) with start/end ranges; name/file strings are duplicated for caller ownership.

## Ownership and lifetime
- All pointers inside `FisicsAnalysisResult` are owned by the caller after a successful call; free them via `fisics_free_analysis_result`.
- Input `source` is not modified or retained.

## Linking
- Build provides `libfisics_frontend.a` and `fisics_frontend.h`. Link with `-L<repo> -lfisics_frontend` and include headers from `src/`.
