# Curated Example Projects

This directory contains small project-shaped `fisiCs` examples.

Unlike single-file examples, each project is meant to be opened as an IDE
workspace, run through named make targets, and used as a compact demo story.

For the first public release example pack, use `kinematics_stepper` as the
canonical curated agent demo. It combines deterministic output, an intentional
units diagnostic, generated build-graph artifacts, and a bounded
`agent_task.md`.

## Run A Project

```bash
cd /path/to/fisiCs
make examples-project NAME=compound_growth
make examples-project NAME=kinematics_stepper
make examples-project NAME=memory_pool_lifecycle
make examples-project NAME=population_competition
make examples-project NAME=collision_1d
```

## Run Intentional Failure Cases

```bash
cd /path/to/fisiCs
make examples-project-invalid NAME=kinematics_stepper
make examples-project-invalid NAME=collision_1d
```

The invalid target expects the compiler to reject the fixture and checks for
stable diagnostic text.

## Generate IDE/Tooling Artifacts

```bash
cd /path/to/fisiCs
make examples-project-artifacts NAME=kinematics_stepper
```

Artifacts are written under the selected project root:

- `build/`: binaries and captured output
- `ide_files/`: build graph or later IDE analysis artifacts

Those generated directories are ignored by git.

## Prepare A Project For Recording

```bash
cd /path/to/fisiCs
make examples-project-video-prep NAME=kinematics_stepper
```

The video-prep target runs the valid project, generates artifacts, runs invalid
fixtures when present, runs memory reports for `memory_pool_lifecycle`, and
writes a recording manifest under `build/demo_artifacts/`.

For the full recording workflow, see [video_playbook.md](./video_playbook.md).

## Run Memory-Check Project Reports

```bash
cd /path/to/fisiCs
make examples-project-memory NAME=memory_pool_lifecycle
```

This writes clean and intentionally leaky `memory_check_report_v1` sidecars
under `examples/projects/memory_pool_lifecycle/ide_files/`.

## Project Contract

Each curated project should include:

- `README.md`: human explanation and commands
- `project.fisics.json`: local manifest shape for tooling/IDE use
- `agent_task.md`: bounded agent task and required checks
- `video_script.md`: short recording outline
- `src/`: project source
- `tests/`: expected output and optional invalid fixtures

The goal is one clear story per project, not one large example that tries to
show every compiler feature.
