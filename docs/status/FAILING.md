# Failing or Suspicious Behavior (make run, include/test.txt)

====== External project validations (in progress) ======
- [PASS] `/Users/calebsv/Desktop/CodeWork/test/src/core/string_view.c` compiles clean (no Error/Warning output).
- [FAIL] `/Users/calebsv/Desktop/CodeWork/map_forge/src/map/tile_math.c` still crashes during object emission (LLVM fatal / ASan SEGV in codegen passes) when compiling with `./fisics -c -I/Users/calebsv/Desktop/CodeWork/map_forge/include`.
- [FAIL] `/Users/calebsv/Desktop/CodeWork/daw` batch compile (`-c` + `-I.../daw/include` + `-I.../daw/src`) => 160 files: 1 pass / 159 fail. Common issues: missing system headers (`SDL2/SDL.h`), missing project headers (`vk_renderer_sdl.h`), and conflicting anonymous struct/enum definitions.
- [FAIL] `/Users/calebsv/Desktop/CodeWork/ide` batch compile (`-c` + `-I.../ide/include` + `-I.../ide/src`) => 135 files: 3 pass / 132 fail. Common issues: missing `SDL2` headers, missing `vk_renderer_sdl.h`, conflicting anonymous enums, and parser errors in render files.
- [PARTIAL] Other external codebases not exercised in this pass.

====== Test suite regressions (internal preprocessor path) ======
- [PASS] `make tests` and `make final` pass (`make final` current snapshot: `0 failing, 36 skipped`).

====== Recently resolved (moved out of failing state) ======
- [PASS] `line_drawing` is now stable for the validated binary lane:
  - compile/link with `fisics`-compiled objects succeeded
  - interactive runtime validation succeeded (window/render/input flow)
  - tracked under `docs/plans/binary_line_drawing_execution_plan.md`
