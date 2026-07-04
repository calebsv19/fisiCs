# Agent Task: Memory Pool Lifecycle

Task: add one more event slot to the lifecycle while keeping the memory report
balanced.

Allowed files:

- `src/main.c`
- `src/memory_pool_lifecycle.c`
- `src/memory_pool_lifecycle.h`
- `tests/expected_stdout.txt`
- `tests/memory/expected_clean_report.json`

Forbidden scope:

- do not remove the memory-check report call
- do not weaken or delete the leaky fixture
- do not edit compiler or runtime source

Required checks:

```bash
cd /path/to/fisiCs
make examples-project NAME=memory_pool_lifecycle
make examples-project-memory NAME=memory_pool_lifecycle
make examples-project-artifacts NAME=memory_pool_lifecycle
```

Acceptance:

- clean report keeps `active=0`
- leaky report still shows exactly one active allocation
- generated memory reports remain under `ide_files/`
