# Agent Task: Compound Growth

Task: add an optional monthly bonus contribution after tick 2.

Allowed files:

- `src/compound_growth.c`
- `src/compound_growth.h`
- `src/main.c`
- `tests/expected_stdout.txt`

Forbidden scope:

- do not change the runner script
- do not change compiler source
- do not add external dependencies

Required checks:

```bash
cd /path/to/fisiCs
make examples-project NAME=compound_growth
make examples-project-artifacts NAME=compound_growth
```

Acceptance:

- output remains deterministic
- expected stdout is updated only if the behavior intentionally changes
- project still writes `ide_files/build_graph.json`
