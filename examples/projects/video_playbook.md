# Curated Project Video Playbook

Use this playbook when recording the curated `fisiCs` project demos.

The goal is to make each video repeatable:

1. open a small project directory in the IDE
2. show the source and project-local contract
3. run the proof command
4. show expected output or expected failure
5. show generated artifacts under `ide_files/`
6. connect the result to compiler/IDE/agent workflows

## Prep Command

Run this before recording a project:

```bash
cd /path/to/fisiCs
make examples-project-video-prep NAME=<project>
```

The prep target runs the valid project, generates build graph artifacts, runs
invalid fixtures when present, runs memory reports for `memory_pool_lifecycle`,
and writes:

```text
examples/projects/<project>/build/demo_artifacts/video_prep_manifest.md
```

## Recommended Recording Order

1. `compound_growth`
   - simplest project-shaped workflow
   - shows deterministic stdout and build graph artifacts
2. `kinematics_stepper`
   - first physics-units story
   - shows valid motion plus invalid units addition
3. `memory_pool_lifecycle`
   - memory-check story
   - shows clean and leaky `memory_check_report_v1` sidecars
4. `population_competition`
   - deterministic behavior/invariant story
   - good AI-agent modification candidate
5. `collision_1d`
   - physics-units collision story
   - shows valid collision math plus invalid units mix

## Per-Project Recording Loop

For each project:

1. Open `fisiCs/examples/projects/<project>` as the IDE root.
2. Open `video_script.md`.
3. Open `README.md`.
4. Run the commands listed in
   `build/demo_artifacts/video_prep_manifest.md`.
5. Show `build/stdout.txt` and compare it to `tests/expected_stdout.txt`.
6. Show `ide_files/build_graph.json`.
7. If present, show invalid fixture output from `build/*.stderr.txt`.
8. If present, show memory reports under `ide_files/`.

## Current IDE Expectations

The CLI/artifact side is ready now. IDE presentation is partially dependent on
the separate `IBUI-S7` lane:

- build graph summary view or Project/Libraries submode
- memory report summary view
- leak-site navigation back to source files

Until those UI pieces land, videos can still show the generated artifact files
directly inside the IDE workspace.

## Acceptance Before Recording

Run all prep commands from the `fisiCs` root:

```bash
make examples-project-video-prep NAME=compound_growth
make examples-project-video-prep NAME=kinematics_stepper
make examples-project-video-prep NAME=memory_pool_lifecycle
make examples-project-video-prep NAME=population_competition
make examples-project-video-prep NAME=collision_1d
```

Each project should produce a `video_prep_manifest.md` and no target should
fail.
