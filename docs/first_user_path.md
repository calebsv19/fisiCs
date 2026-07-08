# First User Path

This is the shortest supported path for a new `fisiCs` user. You can either
download the current packaged compiler or build from source.

## 1. Use The Packaged Compiler

The current public package is listed on the Ecosystem program page:

- human page:
  <https://ecosystem.calebsv.tech/suite/program/?repo=fisiCs>
- agent entrypoint:
  <https://ecosystem.calebsv.tech/agents/START_HERE.md>
- direct manifest:
  <https://ecosystem.calebsv.tech/agents/programs/fisics.json>

For a fresh macOS Apple Silicon package check, fetch the manifest, download the
current archive, verify the listed SHA-256, unpack it, and run `bin/fisics`:

```bash
workdir="$(mktemp -d)"
curl -fsSL https://ecosystem.calebsv.tech/agents/programs/fisics.json \
  -o "$workdir/fisics.json"
python3 - "$workdir/fisics.json" "$workdir/package.env" <<'PY'
import json
import sys

manifest_path, env_path = sys.argv[1:]
manifest = json.load(open(manifest_path, encoding="utf-8"))
for item in manifest["release"]["downloads"]:
    if item["format"] == "tar.gz" and item["platform"] == "macOS" and item["arch"] == "arm64":
        with open(env_path, "w", encoding="utf-8") as f:
            f.write(f"PACKAGE_URL={item['url']}\n")
            f.write(f"PACKAGE_SHA256={item['sha256']}\n")
            f.write(f"PACKAGE_DIR=fisiCs-{manifest['release']['version']}-macOS-arm64-stable\n")
        break
else:
    raise SystemExit("no macOS arm64 tar.gz package in manifest")
PY
. "$workdir/package.env"
curl -fL "$PACKAGE_URL" -o "$workdir/fisiCs.tar.gz"
shasum -a 256 "$workdir/fisiCs.tar.gz"
tar -xzf "$workdir/fisiCs.tar.gz" -C "$workdir"
```

The SHA-256 should match the value in the manifest. First confirm the compiler
reports normal CLI metadata:

```bash
"$workdir/$PACKAGE_DIR"/bin/fisics --version
"$workdir/$PACKAGE_DIR"/bin/fisics --help
```

Then compile a minimal program:

```bash
cat > "$workdir/hello.c" <<'C'
int main(void) { return 0; }
C
"$workdir/$PACKAGE_DIR"/bin/fisics \
  "$workdir/hello.c" -o "$workdir/hello"
"$workdir/hello"
```

Expected result:

- semantic analysis reports no issues
- an executable is produced
- the executable exits with status `0`

## 2. Build From Source

Use this path when you want the full repository, examples, and validation
commands:

```bash
git clone https://github.com/calebsv19/fisiCs.git
cd fisiCs
```

Requirements:

- `cc` or `clang`
- `llvm-config` on `PATH`
- POSIX shell environment

Build:

```bash
make
./fisics --version
./fisics --help
```

Expected result:

- `./fisics` exists
- `libfisics_frontend.a` exists
- `./fisics --version` prints the compiler version
- `./fisics --help` prints usage without compiling a source file

## 3. Compile Hello World

```bash
./fisics examples/hello_world.c -o build/examples/hello_world
./build/examples/hello_world
```

Expected output:

```text
Hello from fisiCs.
```

## 4. Compile And Link A Small Multi-TU Program

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

## 5. Run The Physics-Units Pilot

```bash
make examples-physics-units
./build/examples/ballistics_valid
```

To inspect overlay semantic data:

```bash
./fisics --overlay=physics-units --dump-sema -c examples/physics_units/ballistics_valid.c -o build/examples/ballistics_valid.o
```

The overlay remains opt-in. Normal C mode is still the default compiler path.

## 6. Optional: Run Memory-Check Diagnostics

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

## 7. Run The Minimum Smoke Suite

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

## 8. Optional: Run The Release Example Pack

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
