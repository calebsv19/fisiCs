# Kinematics Stepper

This curated example is a tiny physics-units project for time-step motion.

It updates position and velocity under acceleration, computes kinetic energy,
and includes an intentional invalid fixture for a dimension mismatch.

## Run

```bash
cd /path/to/fisiCs
make examples-project NAME=kinematics_stepper
```

Expected output:

```text
step position_m velocity_mps kinetic_j
1 1.702 17.019 181.029
2 3.306 16.038 160.761
3 4.811 15.057 141.696
```

## Run Intentional Units Failure

```bash
cd /path/to/fisiCs
make examples-project-invalid NAME=kinematics_stepper
```

The invalid fixture tries to add acceleration directly to position. The
physics-units overlay should reject it before code generation.

## Generate IDE Artifacts

```bash
cd /path/to/fisiCs
make examples-project-artifacts NAME=kinematics_stepper
```

Generated artifacts are written under `build/` and `ide_files/`.

## Demo Purpose

- project-shaped physics-units example
- deterministic valid output
- deterministic invalid units diagnostic
- source root suitable for IDE units/search/detail testing
