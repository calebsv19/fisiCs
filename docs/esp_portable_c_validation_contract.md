# ESP Portable C Validation Contract

Status: compiler-owned external intake, semantic, physical, and static hardware
composition, deterministic proof, retained realization comparison, and public
case study complete; historical raw-capture and Bench B deployment gaps remain
reported rather than inferred

This document defines how `fisiCs` may validate a small, production-used C
core owned by the `ESP_Work` repository. It is a compiler intake and evidence
contract. It does not make `fisiCs` an ESP firmware compiler and does not make
compiler output physical evidence.

The first intake is the E016 normalized-signal publication policy described by
`ESP_Work/docs/architecture/portable-embedded-c-core-contract.md`.

The live external source passes the owning repository's
`make test-portable-c-core` and `make emit-portable-c-metadata` targets. The
same source was built into identity-bound Bench A and Bench B candidates,
uploaded through the ESP-owned workflow, and accepted in a bounded recovered
CAN and potentiometer-response regression. These remain ESP-owned evidence;
no source has been copied into a permanent fisiCs regression corpus. The
owning digest, toolchain, upload, runtime, and physical receipt is
`ESP_Work/docs/architecture/portable-embedded-c-core-v1-evidence.md`.

## Purpose

The live validation lane proves that the exact portable C source used by an ESP
firmware composition:

1. satisfies its explicit deterministic vector corpus;
2. has matching host behavior under `fisiCs` and strict Clang;
3. passes the selected `fisiCs` semantic-metadata checks;
4. remains ordinary C accepted by the ESP vendor build; and
5. retains exact source and toolchain provenance across those claims.

The lane is intentionally analogous to the hardware-blind policy portion of
OS Policy validation. It does not inherit the OS lane's x86-64 object, linker,
QEMU, boot, or kernel claims.

## Current Target Boundary

`fisiCs` currently has a documented freestanding
`x86_64-unknown-none` object lane. This contract does not add an ESP32-C6
target, emit an ESP object, link firmware, contact a board, or flash a device.

The Espressif/Arduino toolchain remains the target compiler and linker. Its
successful build is separate evidence supplied by `ESP_Work`.

## Source Ownership And Intake

`ESP_Work` owns the production `.c` and `.h` files, their behavior contract,
the expected vectors, vendor build integration, firmware candidates, runtime
traces, and physical experiment records.

`fisiCs` owns only its compiler-facing runner, diagnostics, normalized result,
metadata export, and any later compiler-owned regression snapshot.

The first live intake must record:

- canonical source paths;
- SHA-256 of every compiled source and public header;
- portable-core contract version;
- vector-corpus version or digest;
- `fisiCs` identity and command;
- Clang identity and command;
- extension portability-header identity and digest; and
- the ESP vendor build receipt supplied by the owning repository.

An external path alone is not stable provenance. No source enters a permanent
compiler-owned corpus until the external behavior is accepted and the intake
records either a stable repository revision or immutable source digests. A
later snapshot must preserve its origin and must not silently become a second
production owner.

## First Accepted C Surface

The initial intake contains only two pure operations:

1. unsigned ADC-domain normalization into the inclusive `0..1000` semantic
   representation; and
2. a stateless initial/change/periodic/hold publication decision.

It does not include ADC reads, filtering, protocol encoding, sequence
mutation, CAN transmission, send-result commit, signal storage, TWAI recovery,
RGB behavior, serial output, allocation, interrupts, or scheduling.

## Required Host Oracle

The owning expected-vector corpus is the primary oracle. Clang parity supports
that oracle but does not replace it.

For the same source bytes and vector bytes, the runner must require:

- successful strict C99 compilation under Clang;
- successful compilation under `fisiCs`;
- exact expected exit status for each executable;
- exact expected output or structured result;
- byte-identical normalized results between compilers; and
- two identical runs per compiler when an artifact is retained as evidence.

Sanitizer execution may be added as supporting Clang evidence. It does not
replace either the expected vectors or `fisiCs` execution.

## Metadata Boundary

Shared source must use the public `FISICS_*` portability macros and the
canonical `<fisics/extensions.h>` contract. The portability header is a source
adapter only: it contributes no runtime implementation and gives the vendor
compiler no `fisiCs` semantics.

The first metadata pass establishes:

- a canonical authored identity for each exported policy operation; and
- mapped `produces` and `consumes` relationships to the protocol-owned semantic
  target through the authored cross-repository identity mapping.

The implementation does not invent a target identity from a C symbol, enum
spelling, GPIO, ADC channel, CAN identifier, or node name. The current mapping
is authored by `ESP_Work/protocol/schema/fisics_semantic_identities_v1.json`.

Translation-unit metadata JSON is the first required compiler artifact. The
semantic project, physical composition, and hardware compatibility graphs are
later bridge inputs, not prerequisites for the two-function host canary.

The normalized `0..1000` representation is dimensionless. No physical unit is
attached merely because its source was an ADC. A later volts, temperature,
distance, airflow, current, energy, or other physical quantity boundary must
use the physics-units contract separately.

## Extension Header Distribution

The ESP vendor build does not depend on an absolute path into a developer's
`fisiCs` checkout. `ESP_Work` vendors the canonical
`<fisics/extensions.h>` bytes under `CodeWorkEspCore` and requires exact parity
with the compiler-owned header before host validation or metadata export. The
vendored header and its digest are build inputs. Locally redefined lookalike
`FISICS_*` macros are not an accepted replacement.

## Evidence Separation

| Evidence | Owner | Strongest allowed claim |
| --- | --- | --- |
| expected-vector run under `fisiCs` | `fisiCs` lane | compiler-validated host behavior |
| expected-vector run under Clang | shared control lane | control-compiler host behavior |
| normalized result comparison | `fisiCs` lane | host differential parity |
| semantic metadata export | `fisiCs` lane | authored relationships accepted for current source |
| ESP vendor compile/link | `ESP_Work` | target firmware built |
| retained firmware manifest | `ESP_Work` | exact candidate identity retained |
| serial or CAN trace | `ESP_Work` | identified runtime behavior observed |
| numbered experiment acceptance | `ESP_Work` | scoped physical behavior accepted |

No row upgrades another row automatically.

## Compiler-Owned Validation Surface

The focused external intake is:

```sh
make integration-esp-portable-c \
  ESP_WORK_ROOT=/absolute/path/to/ESP_Work
```

The target consumes the fixed compiler-owned
`tests/fixtures/esp_portable_c/e016_intake_v1.json` contract. It rejects zero
selection, schema or digest drift, unsafe paths, a rewritten vector oracle,
mapping drift, stale candidate source receipts, and changed candidate
artifacts. It runs the six normalization and twelve publication vectors twice
under strict Clang and twice under fisiCs, requires exact output parity, emits
fresh translation-unit metadata, validates both manifest-v2 candidates, and
writes a deterministic scoped receipt under
`build/integration-esp-portable-c/`.

The ESP repository root is mandatory and must resolve to its exact Git top
level. The runner hashes every non-Git worktree entry, rechecks every selected
input, and compares the complete ESP Git status before and after execution. It
writes no external source or evidence file and does not invoke the ESP vendor
build, uploader, serial monitor, or hardware tools.

Compiler implementation changes require the normal confidence-tier widening.
Adding only a customer canary does not by itself justify changing parser,
semantic, code-generation, or target behavior.

The focused E016 semantic and physical composition lane is:

```sh
make integration-esp-semantic-physical \
  ESP_WORK_ROOT=/absolute/path/to/ESP_Work
```

It consumes the fixed external-intake selection and digest contract, then
regenerates semantic and empty units evidence from the current production C
source. It does not reclassify or refresh separately owned firmware-candidate
receipts. It resolves the two operation records through an explicitly allowed
protocol-owned external target and composes the target with a dimensionless,
scalar, uncalibrated physical contract. The semantic and physical graphs are
each emitted twice and must be byte-identical. Focused negatives reject stale
semantic provenance, stale units provenance, an unapproved external target,
and any change to the frozen physical interpretation. Authored inputs live under
`tests/fixtures/esp_portable_c/`; generated graphs and the deterministic
receipt live under `build/integration-esp-portable-c/`.

The focused E016 static hardware compatibility lane is:

```sh
make integration-esp-hardware-compatibility \
  ESP_WORK_ROOT=/absolute/path/to/ESP_Work
```

It regenerates Slice C, validates digest-pinned ESP board, CAN-bench, E016,
profile, and protocol declarations, then emits the hardware graph twice and
requires byte identity. The authored catalog and topology resolve the two
ESP32-C6 nodes, shared Classical CAN declaration, TJA1050 attachments,
potentiometer GPIO1 attachment, driver declarations, allocation claims, and
producer/consumer signal bindings. Focused negatives exercise exact board,
device, driver, bus, endpoint, allocation, required-port, and physical-contract
joins. Electrical facts unsupported by hardware graph v1 remain explicitly
outside the graph, and all live claims remain `not_assessed`.

## Initial Vector Requirements

The first corpus must cover at least:

- zero, midpoint, maximum, zero-maximum rejection, and above-maximum
  rejection for normalization;
- initial publication;
- movement immediately below and exactly at the threshold;
- elapsed time immediately below and exactly at the periodic interval;
- change precedence when both change and periodic conditions hold;
- unsigned 32-bit timestamp wrap;
- invalid boolean/configuration input; and
- repeated identical input producing identical decisions.

Sequence wrap, encoding failure, queue-full behavior, and send-result commit
remain outside this first corpus because the selected publication operation is
stateless and does not perform transport.

## Completion Gates

### Contract freeze

Complete when this document and the ESP-owned contract agree on ownership,
surface, oracle, provenance, and non-claims. No implementation is implied.

### Host canary

Complete when exact current ESP-owned source passes the vector oracle under
both compilers, produces matching normalized results, and emits fresh
translation-unit metadata where mappings have been frozen.

### Vendor adoption

Complete when the same source digest is built into an identified ESP firmware
candidate by the vendor toolchain.

### Physical regression

Complete only when the adopted candidate is separately uploaded, observed,
and accepted under the owning E016 experiment procedure. The compiler lane
does not perform or infer that step.

## Current Completion Readback

- Contract freeze: complete.
- Live external host canary: complete.
- Translation-unit semantic metadata export: complete.
- ESP vendor adoption and identity-bound candidates: complete.
- Retained Bench A upload evidence: documented consistently.
- Retained Bench B upload evidence: conflict reported between selected `0.2.0`
  upload and older `0.1.0` deployed descriptions; exact selected-pair linkage
  is not established.
- Recovered bidirectional CAN, semantic signal, and bounded potentiometer
  response: documentary summaries retained; selected raw serial capture is not
  retained in the evidence set.
- Compiler-owned `integration-esp-portable-c` target: complete.
- E016 semantic project graph: complete; both relationships remain explicitly
  external.
- E016 dimensionless physical composition graph: complete; the contract state
  is `external_declared` and physical acceptance is `not_assessed`.
- E016 static hardware compatibility graph: complete; declared topology is
  compatible, required-port coverage is complete, and all live claims remain
  `not_assessed`.
- E016 deterministic positive/negative proof: complete.
- Declared/deployed/observed comparison: complete with the Bench B and raw
  capture limitations above preserved as explicit non-claims.
- Project-neutral deployment/raw-observation receipt validator: complete; its
  checked-in fixture is synthetic and establishes no hardware fact.

The full path is documented in
`docs/esp_compiler_bridge_case_study.md`; new exact evidence bundles follow
`docs/external_evidence_receipts.md`. Completion of the compiler-owned rows
does not grant hardware authority or turn physical evidence into a static-graph
result.

## Explicit Non-Goals

- C++ parsing or compiling Arduino sketches with `fisiCs`;
- an ESP32-C6 backend, ABI, linker, SDK, or runtime;
- protocol, CAN-ID, pin, driver, or firmware generation from metadata;
- migration of the complete `CodeWorkEspCore` library to C;
- a generic embedded framework or RTOS abstraction;
- static graph output presented as runtime or physical proof; or
- release, deployment, upload, or physical-device authority.
