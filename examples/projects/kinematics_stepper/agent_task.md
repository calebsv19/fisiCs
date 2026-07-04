# Agent Task: Kinematics Stepper

Task: add one more simulation step while preserving units correctness.

Allowed files:

- `src/main.c`
- `src/kinematics_stepper.c`
- `src/kinematics_stepper.h`
- `tests/expected_stdout.txt`

Forbidden scope:

- do not remove physics-units annotations
- do not edit compiler source
- do not weaken the invalid fixture

Required checks:

```bash
cd /path/to/fisiCs
make examples-project NAME=kinematics_stepper
make examples-project-invalid NAME=kinematics_stepper
make examples-project-artifacts NAME=kinematics_stepper
```

Acceptance:

- valid output remains deterministic
- invalid units fixture still fails
- generated build graph still writes under `ide_files/`
