# Release Example Pack

This is the smallest public example path for checking that `fisiCs` is
coherent as a CLI compiler release and as an agent-operable repository.

Run all commands from the repository root:

```bash
git clone https://github.com/calebsv19/fisiCs.git
cd fisiCs
```

If you already have a source checkout, run from that checkout:

```bash
cd /path/to/fisiCs
```

## What This Pack Proves

- the compiler builds and can run a hello-world program
- single-file and multi-translation-unit compile/link smoke paths work
- practical canaries cover libc/string and numeric/math behavior
- the physics-units overlay has a small valid example
- a curated project can be operated by an agent through documented commands,
  expected output, intentional diagnostics, and generated build-graph artifacts

## 1. Build And Run Hello World

```bash
make
./fisics examples/hello_world.c -o build/examples/hello_world
./build/examples/hello_world
```

Expected output:

```text
Hello from fisiCs.
```

## 2. Run Compile/Link Smoke

```bash
./compilation/run_single.sh ./fisics
./compilation/run_multi.sh ./fisics
```

Expected multi-TU output:

```text
multi-file smoke: helper=7 total=12
```

## 3. Run Practical Canaries

```bash
make examples-canaries
```

Expected output includes:

```text
canary multitu: base=14 adjusted=17 label=ok
canary libc-string: fields=3 mass=12 velocity=34 tag=OK42 checksum=256
canary numeric-math: exp=6 scaled=48 rem=1 quotient=8 sign=1 max=48
```

## 4. Run The Physics-Units Pilot

```bash
make examples-physics-units
./build/examples/ballistics_valid
```

To inspect the units metadata:

```bash
./fisics --overlay=physics-units --dump-sema -c examples/physics_units/ballistics_valid.c -o build/examples/ballistics_valid.o
```

## 5. Run The Canonical Curated Agent Demo

Use `kinematics_stepper` as the first release agent demo. It is small, has
deterministic output, includes an intentional invalid units fixture, and can
write build-graph artifacts for tooling/IDE inspection.

```bash
make examples-project NAME=kinematics_stepper
make examples-project-invalid NAME=kinematics_stepper
make examples-project-artifacts NAME=kinematics_stepper
```

Read the bounded agent task before changing the project:

- `examples/projects/kinematics_stepper/agent_task.md`

Expected valid output:

```text
step position_m velocity_mps kinetic_j
1 1.702 17.019 181.029
2 3.306 16.038 160.761
3 4.811 15.057 141.696
```

Expected invalid-fixture behavior:

- the invalid fixture is rejected
- stderr contains `units addition mismatch`
- code generation is skipped after semantic errors

Generated tooling artifact:

- `examples/projects/kinematics_stepper/ide_files/build_graph.json`

## Optional: Memory-Check Demo

Use this only when the release story needs runtime allocation/free diagnostics:

```bash
make examples-memory-check
make examples-project-memory NAME=memory_pool_lifecycle
```

The memory-check lane is useful, but it is not required for the smallest
release example pack.

## Next References

- first-user path: `docs/first_user_path.md`
- agent guide: `AGENTS.md`
- examples index: `examples/README.md`
- curated project index: `examples/projects/README.md`
- release-confidence checklist: `docs/release_confidence_checklist.md`
