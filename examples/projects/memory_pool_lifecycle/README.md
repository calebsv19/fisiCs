# Memory Pool Lifecycle

This curated example demonstrates the `memory-check` overlay on a tiny
allocation lifecycle.

The clean project run allocates a small entity pool and event buffer, updates
them over two ticks, frees everything, and writes a balanced memory-check JSON
report. The memory mode also compiles an intentional leaky fixture so the same
project can produce a failing report for IDE/media demos.

## Run Clean Project

```bash
cd /path/to/fisiCs
make examples-project NAME=memory_pool_lifecycle
```

## Run Clean And Leaky Memory Reports

```bash
cd /path/to/fisiCs
make examples-project-memory NAME=memory_pool_lifecycle
```

Generated reports:

- `ide_files/memory_report_clean.json`
- `ide_files/memory_report_leaky.json`

## Generate Build Graph Artifact

```bash
cd /path/to/fisiCs
make examples-project-artifacts NAME=memory_pool_lifecycle
```

## Demo Purpose

- prove balanced allocation/free behavior with `active=0`
- show a deterministic leak report with one missed free
- provide project-local JSON sidecars for IDE memory-report ingestion
