# Agent Task: Population Competition

Task: add a small resource recovery amount after each tick while keeping all
reported values deterministic and nonnegative.

Allowed files:

- `src/main.c`
- `src/population_competition.c`
- `src/population_competition.h`
- `tests/expected_stdout.txt`

Forbidden scope:

- do not add random behavior
- do not remove nonnegative clamping
- do not edit compiler source

Required checks:

```bash
cd /path/to/fisiCs
make examples-project NAME=population_competition
make examples-project-artifacts NAME=population_competition
```

Acceptance:

- output remains deterministic
- populations and resource never go below zero
- build graph artifact still writes under `ide_files/`
