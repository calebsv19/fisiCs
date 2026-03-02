# Failing or Suspicious Behavior (make run, include/test.txt)

====== External project validations (in progress) ======
- [PASS] `/Users/calebsv/Desktop/CodeWork/test/src/core/string_view.c` compiles clean (no Error/Warning output).
- [FAIL] `/Users/calebsv/Desktop/CodeWork/map_forge/src/map/tile_math.c` still crashes during object emission (LLVM fatal / ASan SEGV in codegen passes) when compiling with `./fisics -c -I/Users/calebsv/Desktop/CodeWork/map_forge/include`.
- [FAIL] `/Users/calebsv/Desktop/CodeWork/daw` batch compile (`-c` + `-I.../daw/include` + `-I.../daw/src`) => 160 files: 1 pass / 159 fail. Common issues: missing system headers (`SDL2/SDL.h`), missing project headers (`vk_renderer_sdl.h`), and conflicting anonymous struct/enum definitions.
- [FAIL] `/Users/calebsv/Desktop/CodeWork/ide` batch compile (`-c` + `-I.../ide/include` + `-I.../ide/src`) => 135 files: 3 pass / 132 fail. Common issues: missing `SDL2` headers, missing `vk_renderer_sdl.h`, conflicting anonymous enums, and parser errors in render files.
- [FAIL] `/Users/calebsv/Desktop/CodeWork/line_drawing` batch compile (`-c` + `-I.../line_drawing/include` + `-I.../line_drawing/src`) => 36 files: 2 pass / 34 fail. Common issues: missing `SDL2` headers, missing `vk_renderer.h`, and an LLVM codegen crash in `tests/test_math.c`.
- [FAIL] `line_drawing` hybrid C17 sweep (`FISICS_DIALECT=c17 FISICS_EXTENSIONS=1 FISICS_PREPROCESS=hybrid`) => 31 files: 0 pass / 30 fail / 1 timeout. Common issues: SDL/SDL_ttf system header extensions still triggering parser errors (attribute-heavy enum members/declarations, complex macro expansions), plus one hang in `src/UI/ui_panel.c`. `_Static_assert`, `__builtin_*`, and `__func__` are now handled via include/test snippets. Logs: `/tmp/line_drawing_hybrid_pp_c17`.
- [PARTIAL] Other external codebases not exercised in this pass.

====== Test suite regressions (internal preprocessor path) ======
- [PASS] `make tests` and `make final` pass after hex-float lexing fix, parser recovery leniency adjustment, and golden updates for anonymous tag naming.
