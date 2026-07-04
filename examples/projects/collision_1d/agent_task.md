# Agent Task: Collision 1D

Task: change the initial velocities and update the expected output while
preserving dimensional correctness.

Allowed files:

- `src/main.c`
- `src/collision_1d.c`
- `src/collision_1d.h`
- `tests/expected_stdout.txt`

Forbidden scope:

- do not remove physics-units annotations
- do not weaken the invalid fixture
- do not edit compiler source

Required checks:

```bash
cd /path/to/fisiCs
make examples-project NAME=collision_1d
make examples-project-invalid NAME=collision_1d
make examples-project-artifacts NAME=collision_1d
```

Acceptance:

- valid output remains deterministic
- invalid fixture still fails with a units mismatch
- build graph artifact still writes under `ide_files/`
