# Supported Feature Matrix

This document is the public support boundary for `fisiCs` as of the current
compiler-usability pass.

`fisiCs` is still an experimental compiler, but the project now has a clear
"default C first, extensions opt-in" contract.

## Baseline

| Area | Current public contract |
| --- | --- |
| Language baseline | C99-oriented compiler with active hardening toward broader C17-compatible behavior |
| Default mode | ordinary C compilation with no overlay features enabled |
| Primary trust model | fail-closed validation through `make final`, binary lanes, and real-project canaries |
| Intended current use | compiler experimentation, compatibility work, targeted real-project validation, and opt-in extension research |
| Production status | not yet a finished production compiler release |

## Core C Compiler Surface

| Surface | Current status |
| --- | --- |
| Single translation unit compile | supported and routinely exercised |
| Multi-translation-unit compile and link | supported and routinely exercised |
| Core lexer / preprocessor / parser / semantics / codegen | covered by the final test system and active real-project validation |
| Host-available standard-header runtime coverage | green through Wave `323` for the current host-available bucket-14 scope |
| Real-project validation | current validated project set records Stages `A` through `F` closed with no active open risk |
| Clang parity role | `clang` remains the reference control compiler while `fisiCs` hardens |
| Public practical canaries | `examples/canaries/` covers multi-TU compile/link, libc/string parsing, and finite numeric/math behavior |

## Preprocessor / Header / Runtime Support

| Surface | Current contract |
| --- | --- |
| Preprocessor | supported and broadly covered by dedicated lanes plus final manifests |
| Normal host headers | supported when available on the current host/toolchain |
| Host-available standard-library runtime behavior | actively validated through bucket `14` runtime/header manifests |
| Unsupported-by-host headers | not promised when the underlying host does not provide them |
| Current known host boundary | `uchar.h` and `threads.h` remain out of scope on the current host because plain host headers are absent |

## Compile / Link Behavior

| Surface | Current contract |
| --- | --- |
| `./fisics -c file.c -o file.o` | supported |
| `./fisics a.c b.c -o app` | supported |
| Include-path and library-path forwarding | supported through normal CLI flags such as `-I`, `-L`, and link args like `-lm` |
| LLVM-backed object generation | required part of the normal build |
| Final linking | supported for normal CLI flows; external system linkers may still be used for environment-specific cases such as SDL setup |

## Build Graph / Tooling Surface

| Surface | Current contract |
| --- | --- |
| Source-level dependency JSON | supported through the existing `--emit-deps-json <path>` / `EMIT_DEPS_JSON` lane |
| Source-level build graph JSON | initial v0 surface through `--emit-build-graph-json <path>` for a single source compile shape |
| Local project manifests | initial local-only v0 JSON parser through `--build-manifest <path>` |
| Dry-run build planning | initial non-executing v0 graph/plan JSON through `--build-manifest <path> --dry-run --json --emit-build-graph-json <path>` |
| `compile_commands.json` export | initial manifest-backed export through `--build-manifest <path> --emit-compile-db <path>` |
| Manifest-backed build execution | initial local-only execution through `--build-manifest <path>` for explicit TU/object/link manifests |
| Build graph diagnostic summary | compact additive `diagnostic_summary` counts and `partial` / `fatal` state; full diagnostics remain in the diagnostics JSON/pack lanes |
| Package fetching / registries | out of scope for the current local-only Phase 1 tooling lane |

## Behavior Policy

| Area | Current policy |
| --- | --- |
| Default language behavior | preserve ordinary C behavior unless the user explicitly enables an overlay |
| Undefined behavior | no promise of protective reinterpretation; users should assume ordinary C undefined-behavior rules still apply |
| Implementation-defined behavior | follows the selected target/data-layout and the underlying host libc/toolchain behavior where applicable |
| Diagnostics parity | improving, but not yet claimed complete across every surface |
| Stability promise | experimental project; user-facing docs try to describe the current trusted path, not freeze every internal behavior |

## Diagnostics And Explainability Surface

| Surface | Current contract |
| --- | --- |
| Emitted diagnostics JSON | additive taxonomy/stage fields are present for migrated compiler and direct driver/link/cross-TU diagnostics |
| Diagnostics pack | remains the compact pack lane; Phase 2 did not change pack row shape |
| Include-stack context | selected preprocessor-origin emitted JSON diagnostics can include structured `include_stack` frames |
| Macro context | selected macro-expansion emitted JSON diagnostics can include structured `macro_trace` frames |
| Diagnostic explanations | `--explain <diagnostic-code-or-name>` and `--list-diagnostics --json` expose a small stable explanation catalog |
| Physics-units details | selected units mismatch/conversion diagnostics can include structured `details` for dimension and concrete unit context |
| Runtime memory-check reports | optional `memory_check_report_v1` JSON sidecar through `FISICS_MEMCHECK_REPORT_JSON`; separate from compiler diagnostics |
| Build graph summaries | compact `diagnostic_summary` counts/state; full diagnostics remain in diagnostics JSON/pack lanes |

## Extension Boundary

| Area | Current contract |
| --- | --- |
| Overlay default | off |
| Current public overlays | `physics-units`, `memory-check` |
| Other overlay profiles | `ide-metadata`, `all` |
| `--overlay=all` policy | does not include `memory-check` because that mode rewrites allocator calls and links runtime support |
| Extension diagnostic policy | extension-owned diagnostics stay namespaced and do not silently become default-C semantics |
| Memory-check overlay | explicit opt-in allocation/free lifecycle diagnostics for direct `malloc`, `calloc`, `realloc`, and `free` calls |

## Known Gaps And Cautions

- `fisiCs` is not yet presented as a finished production compiler release.
- Public docs now describe the trusted path, but broad feature completeness is
  still subordinate to correctness and validation.
- Some environment-specific link flows still depend on the host toolchain and
  libraries rather than a single fully abstracted `fisiCs` release experience.
- Manifest dry-run planning is a read-only planning surface. It does not
  execute compile/link actions, fetch packages, resolve remote dependencies, or
  replace makefile flows.
- Manifest-backed `compile_commands.json` export is also read-only; it does not
  compile sources or perform final linking.
- Manifest-backed execution is intentionally minimal: it compiles listed local
  translation units to resolved object paths and links them when `link.output`
  is present. It does not fetch packages, generate lockfiles, run version
  solving, or replace makefile flows.
- `memory-check` is a runtime allocation/free diagnostics overlay, not a full
  AddressSanitizer replacement. It currently supports automatic exit reports,
  `FISICS_MEMCHECK_REPORT` policy controls, and source labels for direct
  compiler-rewritten allocator calls; general use-after-free reads/writes,
  redzones, custom allocators, allocator function pointers, stack tracking, and
  threaded guarantees are not current public claims.
- The full internal validation matrix is larger than the day-to-day public
  release checklist; use the checklist doc for contributor-ready checkpoints,
  not as a claim that every internal lane has disappeared.
