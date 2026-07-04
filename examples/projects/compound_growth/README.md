# Compound Growth

This curated example is a tiny time-step project for deterministic value
growth.

It models a starting balance, a fixed periodic contribution, and one growth
rate applied over four ticks. The example is intentionally small so it can be
opened in the IDE and used as the simplest project-shaped `fisiCs` demo.

## Run

```bash
cd /path/to/fisiCs
make examples-project NAME=compound_growth
```

Expected output:

```text
tick balance
0 1000.00
1 1060.00
2 1120.60
3 1181.81
4 1243.62
```

## Generate IDE Artifacts

```bash
cd /path/to/fisiCs
make examples-project-artifacts NAME=compound_growth
```

The command writes generated artifacts under this project root:

- `build/stdout.txt`
- `ide_files/build_graph.json`

## Demo Purpose

- project-shaped example that is easy to open in the IDE
- deterministic stdout oracle
- simple behavior that an AI agent can modify safely
