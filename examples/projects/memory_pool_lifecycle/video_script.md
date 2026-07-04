# Video Script: Memory Pool Lifecycle

1. Open `fisiCs/examples/projects/memory_pool_lifecycle` in the IDE.
2. Show `src/memory_pool_lifecycle.c` and the explicit acquire/release flow.
3. Run `make examples-project NAME=memory_pool_lifecycle`.
4. Run `make examples-project-memory NAME=memory_pool_lifecycle`.
5. Show `ide_files/memory_report_clean.json` with `active=0`.
6. Show `ide_files/memory_report_leaky.json` with one active allocation.
7. Open `tests/memory/leaky_main.c` and point out the missing release.
8. Use the IDE memory-report surface once available to show the same sidecar.
