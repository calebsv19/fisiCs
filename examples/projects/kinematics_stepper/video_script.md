# Video Script: Kinematics Stepper

1. Open `fisiCs/examples/projects/kinematics_stepper` in the IDE.
2. Show the valid source with `length`, `speed`, `acceleration`, `time`, `mass`,
   and `energy` annotations.
3. Run `make examples-project NAME=kinematics_stepper`.
4. Show the deterministic motion table.
5. Open `tests/invalid/bad_position_update.c`.
6. Run `make examples-project-invalid NAME=kinematics_stepper`.
7. Point out the units addition mismatch and skipped code generation.
8. Generate artifacts with `make examples-project-artifacts NAME=kinematics_stepper`.
9. Use the IDE units/diagnostic views to show how the project exposes compiler
   analysis data.
