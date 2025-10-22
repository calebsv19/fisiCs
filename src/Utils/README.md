# Utils

Lightweight helpers shared across the pipeline.

- `utils.h` / `utils.c`
  - `readFile(const char* path)` slurps an input file into a null-terminated buffer (caller owns the memory).
  - `DEBUG_PRINT_SOURCE` toggle gates a convenience diagnostic that echoes the loaded buffer to stdout, useful while iterating on the lexer.

No other utilities live here yet; new helpers should stay small and dependency-free so they can be reused by every phase.
