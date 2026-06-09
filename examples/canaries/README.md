# Practical Canaries

These canaries are small public examples that exercise realistic `fisiCs`
behavior without requiring the internal final-suite or real-project ladder.

Run all commands from the repository root:

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
```

## Run All Practical Canaries

```bash
make examples-canaries
```

Expected output includes:

```text
canary multitu: base=14 adjusted=17 label=ok
canary libc-string: fields=3 mass=12 velocity=34 tag=OK42 checksum=256
canary numeric-math: exp=6 scaled=48 rem=1 quotient=8 sign=1 max=48
```

## What They Cover

- `multitu_app.c` / `multitu_lib.c`: public multi-translation-unit compile and
  link behavior with a small shared header.
- `libc_string_parser.c`: deterministic `string.h`, `ctype.h`, and `stdlib.h`
  parsing behavior.
- `numeric_math.c`: deterministic finite `math.h` behavior using functions
  already inside the current public support boundary.

The physics-units overlay canary remains under `examples/physics_units/` and is
run by `make examples-physics-units`.
