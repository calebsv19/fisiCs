# Stage-G Operational Differential Contract

Stage `G_operational_differential` proves that one deterministic,
production-shaped workflow has the same logical behavior when its C sources
are compiled by Clang and by fisiCs.

Stage G is intentionally stronger than the A-F ladder:

- Stage A asks whether translation units compile.
- Stages B and C ask whether configured source sets link or build.
- Stages D and E ask whether selected runtime and golden surfaces execute.
- Stage F records telemetry.
- Stage G compares a serialized operational workflow: ordered semantic state,
  exact exit, declared outputs, repeat determinism, and compiler parity.

DrawingProgram is the completed pilot, MapForge is the completed second
onboarding, DataLab is the completed third onboarding, and Workspace Sandbox
is the completed fourth onboarding. LineDrawing is the completed fifth
onboarding. New projects must reuse this contract and runner rather than
creating project-specific comparison scripts.

RayTracing is the completed sixth onboarding. It adds a production-shaped
render workflow example spanning an immutable render-request snapshot,
scene-project JSON persistence, async readiness/progress transfer, and the
top-level lifecycle wrapper. Its canonical closure is four targets, two runs
per compiler, with exact trace and SHA-256 artifact parity.

PhysicsSim is the completed seventh onboarding. It adds production-owned 3D
domain/emitter/obstacle state transfer, retained-scene document persistence,
grid and placement interaction boundaries, and a composed project/runtime/
cache workflow. Its canonical closure is four targets, two runs per compiler,
with exact trace and SHA-256 artifact parity.

MemConsole is the completed eighth onboarding. It adds graph/browse/project
filter state transfer, UI and app preference persistence, pane-splitter and
graph-camera interaction, and a real Memory DB mutation/reload workflow. Its
canonical closure is four targets, two runs per compiler, with exact trace and
canonical-artifact parity; raw pack and SQLite containers are existence-only
artifacts rather than byte-level semantic oracles.

## Sources Of Truth

- runner:
  `tests/real_projects/runners/run_project_operational_differential_tests.py`
- project/target configuration:
  `tests/real_projects/config/projects_manifest.json`
- public scaffold and report-lane rules:
  `tests/real_projects/README.md`
- reports:
  `tests/real_projects/reports/{latest,history}/`
- artifacts:
  `tests/real_projects/artifacts/{latest,history}/`
- completed execution records (paths relative to the CodeWork root):
  `docs/private_program_docs/fisiCs/active/drawing_program_operational_differential_bites_plan.md`
  and
  `docs/private_program_docs/fisiCs/active/map_forge_operational_differential_bites_plan.md`
  and
  `docs/private_program_docs/fisiCs/active/datalab_operational_differential_bites_plan.md`
  and
  `docs/private_program_docs/fisiCs/active/workspace_sandbox_operational_differential_bites_plan.md`
  and
  `docs/private_program_docs/fisiCs/active/line_drawing_operational_differential_bites_plan.md`
  and
  `docs/private_program_docs/fisiCs/active/ray_tracing_operational_differential_bites_plan.md`
  and
  `docs/private_program_docs/fisiCs/active/physics_sim_operational_differential_bites_plan.md`
  and
  `docs/private_program_docs/fisiCs/active/mem_console_operational_differential_bites_plan.md`

RayTracing's in-repository executable contract is also fully represented by
its four manifest targets and the fixtures under
`tests/real_projects/fixtures/ray_tracing_stage_g/`. The private execution
checkpoint remains the narrative source for G0 classification and closure
evidence once synchronized.

## Non-Negotiable Comparison Model

Each target must use:

1. the same driver source,
2. the same production source set,
3. the same configured scenario and seed,
4. the same isolated input fixtures,
5. the same explicit runtime environment,
6. the same ordered checkpoint vocabulary,
7. the same expected exit and artifact allowlist,
8. at least two executions per compiler.

Clang is the reference implementation lane, not automatic proof that a result
is correct. Before classifying a fisiCs-only result as a compiler/runtime
defect, exclude:

- undefined or implementation-defined behavior,
- application nondeterminism,
- fixture or manifest errors,
- unstable filesystem ordering,
- host environment leakage,
- raw padding, pointer, address, timestamp, and temp-path comparisons,
- a defective semantic oracle.

## Trace Contract

The driver writes trace records to stdout only:

```text
TRACE|1|checkpoint_name|field=value|field=value|result=1
```

Requirements:

- version is exactly `1`,
- checkpoint names are unique,
- checkpoint names and order exactly match the manifest,
- every line on stdout is a trace line,
- fields are canonical semantic values,
- trace text is deterministic within each compiler,
- complete trace text is identical across compilers.

Good fields include dimensions, counts, enum names or stable numeric values,
logical IDs, canonical coordinates, history cursors, state flags, and bounded
logical digests.

Never trace raw structs, pointer values, padding, addresses, allocator state,
wall-clock time, process IDs, random temp paths, or unordered traversal output.

## Artifact Contract

Every file written in a scenario run directory must be declared. Undeclared or
missing files fail the target.

Supported comparison modes:

- `sha256`: bytes are part of the cross-compiler semantic oracle,
- `exists`: the file must exist, but raw bytes are not compared.

Use `sha256` only for canonical logical outputs or formats known to be
deterministic. Use `exists` for containers whose bytes may contain irrelevant
padding or metadata, and emit a separate canonical projection for comparison.

For JSON, manifests, summaries, image metadata, or traces that contain temp
paths, timestamps, host paths, unordered object keys, or incidental runtime
statistics, write an additional canonical artifact that:

- removes volatile fields,
- sorts unordered collections,
- uses stable relative logical names,
- normalizes numeric representation deliberately,
- preserves every field that affects user-visible semantics.

## Runtime Isolation And Fixture Inputs

DrawingProgram's pilot drivers construct inputs in memory. MapForge required
real region, pin, and job fixtures, so its G0 added one manifest-driven
runtime-input contract to the reusable runner without project-specific shell
setup.

That stable contract provides:

- explicit `run_env` values with project/run-root placeholders,
- configurable project environment-prefix scrubbing before applying
  `run_env`,
- per-compiler, per-repeat fixture copies so mutation cannot cross-contaminate
  runs,
- declared fixture paths excluded from unexpected-output artifact discovery,
- saved command/environment/fixture provenance in the report artifacts,
- contract tests for missing fixture, traversal, duplicate destination,
  environment leakage, mutation isolation, and zero selection.

MapForge's completed G0 owns the runner extension and its contract tests.
Later programs reuse it unchanged unless a separately demonstrated generic
gap requires a bounded contract revision.

## Manifest Target Shape

The current stable fields are:

```json
{
  "id": "project_bite_name",
  "inputs": ["driver.c", "production_source.c"],
  "link_inputs": ["path/to/library.a"],
  "link_args": ["-lm"],
  "run_args": ["--bite", "1", "--seed", "1701"],
  "scrub_env_prefixes": ["PROJECT_"],
  "run_env": {
    "PROJECT_RUNTIME_DIR": "{run_root}/runtime",
    "PROJECT_DATA_ROOT": "{project_root}/data"
  },
  "runtime_fixtures": [
    {"source": "tests/fixtures/input.json", "path": "fixtures/input.json"}
  ],
  "expected_exit_code": 0,
  "expected_checkpoints": ["initialized", "mutated", "shutdown"],
  "expected_artifacts": [
    {"path": "state.canonical", "compare": "sha256"},
    {"path": "state.pack", "compare": "exists"}
  ]
}
```

`run_env` supports `{project_root}`, `{run_root}`, and `{fixture_root}`.
Fixture destinations must be non-empty relative paths without `..`; duplicate
destinations, missing sources, and non-file/non-directory sources fail closed.
Each compiler/repeat gets a fresh copy, fixture files are excluded from output
artifact discovery, and the runner records applied environment values plus
before/after fixture hashes beside each run artifact.

The runtime-isolation extension adds these documented optional fields without
changing existing DrawingProgram target behavior.

## Eligibility Refresh (G0)

Before writing a Stage-G driver:

1. refresh the project's Stage A report without filters,
2. classify every `both_fail`, `clang_only_fail`, and `fisics_only_fail`,
3. refresh the configured B-F targets needed to establish current build truth,
4. identify production sources required for the first operational bite,
5. prove every selected production source compiles under both compilers,
6. record current git commit/branch/dirty state for both repositories,
7. record active compiler and real-project ledgers,
8. reject an empty or stale-only operational selection.

An existing A-F green report is orientation evidence, not a substitute for a
fresh G0 when the project has changed since that report.

## Bite Construction Rules

Build serialized bites from the lowest stable semantic boundary toward one
top-level workflow:

- a bite must exercise production logic rather than reproduce it in the
  fixture,
- test-only stubs are allowed only for unreachable integration dependencies,
- every stub must be named and justified in the checkpoint record,
- fixture code may canonicalize state but may not change application behavior,
- production application code is reference-only unless an application defect
  is separately authorized,
- each later bite reruns every completed earlier bite,
- failures are not repeated unchanged more than twice.

## Automatic Probe/Fix Mode Control

Start in `fisics-probe-mode`.

For a suspected fisiCs defect:

1. reproduce it twice without changing the scenario,
2. prove Clang accepts the same valid C and input,
3. minimize it into the owning compiler bucket,
4. record it in both active-only blocker ledgers,
5. enter `fisics-fix-mode` for one blocker family,
6. apply the smallest compiler/runtime fix,
7. add and register a permanent final regression,
8. run the focused manifest, owner bucket, and relevant probes,
9. rerun every completed Stage-G bite,
10. remove the resolved active-ledger entries,
11. return to probe mode and continue.

Stop the dependent bite for a broad compiler redesign, ambiguous oracle,
shared-library architecture change, or application-production defect. Continue
independent safe work when possible.

## Canonical And Filtered Commands

```bash
# filtered development evidence; noncanonical lanes only
python3 tests/real_projects/runners/run_project_operational_differential_tests.py \
  --project <project> --target <target_id> --repeat 2

# canonical full Stage-G closure
python3 tests/real_projects/runners/run_project_operational_differential_tests.py \
  --project <project> --repeat 2
```

Filtered, dry-run, or partial selection must never replace canonical latest
reports or artifacts.

## Closure Gate

A project Stage-G lane is complete only when:

- G0 is current and documented,
- every planned bite is `both_pass`,
- repeat determinism is green,
- all traces and compared artifacts match,
- exact exits and timeouts are correct,
- no unexpected/missing artifacts remain,
- all later fixes replay every earlier bite,
- focused owner validation is green for every compiler fix,
- one meaningful registered `make final-monitored` gate is green at final
  closure or after a broad-risk compiler fix,
- `make final-promotion-audit` is green,
- both active-only ledgers contain unresolved items only,
- private execution checkpoints and applicable public docs are synchronized,
- the next onboarding boundary is named.

## One-Goal Handoff Requirements

An onboarding plan is ready for one uninterrupted goal only if it includes:

- authoritative plan path and required read order,
- current source/report/git truth,
- exact G0 and bite order,
- fixture and environment isolation rules,
- scenario/checkpoint/artifact expectations per bite,
- automatic probe/fix transitions,
- authorized and forbidden writes,
- replay and broad closure gates,
- durable checkpoint locations,
- explicit terminal conditions and material-stop conditions.

The completed DrawingProgram, MapForge, DataLab, Workspace Sandbox, and
LineDrawing plans satisfy this checklist and provide five onboarding examples
with materially different state, persistence, interaction, and lifecycle
boundaries.

RayTracing provides the sixth example and a reusable template for programs
whose operational boundary is a job handoff rather than an editor model. Its
four serialized bites prove:

1. canonical render-request snapshot construction, invalid dimensions,
   prepared/material/light/acceleration identity, destinations, and cancel
   generation;
2. scene-project request resolve/write/destroy/reload, preservation of unknown
   JSON fields, portable path enforcement, and invalid frame windows;
3. async readiness rejection for each unbound/dynamic state plus exact
   dirty-rectangle deep-copy, stale-generation, reset, and pixel bytes;
4. wrapper bootstrap through project persistence, snapshot readiness,
   events/update/route/submit, run loop, export, shutdown, and post-shutdown
   rejection.

PhysicsSim provides the seventh example and a template for simulation programs
whose semantic boundary combines solver-domain state with retained project
files. Its four serialized bites prove:

1. 3D domain derivation, emitter placement, obstacle policy, volume mutation,
   clear, and destroy;
2. retained-scene duplicate/save/destroy/reload, scene-id rewrite, path reuse,
   and exact runtime/authoring artifacts;
3. grid update and reversal, placement clamping, and rejected invalid emitter
   and boundary-face controls;
4. project bootstrap through runtime projection, select/move/resize state,
   undo/redo checkpoints, retained-scene save/reload, cache status, export,
   canonicalization, and shutdown.

MemConsole provides the eighth example and a template for database-backed
interactive tools whose operational boundary mixes in-memory filters,
preferences, geometry, and persistent mutations. Its four serialized bites
prove:

1. graph edge/node filters, hidden anchors, project pruning, view-mode reset,
   selection state, and a canonical state-transfer artifact;
2. UI/app preference save, state destruction, reload, logical signature/path
   comparison, and canonical persistence output;
3. pane layout, splitter begin/update/reversal and clamp behavior, cursor-
   anchored zoom, drag pan, release suppression, and invalid outside controls;
4. Memory DB open/seed, rename/pin, relationship creation/kind cycling,
   preference save, DB close/reopen, semantic query comparison, invalid
   self-link rejection, canonical output, and shutdown.

For the next one-goal onboarding, keep the same structure: refresh every A-F
eligibility lane first, choose four production-owned semantic seams, declare
all fixture/environment inputs in the manifest, add targets serially, and run
the full configured target set after each bite. Do not begin from a saved
green report or copy another program's state oracle without reclassifying its
production boundaries.
