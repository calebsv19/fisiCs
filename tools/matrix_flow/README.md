# Experimental bounded Structural matrix-flow qualifier

This optional tool checks a selected native SI, non-corotational, already-reduced-owner
mass path from actual resolved C source. The selected integrate endpoint and
cancellation callback are null (the unforced path). It is separate from ordinary compilation
and from the existing physical-semantics strict report. It performs no runtime
instrumentation, source rewriting or compiler selection.

The frontend adapter must be linked against a fresh frontend archive from the
same compiler checkout. Build it explicitly with `build_adapter.py
--compiler-root ROOT --frontend-archive ARCHIVE --frontend-receipt FRONTEND_RESULT_JSON --out FRESH_DIRECTORY`.
The adapter accepts `SOURCE FRESH_JSON INCLUDE...` and emits resolved declaration
identities, dimensions, C structure and include dependencies. The CLI sets
`FISICS_AST_SOURCE_ROOT` to export only core-owned implementations and the input
file itself; all include dependencies remain in the readset. SDK implementations
are external trust and are never inferred through an exported body. Its internal AST
schema is experimental and has no compatibility promise.

Run `cli.py --adapter ADAPTER --compiler-root ROOT --core-root CORE_STRUCTURAL
--readset READSET_JSON --adapter-receipt ADAPTER_RESULT_JSON --admit-planar-si-exclusive-owner --out FRESH_DIRECTORY`.
The reviewed readset is a JSON list of all absolute input and include paths from
the adapter's dependency exports. A new dependency refuses qualification until a
fresh readset is prepared. Duplicate cross-TU function names must have identical
resolved source bodies after normalizing only transient AST identities. Source, tool and dependency hashes accompany each
report, and before/after hashes guard the readset.

The bounded instance has eight nodes, zero joints, seven active members from
eight original members (member3 already absent), and sixteen fibers, with node0
fixed. It uses direct_frame=false and24 active DOFs in27-DOF-capacity storage. Existing model/settings validity and successful
solver ports are admissions, not conclusions of the checker. Failure triggering and material-law acceptance are outside this input fixture.
A separate rotation-map range/role check
covers all 72 admitted node/joint/end-count combinations. Model topology and
basis/material meaning, valid finite SI inputs, a live exclusively owned owner,
valid disjoint C subobjects, explicit local/output/owner mass layouts and planar basis zero representation,
existing kinetic/damping/work quantity-kind/DOF interfaces and
successful solver displacement/force ports remain authored admissions. Native C math contracts for hypot/sqrt/fabs/fmax remain explicit external trust;
SDK inline bodies are excluded from inferred source implementations. Compiler
infinity constants and standard fresh/disjoint allocation/free remain explicit
external contracts. The
cancellation callback must be null. No intervening external owner operation is
covered. Cross-target layout and concrete-unit scale propagation are refused. Index
arithmetic uses only bounded nonnegative integers up to 4096, ordinary int or
the pinned native SDK size_t. Narrow/unknown typedef conversions and non-double floating storage are refused;
double quantities retain dimensions without evaluating nonzero floating values.
Only exact structural literal-zero facts are retained; nonzero spellings are not
rounded to zero by the host. C short-circuit and boolean/return conversions are
checked. An annihilated expression supplies no live provenance even when its
floating value is unknown (for example because of overflow or NaN). Typed zeros retain their
dimensions. New local dimensional or concrete-unit restoration is refused.

The tool interprets actual constructor/transform bodies, assembly/map writes,
predictor/state updates and selected later consumer reads. All four actual
momentum contractions are checked before scalar energy contraction. Decay rate
cannot replace angular rate, and scalar energy cannot replace a generalized
torque. Typed quantities multiplied by zero retain their dimension/role; only
explicit layout/basis zero admissions can initialize a new storage meaning. It audits matrix and
owner capability uses, read-only solve input and the closed evaluate callback
forwarding. Unknown aliases, casts, calls receiving tracked capabilities,
unselected matrix/state writes and resource exhaustion refuse the path.
Repeated scalar calls and fresh 36-cell constructor outputs may reuse a summary
only after the actual source body has been checked for the same complete abstract
inputs and output layout. Reports count source-summary reuse separately; a
declared interface is never substituted for this body check.

Tracking is finite: 729 cells per array, bounded loops, call depth and two million
analysis steps. This is not general alias or whole-program inference.

Reports separate expanded storage-effect checks, source sites and explicit zero
admissions. These counts are not the ordinary compiler's required/checked/trusted
physical-obligation denominator. Full dispatcher/integrate strict reports retain
their unsupported rows. A passing path establishes dimensional/DOF provenance
under its explicit roots; it does not establish coefficient accuracy, general
quantity-kind inference, conservation, solver convergence or physical acceptance.
Use independent numerical, mutation, compiler and performance qualification
before treating any candidate as qualified. Default compilation never invokes
this tool. Canonical adoption and consumer adoption are separate decisions.

## Soundness restriction and regression gate

The selected constructor, transform, assembly, predictor, kinematic and consumer
regions are interpreted. Their surrounding control flow and ten owner/support
routines must match a fixed reviewed AST envelope. The interpreted loops must
occupy their reviewed execution positions. This prevents early success, skipped
regions, reordered kinematics, later overwrites and disconnected publication from
passing through source-region selection. This restriction is not general CFG
verification: even harmless unreviewed envelope edits can refuse. Do not regenerate
pins from a candidate to obtain acceptance. Authored successful solver ports and
external implementation contracts remain assumptions.

Transient AST addresses are normalized while preserving declaration/use binding
relationships. Function return types and long-double flags are exported by the
adapter; build a fresh adapter after this change. Ambiguous writes are weak
updates: a possibly targeted uninitialized cell remains uninitialized. Reported
lineage is a set of possible live input origins; it is not an algebraic sensitivity
proof or a statement that every matrix cell is nonzero.

Run the explicitly invoked source-mutant regression gate with a fresh output:

```sh
python3 -B tools/matrix_flow/check_soundness.py \
  --adapter ADAPTER --adapter-receipt ADAPTER_RESULT_JSON --compiler-root ROOT --core-root CORE_STRUCTURAL \
  --out FRESH_DIRECTORY
```

The adjacent manifest retains positive controls and broken-flow mutations. Each
negative must pass the C frontend and then be refused by the qualifier. The gate
also checks transient identity normalization and preservation of distinct bindings.
Default compilation and the ordinary strict-coverage denominator remain unchanged.

## Bounded report-cost implementation

A per-analysis cache holds at most4096 successful immutable plain-scalar/index
operations, keyed by operation, both complete abstract values and unary range
context. Matrix coefficients and generalized-rate operands bypass it, so every
momentum contraction still performs its source check. Refusals are not cached,
and capacity exhaustion falls back to ordinary checked evaluation. No source
body, storage read/write, layout admission, resource step or proof obligation is
skipped. Singleton storage reads keep their normal bounds/initialization checks
and avoid constructing a join; immutable coefficient values avoid redundant
copying when their role is already unchanged.

Run `python3 -B tools/matrix_flow/check_arithmetic.py` alongside the source-mutant
gate after modifying this path. Full fresh export/report cost remains a separate
qualification; a cache-control pass is not a timing or solver claim.

## Maintained explicit workflow

The adapter requires native declaration-origin/access-type interfaces in the
selected compiler source. Those native interfaces are a separately owned
prerequisite; adding this tool does not make an older compiler support them.
The ordinary compiler makefile never includes this tool's Makefile.

From a matching compiler source tree, invoke:

```sh
make -f tools/matrix_flow/Makefile check \
  COMPILER_ROOT=/absolute/matching/compiler/source \
  CORE_ROOT=/absolute/reviewed/core_structural \
  OUT=/absolute/fresh/output
```

This builds a contained frontend/adapter, verifies the reviewed168-file source
cohort and84 include inputs, prepares a fresh contained snapshot/readset, writes
the report and runs106 source cases, two binding controls,4245 cache checks and
the finite operational controls, including malformed receipt shapes, legacy-record
refusals, fixed build roots and real emitted dependency omissions/drift. Every physical negative must pass the frontend
and fail in the qualifier. The declared unknown-producer predecessor case now
has a valid C declaration. Reviewed input hashes/envelope pins are never refreshed
automatically. Requiring a different cohort is a separate review.

`build_frontend.py` uses fixed native/shared roots and an allowlisted build
environment; caller Make, CPP, SDK and shared-root overrides are not inherited.
Selected absolute tool paths, their hashes/versions, SDK/resource/LLVM include
roots and flags are recorded. Native and trusted toolchain headers are hashed
before compilation; real compiler-emitted dependencies (including system headers)
must match those bytes before a passed receipt is issued. Unexpected dependencies
or drift refuse the build. Tool discovery still uses caller PATH and selected
toolchain installations remain explicit external trust, not a supply-chain proof.

`build_frontend.py` produces a source/configuration/dependency/archive receipt.
`build_adapter.py` verifies it and produces an adapter receipt. The report CLI
requires `--adapter-receipt` and verifies binary, adapter-source and native-source
identities. Build records use experimental v2 schemas; older v1 records require a fresh
build. Receipts must be JSON objects with supported field shapes. Malformed
values produce controlled refusal records; existing outputs remain protected.
These local experimental build records detect drift/mixed inputs;
they are not signatures or protection against an operator forging both artifacts
and records. The linked archive is checked at build time; a static adapter no
longer needs that archive to exist when analyzing source.

For repeated analysis, build/prepare once with the workflow, then use its printed
command manifest for `cli.py`. The separate1s/256MiB qualification measures the
entire CLI, including fresh SDK/resource discovery, all five exports, provenance,
hashing and report writing. Native bootstrap compilation and the adversarial
suite have separate build/test costs; they are not hidden in a report timing.

Outputs must be fresh. Input/provenance/export/qualifier failures return nonzero;
created report outputs retain `accepted:false`, stage, diagnostic and available
first-loss location. Existing output paths are refused without overwriting them.
Build failure logs and workflow command results remain in the selected output.
This tool does not remove successful output automatically: the caller inspects,
keeps compact evidence and removes only its own disposable root after checking
consumers. No default compiler, app, solver, installed baseline or release changes
follow from this command. Internal AST/report/build schemas remain experimental.


## Supported developer delivery

The release archive includes an optional matching source kit under `developer/`.
Extract its `fisiCs-matrix-flow-source-*.tar.gz` into a fresh directory and start
at `SOURCE_KIT.md`. The kit includes compiler/frontend/adapter sources, portable
units examples, a small physical policy and this tool. It contains no SDK or
private solver fixture. Python3.10+, Apple SDK command-line tools and LLVM22
development libraries are required for this experimental macOS workflow.

Select the intended LLVM22 `bin` directory in PATH. Run the explicit preflight:

```sh
python3 -B tools/matrix_flow/preflight.py
```

The workflow reports its selected LLVM installation and uses that installation's
matching Clang resource headers in child analysis commands. It does not change
the calling shell or a default compiler. Unsupported LLVM versions refuse before
building. Native compilation tools and actual dependencies remain separately
bound by the frontend/adapter receipts. Input preparation still checks every
reviewed source/header digest; preflight does not authorize new header bytes.

Build a matching source kit independently with the explicit optional target:

```sh
make -f tools/matrix_flow/Makefile source-kit COMPILER_ROOT="$PWD" OUT=/new/absolute/kit
```

From a clean extracted kit, build the CLI/frontend in fresh outputs with
`build_frontend.py --compiler-root "$PWD" --out /new/absolute/build --with-compiler`.
Then run `walkthrough.py --compiler-root "$PWD" --compiler /new/absolute/build/fisics
--out /new/absolute/walkthrough`. This checks ordinary runtime, a valid physical
flow, wrong-kind rejection and visible unknown reporting. Unknown output remains
unproved. The full matrix-flow check additionally requires the exact separately
supplied reviewed `core_structural` directory and sibling `core_base` inputs:

```sh
make -f tools/matrix_flow/Makefile check COMPILER_ROOT="$PWD" CORE_ROOT=/reviewed/core_structural OUT=/new/absolute/check
```

The kit manifest records relative paths and SHA256 identities. It is a bounded
developer source kit, not the full contributor test checkout. Use the normal
release archive for the ready-built ordinary CLI. Optional developer preparation
and report time are distinct; ordinary compilation never builds or invokes this
qualifier. No pins, trust classifications or performance budgets are relaxed.
