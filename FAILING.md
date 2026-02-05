# Failing or Suspicious Behavior (make run, include/test.txt)

====== External project validations (in progress) ======
- [PASS] `/Users/calebsv/Desktop/CodeWork/test/src/core/string_view.c` compiles clean (no Error/Warning output).
- [FAIL] `/Users/calebsv/Desktop/CodeWork/map_forge/src/map/tile_math.c` still crashes during object emission (LLVM fatal / ASan SEGV in codegen passes) when compiling with `./fisics -c -I/Users/calebsv/Desktop/CodeWork/map_forge/include`.
- [PARTIAL] Other external codebases not exercised in this pass.
