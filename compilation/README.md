# Compilation Samples

Quick smoke fixtures that exercise our driver without touching the makefile. Outputs land in `compilation/out/` so they can be safely ignored.

## Single-file (no headers)

```bash
./compiler compilation/single.c -o compilation/out/single_bin
./compilation/out/single_bin
```

Expected: prints `single-file smoke: add 2 + 3 = 5` and exits 0.

## Multi-file (helper + main)

```bash
./compiler compilation/multi_main.c compilation/multi_helper.c -o compilation/out/multi_bin
./compilation/out/multi_bin
```

Expected: prints the helper result and combined sum, exits 0.

Scripts are provided for convenience:

```bash
./compilation/run_single.sh
./compilation/run_multi.sh
```
