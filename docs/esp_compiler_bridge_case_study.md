# ESP Portable C And Static Graph Case Study

Status: runnable compiler path implemented; historical realization mismatch
retained; no new bench run performed

This case study explains how `fisiCs` is useful to an ESP project without
becoming an ESP compiler. The E016 normalized-signal policy is ordinary C99
owned by `ESP_Work`. `fisiCs` validates the same C behavior on the host and
extracts optional semantic metadata. Espressif's Arduino toolchain still
compiles and links the firmware for ESP32-C6.

## How The Same C Reaches Both Systems

The portable header wraps its public functions in `extern "C"` when included
from C++. Arduino sketches can therefore call the C implementation without C++
name mangling. The header includes `<fisics/extensions.h>` and uses
`FISICS_ID`, `FISICS_PRODUCES`, and `FISICS_CONSUMES` on the two function
declarations.

When `fisiCs` preprocesses the file, `__FISICS__` is defined and those macros
expand to opt-in semantic attributes. When Clang or the Espressif/Arduino
toolchain compiles the same header, the vendored portability macros expand to
nothing. The function signatures and runtime behavior remain ordinary C; no
metadata runtime is linked into the ESP firmware.

The two operations are intentionally small:

- normalize a bounded ADC-domain value into the dimensionless `0..1000`
  representation;
- decide whether to publish because of initial state, movement, periodic time,
  or hold.

ADC reads, filtering, CAN encoding/transmission, sequence state, send-result
commit, GPIO, LEDs, and scheduling remain in the ESP-owned C++/Arduino layers.

## What The Compiler Actually Does

The bridge proceeds through distinct artifacts:

1. The external intake pins exact `.c`, `.h`, vector, mapping, candidate, and
   protocol hashes. It runs six normalization and twelve publication cases
   twice under strict Clang and twice under `fisiCs`, requiring exact results.
2. `fisiCs --overlay=semantic-metadata --emit-sema-json` parses the portable C
   declarations and emits translation-unit metadata for two operation IDs and
   their producer/consumer relationships.
3. The semantic project reader resolves those relationships to the authored
   protocol identity `signal.bench_potentiometer.test_level`.
4. The physical reader binds that identity to a dimensionless scalar,
   explicitly uncalibrated contract.
5. The hardware reader joins that contract to the authored ESP32-C6 nodes,
   CAN bus, TJA1050 devices, potentiometer, drivers, endpoints, allocations,
   and signal bindings.
6. The deterministic proof rebuilds each terminal graph twice and runs the
   positive and hostile negative corpus.
7. The realization comparison references retained candidate and ESP evidence
   without giving the compiler upload, runtime, or physical authority.

The established terminal graph hashes for the retained E016 projection are:

| Artifact | SHA-256 |
| --- | --- |
| semantic project graph | `0ccb73417bb08a09c79b87df4811010cd296060ed1b8ac034bd706f54543eb0d` |
| physical composition graph | `29bb340605e96b425cab36722537d9e1418b6948227a45eccb0701ee32788c76` |
| hardware compatibility graph | `e04b5cbdbbe97b6f3d3f86f11dc82252363d4a991f65a07b11489d5c42c36ee7` |

These are retained historical identities, not aliases for the current moving
ESP worktree.

## Reproduce The Compiler-Owned Path

Build the compiler, then point the fixed lanes at the exact ESP Git root:

```bash
make
make integration-esp-portable-c \
  ESP_WORK_ROOT=/absolute/path/to/ESP_Work
make integration-esp-semantic-physical \
  ESP_WORK_ROOT=/absolute/path/to/ESP_Work
make integration-esp-hardware-compatibility \
  ESP_WORK_ROOT=/absolute/path/to/ESP_Work
make integration-esp-deterministic-proof \
  ESP_WORK_ROOT=/absolute/path/to/ESP_Work
make integration-esp-realization-evidence \
  ESP_WORK_ROOT=/absolute/path/to/ESP_Work
```

The later targets validate their own prerequisites, so the final target is a
convenient full focused run. Generated artifacts live under
`build/integration-esp-portable-c/`. Every lane fingerprints and rechecks the
ESP worktree and must leave it unchanged.

For a new project, do not copy E016 IDs or topology. Author a new digest-bound
intake, semantic mapping, physical contract, component catalog, and topology
for that project, then use the same graph readers and evidence vocabulary.

## What Is Proven Today

The compiler proves that the pinned portable C behavior passes its exact host
oracle under both compilers, its metadata resolves, and the supplied static
hardware declaration is compatible. It also proves deterministic graph output
and rejection of stale provenance, identity conflicts, incompatible physical
meaning, hardware mismatches, allocation conflicts, missing bindings, and
attempted live-claim promotion.

The realization receipt preserves two historical limitations:

- retained reports disagree about whether the selected Bench B `0.2.0`
  candidate or the older `0.1.0` candidate was deployed;
- the retained reports summarize serial/CAN observations but do not retain the
  selected raw transcript, and the current protocol digest has moved beyond the
  historical graph input.

Therefore the exact selected-pair deployment/runtime linkage is not
established. The historical observation remains documentary evidence, not a
fresh observation and not a raw-capture-bound receipt.

## How To Improve The Next ESP Run

No new bench run is required for the compiler or static graph system. On the
next independently justified ESP upload/test—not as a special replay for this
documentation—the ESP lane should retain:

1. immutable candidate manifest, firmware artifact, and protocol snapshot;
2. one deployment receipt per USB-identified board;
3. the raw serial/CAN capture beginning after those deployments;
4. one observation receipt referencing both deployment receipts and exact raw
   line fragments; and
5. any operator physical observation as a separate labeled field.

Validate that bundle with
`scripts/validate_external_evidence.py`. This will make the next realization
comparison exact without increasing the compiler's hardware authority or
requiring routine bench work for compiler changes.

## Practical Refinement Boundary

The next useful software improvements are adoption work, not more compiler
syntax: have ESP-owned upload/capture commands emit these receipts directly;
preserve immutable protocol snapshots instead of relying only on a moving
`v1` path; and add a project-neutral comparison report that joins static graph
hashes to a validated external evidence-chain receipt. Physical reruns should
remain exception-driven by an exact missing candidate/deployment/capture link.
