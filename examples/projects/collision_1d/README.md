# Collision 1D

This curated example models a one-dimensional collision with mass, velocity,
momentum, and kinetic energy annotations.

The valid run computes post-collision velocities for two bodies. The invalid
fixture intentionally mixes incompatible dimensions so the physics-units
overlay can reject the bug before code generation.

## Run

```bash
cd /path/to/fisiCs
make examples-project NAME=collision_1d
```

Expected output:

```text
body velocity_mps momentum energy_j
A -1.000 -2.000 1.000
B 3.000 3.000 4.500
```

## Run Intentional Units Failure

```bash
cd /path/to/fisiCs
make examples-project-invalid NAME=collision_1d
```

## Generate IDE Artifacts

```bash
cd /path/to/fisiCs
make examples-project-artifacts NAME=collision_1d
```

## Demo Purpose

- show a compact physics model with units annotations
- catch a meaningful unit/dimension mismatch in a collision-like calculation
- provide a small IDE-openable project for future visual or game-style demos
