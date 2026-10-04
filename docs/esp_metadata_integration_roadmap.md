# ESP metadata integration: current state and next steps

Documentation reconciliation: 2026-09-11. This page separates the inspected
software surface from proposed work. It does not record a fresh test run.

## Start here

Compiler-lane refinement: the [declared attachment handoff
contract](attachment_handoff_contract.md) now owns the project-neutral v1
verification interface and audit findings. This task's next work stays in
compiler attachments/provenance; native UI, scene and execution orchestration
remain separate consumer-owned work. The AHT20 integration below is a retained
witness, not the public API for all consumers.

Compiler-only update (2026-09-12): AHT20 declaration validation now delegates
to that interface with execution parity and unchanged pins. Vector/frame,
tensor/frame, relative, cyclic, convertible-kelvin and dimension-only declaration
cases are qualified. [Optional producer provenance](attachment_handoff_provenance.md)
adds explicit handoff-v2 semantic-parent byte association without changing
graph-v1 formats. The [frozen offline input closure](frozen_graph_inputs.md) is
also implemented: copied metadata/source/catalog/topology inputs feed all three
graph stages with detached replay and receipt verification. This closes the
bounded offline compiler milestone, not native model execution or CAN firmware
work. Full C preprocessing/export reproducibility remains separate. This update does not
refresh the independently owned, dated native/runtime assessment below.

The compiler-to-ESP bridge is implemented, not awaiting its first graph.
The remaining integration problem is making validated identities useful to
simulation consumers without confusing a declaration with executable behavior.

Use the [extension agent guide](agent_extension_guide.md) for authoring and
the [CAN lane closeout](esp_can_metadata_lane_closeout.md) for exact fixtures,
commands, retained results and digest pins. The
[E016 case study](esp_compiler_bridge_case_study.md) explains the shared-C
boundary. Older numbered plans remain implementation history, not a required
learning sequence or a claim that their completed work is still pending.

## Ownership and established capability

| Owner | Established surface | Boundary |
| --- | --- | --- |
| fisiCs | Portable annotation macros, units and semantic validation, translation-unit export, semantic/physical/hardware composition; external E016 intake and deterministic CAN evidence chain | Static compatibility does not execute a driver, prove wiring, or compile Arduino C++ |
| ESP_Work | Production runtime/protocol and host simulation; sixteen admitted profiles through the common control-data lifecycle and consumer v2 | Profiles have individual capabilities; admission is not arbitrary composition or physical calibration |
| ESP Network Sim | Native retained-result inspection through consumer v2, historical wire replay, static graph intake and physical placement | Executable compiler-graph model binding is not established; installed Desktop is older than the inspected Main Edit source |
| Experiment records | Board, firmware, capture and operator evidence | Historical E016 exact selected firmware-pair acceptance remains unestablished; raw serial was not retained |

The CAN software chain already executes ESP-owned AHT20 and ultrasonic models,
projects their observations through static identities, models transport and
receiver behavior, and checks a protocol-v1-compatible projection. Do not
rebuild this as a new metadata feature. Earlier behavioral codecs are not the
actual wire contract; the protocol/session contract owns that distinction.

Native v2 readback has retained mixed-CAN, AHT20-family and MPU6050-family
coverage. This is not blanket native execution support for every backend
profile. A rendered graph or room layout is not an environmental solver.

## Source-of-truth routing

In an independently located ESP_Work checkout, start at `simulation/README.md`,
then `simulation/control_data/consumer-v2.md`, `simulation/profiles/README.md`
and `simulation/scenarios/README.md`. The admitted registry lives in
`simulation/control_data/registry.py`; use its discovery surface instead of
assuming this dated profile count remains current.

In the active native development checkout, read `docs/current_truth.md` and
`docs/graph-intake.md`. Dated reconciliation and experiment records remain
historical evidence. Do not substitute installed-app behavior for source
capability, or silently resolve conflicting deployment records.

## Executable graph-to-profile binding milestone

The bounded AHT20 backend adapter and native retained-result inspection are now
implemented; see the
[executable binding contract](esp_aht20_executable_binding.md) for its exact
scope, commands and retained-result verification. Native in-app preparation and
execution remain future work. The sequence records the milestone boundaries rather than claiming
that a general graph execution system is complete.
Start with `environment.aht20.v1`: it already participates in the compiler
simulation chain and has native retained-result coverage.

1. **Audit the existing join.** Inventory the current graph identities, AHT20
   ports, physical representations, runner pins, consumer-v2 fields and native
   binding states. Reproduce the existing CAN gate against the selected ESP
   source snapshot. Classify drift before repinning anything. Exit with an
   explicit reuse/gap table, not a new adapter implementation.
2. **Freeze a minimal binding contract.** Specify graph and source digests,
   device/port semantic identities, profile/model/adapter versions, quantity
   and representation compatibility, and supported/unbound/unsupported/error
   behavior. Reuse existing fields where possible. Models must be explicitly
   selected; names do not imply executable support. Ordinary C carriers and
   C++ runtime implementations retain separate provenance.
3. **Implement one backend binding.** Validate the binding before execution;
   retain its identity and graph provenance with the immutable result using
   the existing consumer contract where possible. Prove rejection of stale
   graphs, unknown identities, duplicate assignments, incompatible ports and
   unsupported models. No silent unit conversion or schema-version bypass.
4. **Adopt the result in native inspection.** Use the existing v2 readback
   surface to show the verified device/port/model association and evidence
   level. Compare values, quality, freshness and identities against backend
   results. Missing support must remain visible rather than auto-binding.

Completion means one reproducible, digest-bound software example with negative
tests and documented commands. It does not require new compiler syntax,
firmware generation, arbitrary graph execution, or a bench session.

## Related but separate work

- **ESP transport correctness:** audit, then enforce and host-test flushing
  queued signals before a new-session heartbeat. Protocol v1 has no session
  identity in each signal; a receiver cannot infer a delayed signal's origin.
- **Protocol-compatible visual replay:** admit the existing CAN protocol/session
  replay through a versioned native adapter and compare receiver projections.
  Keep this distinct from generic model binding and historical replay pins.
- **New physical comparison:** only after selecting a physical question, retain
  candidate digests, board identities, raw capture, timestamps and operator
  acceptance. New evidence must not rewrite the historical E016 gap.
- **Later generalization:** qualify a second profile before extracting a generic
  adapter system. Environmental prediction, calibration and closed-loop control
  need their own contracts and evidence.

This documentation pass authorizes none of those implementations, hardware
operations, source-control checkpoints, packaging or deployment actions.
