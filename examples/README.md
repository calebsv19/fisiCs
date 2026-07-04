# Examples

This directory contains minimal runnable examples for `fisiCs`.

## Files

- `hello_world.c`: basic stdout example
- `canaries/`: practical public canaries for multi-TU compile/link,
  libc/string parsing, and numeric/math behavior
- `memory_check/`: explicitly opt-in leak demo for the memory-check overlay
- `projects/`: small IDE-openable curated demo projects with scripts, expected
  output, video outlines, and agent task prompts
- `release_example_pack.md`: smallest release-readiness demo path for humans
  and agents
- `sdl_window_loop.c`: SDL window with event loop and animated clear color
- `physics_units/`: public pilot lane for the physics-units overlay

## Release Example Pack

Use this first when checking the public release story:

- [release_example_pack.md](./release_example_pack.md)

It covers hello world, compile/link smoke, practical canaries, the
physics-units pilot, and the canonical curated agent demo
`kinematics_stepper`.

## Build Examples And Practical Canaries

```bash
cd /path/to/fisiCs
make examples
```

## Run Practical Canaries

```bash
cd /path/to/fisiCs
make examples-canaries
```

Expected output includes:

```text
canary multitu: base=14 adjusted=17 label=ok
canary libc-string: fields=3 mass=12 velocity=34 tag=OK42 checksum=256
canary numeric-math: exp=6 scaled=48 rem=1 quotient=8 sign=1 max=48
```

For more detail, see [canaries/README.md](./canaries/README.md).

## Run Memory-Check Leak Demo

```bash
cd /path/to/fisiCs
make examples-memory-check
```

This target intentionally leaks one allocation and prints the memory-check
runtime report. It is opt-in and is not part of default `make examples`.

For more detail, see [memory_check/README.md](./memory_check/README.md).

## Run Curated Example Projects

```bash
cd /path/to/fisiCs
make examples-project NAME=compound_growth
make examples-project NAME=kinematics_stepper
make examples-project NAME=memory_pool_lifecycle
make examples-project NAME=population_competition
make examples-project NAME=collision_1d
make examples-project-invalid NAME=kinematics_stepper
make examples-project-invalid NAME=collision_1d
make examples-project-artifacts NAME=kinematics_stepper
make examples-project-memory NAME=memory_pool_lifecycle
make examples-project-video-prep NAME=kinematics_stepper
```

These project directories are designed to be opened directly in the IDE and to
produce deterministic build/run artifacts. For more detail, see
[projects/README.md](./projects/README.md).

## Build And Run: Physics Units Pilot

```bash
cd /path/to/fisiCs
make examples-physics-units
./build/examples/ballistics_valid
```

For overlay-specific usage and semantic dump flows, see:

- [physics_units/README.md](./physics_units/README.md)
- [physics_units/helper_patterns.md](./physics_units/helper_patterns.md)

Those physics-units references are also the current public source of truth for:

- canonical `dim(...)` and `unit(...)` authoring style
- explicit conversion-only boundaries
- widened practical unit-family examples

## Build and Run: Hello World

```bash
cd /path/to/fisiCs
./fisics examples/hello_world.c -o build/examples/hello_world
./build/examples/hello_world
```

## Build and Run: Multi-TU Smoke

Use the public compile/link fixtures under `compilation/`:

```bash
cd /path/to/fisiCs
./fisics compilation/multi_main.c compilation/multi_helper.c -o compilation/out/multi_bin
./compilation/out/multi_bin
```

You can also use:

```bash
cd /path/to/fisiCs
./compilation/run_multi.sh ./fisics
```

## Build and Run: SDL Window Loop

Requires SDL2 development libraries.

```bash
cd /path/to/fisiCs
mkdir -p build/examples
./fisics -c examples/sdl_window_loop.c -o build/examples/sdl_window_loop.o
clang build/examples/sdl_window_loop.o -o build/examples/sdl_window_loop $(sdl2-config --cflags --libs)
./build/examples/sdl_window_loop
```

Press `Esc` or close the window to exit.

## Notes

- The SDL example uses clang for final linking in this README to keep environment setup straightforward.
- If your local `fisics` link flow is configured for SDL2, you can also link directly with `fisics`.
- The clean-user path for these examples is summarized in
  [../docs/first_user_path.md](../docs/first_user_path.md).
