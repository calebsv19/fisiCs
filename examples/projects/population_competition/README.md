# Population Competition

This curated example simulates two populations competing over a shared resource
pool.

The model is intentionally deterministic: each tick applies growth, resource
pressure, and conflict loss while clamping populations and resources to
nonnegative values.

## Run

```bash
cd /path/to/fisiCs
make examples-project NAME=population_competition
```

Expected output:

```text
tick alpha beta resource
0 40 28 120
1 43 30 115
2 45 32 109
3 47 33 102
4 48 34 94
```

## Generate IDE Artifacts

```bash
cd /path/to/fisiCs
make examples-project-artifacts NAME=population_competition
```

## Demo Purpose

- show a deterministic behavior/invariant project that is not physics-only
- provide an AI-agent task around changing update rules while preserving
  nonnegative state
- give the IDE a small project root with clear source ownership
