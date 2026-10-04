# Docs Layout

This directory is the public documentation lane for `fisiCs`.
It is intentionally focused on stable user/contributor references.
Last audited: 2026-09-11.

## Repository Entry

- Root `README.md` is the canonical GitHub-facing project summary and quickstart.
- Root `AGENTS.md` is the concise operating contract for automated tooling and
  fresh AI agents.
- `TOP_README.md` is now a legacy compatibility pointer to root `README.md`.

## Top Level

- `00_docs_index.md`: primary table of contents for all Markdown docs in `docs/`
- `supported_feature_matrix.md`: current public support boundary for default C,
  headers/runtime, compile/link behavior, build-graph/local-manifest dry-run
  tooling scope, behavior policy, and extension limits
- `first_user_path.md`: shortest supported path from build to hello world,
  multi-TU compile/link smoke, physics-units pilot, and minimum smoke gates
- `release_confidence_checklist.md`: smaller contributor-facing release
  readiness checklist
- memory-check validation has a focused structured lane:
  `make memory-check-test`
- `public_roadmap.md`: high-level public roadmap and current focus areas
- `contributor_agent_quickstart.md`: concise operating flow for contributors and automated tooling
- `compiler_test_system_rearchitecture_context.md`: core constraints and end-state targets for test architecture
- `compiler_test_architecture.md`: authoritative public architecture map for the compiler test system
- `compiler_test_failure_taxonomy.md`: shared failure-kind, severity, and origin vocabulary across compiler test lanes
- `compiler_test_regression_intake.md`: canonical reduce, classify, tag, promote, and revalidate workflow for real-world regressions
- `compiler_test_confidence_tiers.md`: canonical day-to-day command ladder from fast sanity through full trust and timing checkpoints
- `compiler_test_coverage_blueprint.md`: coverage map and harness structure for compiler validation
- `compiler_test_workflow_guide.md`: operator guide for bucket-by-bucket validation/fix workflows
- `os_policy_validation.md`: freestanding OS-policy semantic, object-invariant,
  repeated QEMU, closed OS-P3 stress, post-EDU-19 source-intake and stable
  runtime promotion, and optional downstream-canary contract
- `esp_portable_c_validation_contract.md`: frozen intake and evidence contract
  for validating small production-used ESP-owned C policy modules under
  `fisiCs`, Clang, and the separate ESP vendor build
- `esp_metadata_integration_roadmap.md`: current cross-system ownership,
  implemented capability, evidence limits and proposed graph-to-profile steps
- `attachment_handoff_contract.md`: compiler-owned, project-neutral declared
  attachment audit, v1/v2 verification CLI/Python interface, portable example and
  consumer migration boundaries
- `attachment_handoff_provenance.md`: optional producer-parent byte receipts,
  backward compatibility and evidence limits
- `frozen_graph_inputs.md`: complete offline graph data-input snapshots,
  detached replay/readback, source identity preservation and closeout boundaries
- `esp_aht20_executable_binding.md`: bounded AHT20 graph-to-profile execution,
  preflight rejection, retained artifacts and hardware-free readback
- `e016_sensor_metadata_bridge_plan.md`: completed bridge implementation plan for
  composing the proven E016 portable C sensor carrier through semantic,
  physical, and hardware graphs while retaining separate deployment and
  observation evidence
- `external_evidence_receipts.md`: project-neutral, digest-bound contract for
  candidate, deployment, raw text capture, observation, and operator evidence;
  includes the hardware-free validator command and retention rules
- `esp_compiler_bridge_case_study.md`: runnable explanation of the E016 shared
  C/Arduino boundary, compiler graph flow, retained realization mismatch, and
  smallest future ESP adoption step
- `esp_metadata_preservation_checkpoint.md`: current cross-repository
  preservation baseline, sensor-profile compiler adoption matrix, retained
  mixed-graph hashes, reconciliation findings, and Pass 2 entry contract
- `esp_can_topology_pass2.md`: completed deterministic three-node ESP/CAN
  static topology, canonical environmental quantity migration, bounded
  endpoint/middle termination-intent validation, and exact evidence boundaries
- `esp_can_simulation_pass3.md`: software-only execution of the admitted ESP
  AHT20 and ultrasonic models, exact binding to the Pass 2 graph hashes, and a
  deterministic simulated producer/consumer projection
- `esp_can_transport_pass4.md`: frame-level Classical CAN software model with
  payload round trips, CRC and stuffing, arbitration, queues, latency,
  loss/retry fault scenarios, and utilization receipts
- `esp_can_receiver_pass5.md`: supervisor RX queue, protocol-conformance,
  duplicate/order, latest-good, freshness-expiry, and compact debugger replay
  proof with explicit v1 session/version limitations
- `esp_can_protocol_session_pass6.md`: reconciliation with the actual ESP
  extended-29-bit big-endian protocol, generated golden vectors, heartbeat
  sessions, restart/timeout/recovery behavior, and stable-base assessment
- `esp_can_metadata_lane_closeout.md`: canonical restart and closeout page for
  the complete E016-to-Pass-6 metadata, static graph, simulation, transport,
  receiver, and protocol-session evidence chain
- `esp_network_sim_growth_architecture.md`: cross-system design handoff for
  deterministic CAN execution, event journaling, physics adapters, compiler
  graph intake, scaling, and physical-test boundaries
- `validation_workflow.md`: full-project validation workflow for external program compile checks
- `frontend_api.md`: reusable frontend library API notes
- `extension_overlays.md`: public reference for opt-in overlay lanes, including
  the physics-units surface and the explicit memory-check runtime diagnostics
  overlay
- `extensions_guide.md`: concise selection and quickstart path for current
  overlays plus the offline static graph pipeline
- `metadata_system_concepts.md`: stable public vocabulary for the metadata and
  static graph capabilities
- `extension_portability.md`: shared-source macro contract, public-example
  adoption boundary, and strict Clang/fisiCs parity gate through
  `include/fisics/extensions.h`
- `agent_extension_guide.md`: agent routing, artifact freshness,
  evidence-language, validation, and completion contract for extensions and
  the static physical-system graph pipeline
- `semantic_metadata_contract.md`: implemented identity/domain/role attachment
  and relationship contract plus project, physical, and hardware boundaries
- `semantic_metadata_export_contract.md`: implemented extended-frontend and
  deterministic translation-unit JSON export contract
- `semantic_metadata_project_graph_contract.md`: dedicated manifest,
  strict cross-TU resolution, provenance, output, and failure contract;
  focused and broad validation evidence is recorded in the contract
- `semantic_metadata_physical_composition_contract.md`: implemented physical
  type, units-evidence, quantity-catalog, compatibility, provenance, output,
  failure, and focused-proof contract
- `semantic_metadata_hardware_contract.md`: implemented static hardware
  catalog/topology, profile/model/driver, allocation, physical
  signal binding, evidence-scope, and canary contract
- `semantic_metadata_implementation_plan.md`: historical design and completed
  implementation record for maintainers; current users should start with the
  concepts, quickstart, and capability contracts
- `compiler_ide_data_contract.md`: versioned compiler-to-IDE communication contract (current: `fisiCs.analysis.contract` `1.9.0`)
- build graph JSON now carries compact diagnostic summaries while full
  diagnostic payloads remain in the dedicated diagnostics JSON/pack lanes
- `cli_release_workflow.md`: CLI release packaging/sign/notarize flow for macOS artifacts
- `make_final_timing_log.md`: public timing-baseline lane and monitored broad-run policy for `make final`

Public example references live under `examples/`:
- `examples/README.md`: top-level examples lane
- `examples/canaries/README.md`: practical public canaries for multi-TU,
  libc/string parsing, and numeric/math behavior
- `examples/physics_units/README.md`: first public pilot for the physics-units overlay
- `examples/semantic_metadata/README.md`: basic semantic metadata, strict
  Clang/fisiCs portability, and a complete static hardware example

Public compile/link smoke fixtures also live under `compilation/`:
- `compilation/README.md`: single-file and multi-TU public smoke path

Additional public testing references live outside `docs/` under `tests/final/`:
- bucket scope references (for example `tests/final/11-functions-calls.md`, `tests/final/12-diagnostics-recovery.md`)
- probe lane references (`tests/final/probes/README.md`, `tests/final/probes/run_probes.py`)

Freestanding operating-system policy validation lives under
`tests/os_policy/`; its focused command surface is `make os-policy-object`,
`make os-policy-runtime`, and `make os-policy`.

Real-project validation references live under `tests/real_projects/`:
- scaffold overview and stage contracts: `tests/real_projects/README.md`
- stage runners (`A`..`F`): `tests/real_projects/runners/`
- project manifest: `tests/real_projects/config/projects_manifest.json`
- exact compile oracle: `tests/real_projects/runners/run_project_exact_compile_oracle.py`
- profile oracle: `tests/real_projects/runners/run_project_profile_oracle.py`
- canonical stage reports now carry shared `report_contract` metadata so
  narrowed and full-closure runs remain distinguishable in public evidence

Current public test campaign context:
- higher-stress runtime and diagnostics validation expansion is active under `tests/final/`
- real-project compile and runtime-smoke validation continues under `tests/real_projects/`
- current runner maintenance also includes keeping the staged canary reports on
  one explicit canonical/noncanonical contract

## Private Docs Boundary

Internal bucket-level run logs, triage plans, external gap reports, and raw status logs are intentionally kept in private maintainer documentation outside this public docs lane.

Public docs in this directory should remain concise, stable, and GitHub-facing.
