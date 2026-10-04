# Frozen offline graph inputs

Status: implemented source-tool contract v1. Qualification: 2026-09-12.

This captures the **complete data-input closure of the v1 offline graph
pipeline**, then regenerates semantic, physical and hardware graphs from copied
bytes without reading the original checkout. It does not reproduce the earlier
C preprocessing/export build. That distinction is part of the contract, not an
optional disclaimer.

## What is frozen

| Input | Treatment |
| --- | --- |
| Semantic, physical and hardware manifests | Retain exact authored bytes and policies; derive separate execution manifests with snapshot paths |
| Translation-unit semantic exports | Retain exact bytes, original source identities and receipts |
| Units exports | Retain exact bytes and source receipts |
| C files named by those receipts | Copy bytes, check existing length/FNV receipt, add SHA-256 inventory |
| Physical catalogs | Copy every manifest-selected catalog |
| Hardware catalogs and topology | Copy every manifest-selected document |
| Authored attachment array | Copy exact bytes; validate declarations after graph generation |
| Compiler executable | Pin SHA-256; require that binary at replay, check before/after; do not copy or authenticate it |

The three offline readers were audited for file reads. Their v1 catalogs and
topology are terminal documents: they do not recursively import other files.
The source-receipt reads are included even though they are not top-level
manifest entries. Existing upstream graph paths in physical/hardware manifests
are replaced by freshly generated predecessor graphs, not captured as opaque
inputs. No generated metadata or graph JSON is rewritten to manufacture a pass.

The public witness has 14 inputs: three manifests, four metadata exports, two C
sources, three catalogs, one topology and one attachment array.

## Commands

From a source checkout, generate fresh exports and manifests first:

```sh
make
make examples-semantic-hardware

python3 -B tools/frozen_graph_inputs.py freeze \
  --compiler ./fisics \
  --semantic build/examples/semantic_metadata/hardware_project/semantic.manifest.json \
  --physical build/examples/semantic_metadata/hardware_project/physical.manifest.json \
  --hardware build/examples/semantic_metadata/hardware_project/hardware.manifest.json \
  --attachments examples/semantic_metadata/hardware_project/attachments.json \
  --out /absolute/path/to/new-snapshot
```

Record the SHA-256 of `new-snapshot/closure.json` as the expected snapshot
identity. On macOS use `shasum -a 256`; on Linux use `sha256sum`. Supply the
resulting 64-character digest, not the explanatory placeholder below:

```sh
python3 -B tools/frozen_graph_inputs.py verify \
  --snapshot /absolute/path/to/new-snapshot --expected-sha256 <closure-sha256>

python3 -B tools/frozen_graph_inputs.py build \
  --compiler ./fisics \
  --snapshot /absolute/path/to/new-snapshot --expected-sha256 <closure-sha256> \
  --out /absolute/path/to/new-result
```

Capture the SHA-256 of `new-result/build-receipt.json` for detached readback:

```sh
python3 -B tools/frozen_graph_inputs.py verify-build \
  --out /absolute/path/to/new-result --expected-sha256 <build-receipt-sha256>
```

`verify` and `verify-build` are read-only and run no compiler or model.
`freeze` and `build` require new directories and never overwrite an existing
result. Success returns JSON/exit 0; rejected input reports stderr/exit 2. A
failed operation can leave an incomplete new directory. Only the final
successful receipt establishes completion; never infer it from directory or
graph-file existence.

The tools require Python 3.9+ standard library and a compatible fisiCs source
build. Release archive inclusion and independent-host acceptance are not yet
qualified. Keep `attachment_handoff.py` beside `frozen_graph_inputs.py`.

## Snapshot schema and admission

`closure.json` is canonical sorted-key, indented JSON with a trailing newline.
It has exactly:

- `schema: fisiCs.frozen_graph_inputs` and integer `schema_version: 1`;
- `project_id` and `compiler_sha256`;
- `roots`: original absolute identities for `semantic`, `physical`, `hardware`,
  and `attachments`;
- `inputs`: entries with `identity`, `role`, canonical bundle-relative `path`,
  lowercase `sha256`, and integer `length_bytes`.

Noncanonical closure bytes reject at verification, even when their SHA-256 is
correctly supplied. Replay checks this before invoking the compiler or creating
its output directory; the writer's canonical encoding is the supported format.

Source identities remain the original canonical absolute strings; their bytes
are stored below `sources/<original absolute path without its leading slash>`.
Other inputs use `data/<SHA-256 of original identity>.json`. These are storage
names, not semantic identities. Duplicate identities, conflicting roles,
missing dependencies, extra inventory entries, unsafe paths, symlinks,
unsupported schemas, and stale source receipts reject. Verification walks the
captured manifests again and requires exactly the discovered inventory; merely
listing a set of hashes is insufficient.

Limits: 1,024 files, 16 MiB per file and 256 MiB aggregate captured data. This
source-tool intake is intentionally narrower than some compiler reader limits.
An over-limit input rejects; it is never partially captured or silently omitted.

The expected digest is explicit for verification/replay. Keep it in a trusted
caller or admission record. Recomputing a pin from an untrusted replacement
bundle establishes internal consistency only, not intended-input authority.

## Replay and source identity

The build copies the verified snapshot into its new result directory, generates
separate effective manifests, and runs each graph stage twice. Graph bytes must
match between those two executions at the same location. The generic attachment
verifier must then accept all declared attachments before `build-receipt.json`
is written last.

The new compiler environment variable `FISICS_GRAPH_SOURCE_SNAPSHOT` affects
only source-receipt reads in the three offline graph readers. It must name a
canonical absolute source-copy root. Each original absolute source identity
maps beneath that root. Missing, noncanonical, symlinked or mismatched copies
fail; there is **no live-source fallback**. Source identities, existing hash
checks and graph schemas are unchanged. Unset behavior retains ordinary
live-source verification. Do not export this variable globally; the tool sets
it only in its compiler subprocesses, with a minimal environment.

`build-receipt.json` uses `fisiCs.frozen_graph_build`, integer version 1. It binds
the closure, compiler, all graph bytes, effective manifests and handoff bytes.
The result retains the snapshot, three graphs, three effective manifests,
`handoff.json`, and this completion receipt. `verify-build` checks every retained
input, the expected build receipt, output digests and generic declaration joins.
It does not rerun compilation or authenticate the original execution.

The build receipt's evidence object has a fixed v1 vocabulary. Missing, extra
or changed claims reject, including attempted promotion to authenticated,
executed or physically accepted evidence. A matching digest alone does not
make an unsupported evidence claim valid. This is schema enforcement, not a
signature or proof against an attacker replacing an entire trusted bundle.

The included `handoff.json` remains version 1. Its ordinary verifier alone does
not establish frozen-input evidence: consumers requiring that evidence must use
`verify-build` with their expected build-receipt digest. Legacy handoff v1 and
optional producer-parent v2 retain their existing meanings; there is no silent
downgrade or reinterpretation.

## Portability, mutation and evidence limits

Captured bytes and inventories can be relocated and verified without original
paths. Replay works after the original source tree is unavailable. Original
source identity and authored bytes remain unchanged; effective manifests and
generated graphs embed their new execution paths. Consequently same-location
repeat bytes are deterministic, but different replay locations need not yield
the same graph hashes. Do not normalize or edit those outputs to pretend they
are byte-identical.

Capture checks originals again before finalizing; replay checks its copies
before every graph invocation and after the last. These are fail-closed drift
checks under a **single-writer, quiescent-input assumption**, not an atomic
filesystem snapshot or protection against a malicious process changing and
restoring bytes between checks. The closure and receipts are unsigned.

This contract does not capture headers, command-line macro definitions,
preprocessor configuration, or toolchain dependencies that produced the
translation-unit exports. It does not rebuild C, fingerprint dynamically loaded
libraries/the OS, prove a hermetic toolchain, or establish current live-source
freshness after capture. Exported metadata is the input boundary; matching C
source receipts are not proof of the complete earlier preprocessing environment.

No model or driver is executed. Sensor freshness, calibration, simulation
accuracy, wiring, firmware, deployment and physical acceptance remain separate
consumer or hardware evidence.

## Qualification and closeout

```sh
make test-frozen-graph-inputs
make test-attachment-handoff
make test
```

The frozen-input gate is also a prerequisite of `make test`.

The focused gate covers real compiler exports and detached replay, all input
classes including equal-length tampering, deterministic capture and repeated
graph bytes, relocated replay/readback, missing/duplicate/path/schema/source/
compiler rejection, capture and replay drift, source-map no-fallback behavior,
and create-only completion. Existing graph and attachment contracts continue to
pass without enabling snapshots.

This closes the bounded **offline compiler attachment construction and
verification milestone**. It is reasonable to pause this lane and use it from a
consumer. Further work should be driven by a concrete requirement:

1. Full C-to-export reproducibility needs a separately designed preprocessing
   dependency capture and toolchain/environment contract.
2. Distribution needs package inclusion, an isolated consumer test, and explicit
   release approval; authenticity needs its own signing/admission contract.
3. Broader physical admission needs concrete supported-unit, frame, reference,
   external-source or conversion cases, without relaxing current checks.
4. Native/ESP adoption needs consumer capability and execution evidence, not
   new compiler syntax inferred from device names.
