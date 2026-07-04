# First User Path

This is the shortest supported path for a new `fisiCs` user.

Run all commands from:

```bash
cd /path/to/fisiCs
```

## 1. Build From Source

Requirements:

- `cc` or `clang`
- `llvm-config` on `PATH`
- POSIX shell environment

Build:

```bash
make
```

Expected result:

- `./fisics` exists
- `libfisics_frontend.a` exists

## 2. Compile Hello World

```bash
./fisics examples/hello_world.c -o build/examples/hello_world
./build/examples/hello_world
```

Expected output:

```text
Hello from fisiCs.
```

## 3. Compile And Link A Small Multi-TU Program

Use the public smoke fixtures under `compilation/`:

```bash
./fisics compilation/multi_main.c compilation/multi_helper.c -o compilation/out/multi_bin
./compilation/out/multi_bin
```

Expected output:

```text
multi-file smoke: helper=7 total=12
```

You can also use the convenience script:

```bash
./compilation/run_multi.sh ./fisics
```

## 4. Run The Physics-Units Pilot

```bash
make examples-physics-units
./build/examples/ballistics_valid
```

To inspect overlay semantic data:

```bash
./fisics --overlay=physics-units --dump-sema -c examples/physics_units/ballistics_valid.c -o build/examples/ballistics_valid.o
```

The overlay remains opt-in. Normal C mode is still the default compiler path.

## 5. Optional: Run Memory-Check Diagnostics

`memory-check` is an opt-in runtime diagnostics overlay for direct
`malloc`/`calloc`/`realloc`/`free` calls. It is useful for occasional
allocation/free lifecycle checks, not as a full AddressSanitizer replacement.

Minimal example:

```c
#include <stdlib.h>

int main(void) {
    void *p = malloc(32);
    (void)p;
    return 0;
}
```

Compile and run:

```bash
./fisics --overlay=memory-check your_file.c -o build/examples/memory_check_probe
./build/examples/memory_check_probe
```

Expected diagnostics are written to `stderr` with the
`[fisics:memory-check]` prefix. The runtime reports automatically at exit
after tracked activity and includes source labels for direct rewritten
allocator calls. Use `FISICS_MEMCHECK_REPORT=errors`, `leaks`, or `manual` to
reduce or suppress automatic reports. This overlay is default-off and is not
included in `--overlay=all`.

To see a known leak report without writing a file:

```bash
make examples-memory-check
```

## 6. Run The Minimum Smoke Suite

```bash
make examples
make examples-canaries
./compilation/run_single.sh ./fisics
./compilation/run_multi.sh ./fisics
```

The practical canary output should include:

```text
canary multitu: base=14 adjusted=17 label=ok
canary libc-string: fields=3 mass=12 velocity=34 tag=OK42 checksum=256
canary numeric-math: exp=6 scaled=48 rem=1 quotient=8 sign=1 max=48
```

If you want one focused compiler confidence check after that:

```bash
make final-manifest MANIFEST=14-runtime-surface-wave323-header-math-loop-edge-runtime-promotion.json
```

If you want the broad checkpoint lane:

```bash
make final-monitored
```

## 7. Optional: Run The Release Example Pack

For the smallest public demo story that also includes a curated agent task,
use:

```bash
make examples-project NAME=kinematics_stepper
make examples-project-invalid NAME=kinematics_stepper
make examples-project-artifacts NAME=kinematics_stepper
```

The full release example pack is documented in
[../examples/release_example_pack.md](../examples/release_example_pack.md).

## Notes

- For the current supported boundary, read [supported_feature_matrix.md](./supported_feature_matrix.md).
- For practical public canaries, read [../examples/canaries/README.md](../examples/canaries/README.md).
- For overlay behavior and limits, read [extension_overlays.md](./extension_overlays.md).
- For release/archive packaging, read [cli_release_workflow.md](./cli_release_workflow.md).
