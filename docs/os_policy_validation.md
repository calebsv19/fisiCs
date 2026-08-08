# OS Policy Validation Contract

Status: OS-P3 probe and fix closure complete
Lane root: `tests/os_policy/`

## Purpose

The OS Policy lane (`OS-P`) gives `fisiCs` a compiler-owned trust surface for
freestanding C patterns used by operating-system policy code. It exists because
downstream `os-dev` proof is strong but cannot by itself provide a fast,
portable, compiler-owned regression boundary.

OS-P connects three distinct forms of evidence without collapsing them:

1. language and runtime semantics;
2. target object and ABI invariants;
3. downstream guest/kernel execution.

A pass at one layer does not imply a pass at another.

## Authority Boundary

Generated C may decide hardware-blind policy from bounded inputs. It does not
own interrupt state, page tables, privilege transitions, device I/O, atomic
locking, or resource authority merely because it emitted machine code.

Compiler-to-kernel metadata follows the same rule: it is a versioned request
that the loader/kernel may accept, narrow, reject, or safely ignore. OS-P will
not claim metadata validation until a real format and its rejection behavior
exist.

## Evidence Layers

### OS-P Object

`make os-policy-object` compiles every registered policy source twice for
`x86_64-unknown-none`, requires byte identity, and verifies:

- ELF64 little-endian System V relocatable output;
- `EM_X86_64`;
- required exports;
- exact undefined-symbol policy;
- bounded relocation types;
- no negative `RSP` access when red-zone use is forbidden;
- absence of declared privileged or architectural instructions.

Object checks assert invariants rather than one exact disassembly snapshot.

### OS-P Runtime

`make os-policy-runtime` compiles the policy kernel and its deterministic
driver separately with `fisiCs` and Clang on the validation host. Both runs
must match:

- explicit expected stdout;
- explicit expected exit status;
- each other exactly;
- the case's bounded vector contract.

Clang parity is supporting evidence, not the sole oracle. Each fixture must
also carry explicit expected results.

### OS-P Guest

`make os-policy-guest` now compiles each policy source independently with
`fisiCs` and Clang for `x86_64-unknown-none`, links each policy object against
the same Clang-built adapter and NASM runtime, and boots both raw images through
the same legacy-BIOS/long-mode harness.

For every variant, OS-P requires:

- byte-identical repeated policy objects and linked kernels;
- an exact boot-sector size and `0x55AA` signature;
- a bounded kernel sector count;
- two QEMU executions under the declared machine/CPU/memory/SMP contract;
- exact serial bytes and the declared `isa-debug-exit` process status;
- one serial-artifact SHA-256 across both compilers and all repeat runs;
- retained images, ELF kernels, link maps, serial logs, commands, tool
  versions, and hashes.

This proves execution in the minimal compiler-owned QEMU guest. It does not
claim real-hardware behavior or integration with the downstream `os-dev`
kernel.

### `os-dev` Canary

A later optional canary will compile the current external `os-dev` source using
temporary outputs and record the exact source revision. It must not replace or
bypass the OS repository's accepted compiler identity. The compiler-owned
stable suite remains self-contained.

## OS-P0 Baseline

OS-P0 establishes:

- schema-versioned `tests/os_policy/manifest.json`;
- fail-closed zero-selection and duplicate-id handling;
- canonical versus filtered report classification;
- `build/os_policy/latest-*.json` evidence;
- manifest/selector/path-containment contract tests;
- one `osp0_core_policy` case derived from the OS's range-admission,
  little-endian parsing, lock-order, and generation-token policy families;
- `make os-policy-object`, `make os-policy-runtime`, and `make os-policy`.

The baseline is intentionally small. Its job is to prove the lane contract
before breadth expansion.

## OS-P1 Guest Closure

OS-P1 establishes:

- explicit real-mode entry, segment and stack setup, A20 enablement, GDT
  ownership, protected-mode entry, identity paging, and long-mode transfer;
- assembly-owned COM1 serial and QEMU debug-exit operations;
- hardware-blind policy C;
- fisiCs-versus-Clang guest policy-object substitution behind a shared harness;
- exact exit-85 and serial-artifact parity with two runs per image.

## OS-P2 Current Families

The first OS-P2 family is `osp2_storage_admission`. Its 18 explicit vectors
cover:

- storage magic, version, block size, minimum-capacity, and journal bounds;
- reserved-range and overflow-safe extent admission;
- clean, dirty, committed, and durable-commit transition policy.

The family passes repeated object checks, exact fisiCs/Clang host behavior,
Clang AddressSanitizer/UndefinedBehaviorSanitizer execution, and both repeated
QEMU guest variants. Later OS-P2 families remain separately bounded.

The second family is `osp2_elf_admission`. Its shared `elf31-v1` corpus has 31
exact vectors for:

- decoded ELF64/little-endian/`ET_EXEC`/`EM_X86_64` header admission;
- bounded program-header-table geometry and user-entry bounds;
- ignored non-`PT_LOAD` records and accepted RX, RW, and BSS-only loads;
- overflow-safe file and virtual ranges, page-offset congruence, legal
  permission bits, and W^X rejection;
- the one-through-four load-segment count policy.

The same corpus is compiled into both host and guest adapters, preventing
their aggregate PASS transcripts from drifting onto different inputs. The
eleven-argument header API and seven-argument segment API additionally test
SysV stack-argument interoperability between the shared Clang guest adapter
and the compiler-variable policy object.

This is a reduced field-admission policy, not a complete raw ELF parser. It
does not claim pointer safety, segment overlap handling, executable-entry
membership, process mapping, rollback, page-table permissions, CPL-3 entry,
dynamic linking, ASLR, or general hardened loader correctness.

The third family is `osp2_job_admission`. Its shared `job27-v1` corpus has 27
exact vectors for:

- job header magic, version, type, checksum, and directory identity;
- nonzero job identifiers and bounded nonzero step counts;
- checkpoint, flags, result, compiler, and lesson fields;
- exact two-worker/two-page requests, available CPU/page capacity, and the
  minimum 832-byte XSAVE area;
- above-minimum capacity acceptance and source-order failure precedence when
  multiple fields are invalid.

The same corpus is compiled into both host and guest adapters. Its
eighteen-argument API adds deliberate SysV stack-argument pressure across the
fisiCs/Clang host differential and the Clang-adapter/fisiCs-policy guest
boundary. This is field and resource admission only: raw sector hashing,
hardware discovery, persistence, queueing, dispatch, and interrupted-running
recovery remain separate policy or assembly-owned boundaries.

The fourth family is `osp2_queue_transition`. Its shared `queue44-v1` corpus
has 44 exact vectors for:

- raw 512-byte queue metadata identity, version, fixed entry count/LBA, and
  full-record FNV-1a checksum admission;
- raw entry identity, version, one-through-four index bounds, checksum,
  pending-only state, result/compiler identity, cancellation, and resource
  availability;
- failure precedence across checksum, state, identity, cancellation, and
  resource decisions;
- persisted pending, running, complete, failed, cancelled, empty, and unknown
  state classification;
- terminal complete, compute-failed, format-failed, resource-failed, and
  cancelled decisions, including explicit lease-clear intent.

The same byte-building and decision corpus is compiled into both host and
guest adapters. Persisted `running` work is reduced to
`failed(interrupted)`, requests lease clearing, and never requests compute.
This does not claim ATA persistence, flush/readback, page lease correctness,
actual SMP compute, queue counters, or downstream two-boot recovery; those
remain assembly- and OS-owned boundaries.

The fifth family is `osp2_scheduler_transition`. Its shared `sched60-v1`
corpus has 60 exact assertions for:

- initialization and direct observation of both task states and all counters;
- bounded two-task yield and alternating timer selection;
- sleep transitions, before-deadline fallback, and exact-deadline wakeup;
- exit and fault transitions with runnable-task fallback;
- explicit terminated/faulted task reactivation and invalid-request rejection;
- unknown-reason handling and exact switch, yield, sleep, and preemption
  counter effects.

The same stateful corpus is compiled into both host and guest adapters. Local
BSS state accesses require `R_X86_64_32S`; that case-specific relocation was
independently reproduced by Clang. This family does not claim interrupt entry,
raw frame ownership, task stacks, timer programming, context save/restore, or
`IRETQ`, which remain assembly- and downstream-OS-owned.

The sixth family is `osp2_sync_rank`. Its shared `sync51-v1` corpus has 51
exact assertions for:

- the accepted two-CPU, two-update, one-AP-attempt contention observation;
- CPU-count, shared-trace, contention, held-lock, and IRQ-progress rejection;
- exact source-order failure precedence when multiple observations are invalid;
- the complete 5-by-5 held/requested rank matrix over values zero through four;
- maximum-unsigned held and requested rank boundaries.

The same corpus is compiled into both host and guest adapters. Queue rank `1`
may nest into PMM rank `2`, which may nest into device rank `3`; zero,
out-of-range, equal, and descending requests return the order error. This
family does not claim atomic acquisition, interrupt preservation, IPI
delivery, shared-state ownership, recursion, sleeping, or fairness, which
remain assembly- and downstream-OS-owned.

The seventh family is `osp2_pmm_extent`. Its shared `extent62-v1` corpus has
62 exact assertions covering one-through-sixteen-page request bounds, the two
valid owners, count-before-owner failure precedence, live six-page/two-handle
observations, contention/AP/zeroing fields, final free-count restoration,
deterministic reuse, generation change, IRQ progress, and the fixed
sixteen-page/eight-handle geometry selectors.

The same corpus is compiled into both host and guest adapters. It validates
hardware-blind transaction observations; allocator mutation, page and handle
state, token publication, stale-token enforcement, zeroing, locking, interrupt
state, and rollback execution remain assembly- and downstream-OS-owned.

The eighth family is `osp2_kernel_object`. Its shared `kobj51-v1` corpus has
51 exact assertions covering fixed 64-byte object size/alignment/capacity,
source-order request failures, live two-object/62-free observations,
contention and AP attempt fields, zeroing and one-page backing observations,
final cache readiness/backing ownership, deterministic reuse, generation
change, poison/IRQ completion, and fixed geometry selectors.

The same corpus is compiled into both host and guest adapters. Bitmap and
generation mutation, token encoding, pointer authority, zeroing, poisoning,
locking, allocation/free execution, and stale/double-free rejection remain
assembly- and downstream-OS-owned.

The ninth completed family is `osp2_process_lifecycle`. Its shared
`process34-v1` corpus has 34 exact assertions covering:

- legal create-from-empty and destroy-from-exited/faulted admission;
- task-0 ownership, one-slot presence, and complete opaque token/pointer pairs;
- active-versus-presented authority matching and stale replay after the active
  pair is cleared;
- the immutable EDU-19 totals of four creates, four destroys, two exits, two
  faults, and 32 reclaimed pages;
- independent PMM baseline, object-cache baseline, stale-release, and final
  empty-slot observations with explicit failure precedence.

The nine-argument final validator crosses the six-register SysV boundary in
both host and QEMU guest execution. Repeated fisiCs and Clang policy objects,
host output, sanitizer execution, four QEMU runs, exit `85`, and exact serial
SHA-256 parity pass. This is admission and observation policy only: allocation,
mapping, copying, zero-fill, page-table mutation, scheduler installation or
removal, actual reclaim, token mutation, CPL-3 execution, and faults remain
assembly- and downstream-OS-owned.

## Case Admission Rules

Every case must:

1. have a stable id and bounded purpose;
2. name its source snapshot and derivation;
3. be deterministic and avoid intentional undefined behavior;
4. declare exact runtime and object policies;
5. fail closed on missing inputs, missing exports, unexpected helpers,
   unexpected relocations, output drift, or exit drift;
6. remain hardware-blind unless the guest harness explicitly owns the
   architectural boundary.

Use Clang sanitizers to validate complex reference fixtures before treating
them as trusted oracles.

## Planned Policy Families

The ordered expansion after OS-P0 is:

1. ELF field admission — complete; raw-image parser/state deepening remains;
2. filesystem and durable-result parsing/building — initial family complete;
3. job type/resource admission — initial family complete;
4. queue transition and interrupted-running recovery — initial family complete;
5. scheduler state transitions — initial family complete;
6. synchronization rank policy — initial family complete;
7. PMM extent geometry and rollback decisions — initial family complete;
8. kernel-object generation/token validation — initial family complete;
9. scalar-double simulation ABI and deterministic reduction — strict and
   reduced cases complete;
10. protected-process lifecycle policy — initial family complete;
11. deterministic bounded mutation/property matrices — OS-P3 complete;
12. immutable post-EDU-19 policy-contract intake — active across EDU-23
    through EDU-41;
13. the optional current-`os-dev` canary.

## Scalar-Double Fix Closure

The strict `osp2_simulation_abi` family is derived from immutable EDU-12
`simulation_kernel.c`. Its shared `sim38-v1` corpus covers scalar-double
position/velocity/acceleration arguments and return, exact integration through
63/64/65 steps, the published `82.0` and `26.5` results, unsigned
rotate/add/xor boundaries, repeated calls, and the published
`0x6EC4E5DB9E1056CF` reduction.

The compiler now lowers scalar integer, floating, and pointer literal-zero
initializers as typed stores while retaining byte-wise zero-fill for
aggregates. The stable `13__ir_scalar_zero_init_no_memset` regression forbids
`llvm.memset` on the scalar path.

The strict and reduced cases pass exact fisiCs/Clang host parity, Clang
ASan/UBSan, repeated freestanding object inspection with no undefined helpers,
and repeated fisiCs/Clang QEMU execution. The guest harness retains its
manifest-gated assembly-owned SSE2 enablement path; all other cases remain
SSE-disabled by default. Canonical OS-P now passes all twelve registered cases.

## OS-P3 Stress Closure

OS-P3 expands the compiler-owned frontier with deterministic admission,
aggregate checkpoint, callback, queue, scheduler, synchronization, raw ELF,
raw job, and raw storage policy matrices. Its object lane also requires
freestanding aggregate zeroing, assignment, and indirect-return copies to
remain free of undefined `memset` and `memcpy` helpers.

The combined P3 probe frontier closes at `95 resolved / 0 blocked / 0 skipped`.
The stable torture-differential lane now represents the two reviewed
long-double ABI transcripts through exact `expected_stdout_variants` files,
while continuing to require fisiCs, Clang, and available GCC agreement. This
is a target-aware oracle, not a permissive output pattern.

## Post-EDU-19 Source Intake

OS-P3 closure predates substantial compiler-generated C growth in `os-dev`.
The stable OS-P manifest remains intentionally self-contained and its existing
provenance ends at EDU-19, so later behavior enters through bounded immutable
source snapshots before any optional live-repository canary.

### EDU-20 Through EDU-31 Crosswalk And Backfill

The historical coverage crosswalk is now complete for EDU-20 through EDU-31.
EDU-20 changed only assembly/platform-discovery behavior, so it has no honest
compiler-C probe. EDU-21 through EDU-31 are covered by eleven runtime matrices
with 574 deterministic vectors and eleven freestanding object probes:

- the exact EDU-21 `control_kernel.c` Wire-v1 validator, including its original
  six-operation ceiling, operation-three payload shape, reserved fields,
  checksum/error precedence, unsupported operations, and unaligned frames;
- the exact EDU-22 `queue_kernel.c` Queue-metadata-v3, Entry-v2, and Trace-v1
  validators, including event ordering, state-specific evidence, padding,
  resource/cancellation precedence, and the required validate-before-action
  caller sequence;
- EDU-23 generated-C parallelism admission, grants, deterministic partition
  values, path evidence, terminal state, and identity contradictions;
- EDU-24 assembly-derived result-artifact lookup, exact identity, bounded
  cursor, 512-byte chunk geometry, and final-marker validation;
- exact generated-C Wire-v2 through Wire-v7 history spanning EDU-24 through
  EDU-31, including cross-version rejection, payload bounds, reserved bytes,
  and checksum/error precedence;
- EDU-25 assembly-derived boot geometry and bounded EDD transfer plans;
- EDU-26 generated-C generation-safe queue reuse, tombstones, ACK identity,
  stale generations, and wrap rejection;
- EDU-27 phase/event ordering, width-dependent values, prefixes, trace
  histories, and impossible-state rejection;
- the exact EDU-28 artifact-metadata validator slice, including
  bitmap/count/length/checksum coherence and recomputed contradictions;
- EDU-29/30 activation and cooperative-stop behavior, including FIFO
  eligibility, exact runner identity, progress shape, and
  cancellation/deadline/budget precedence;
- EDU-31 calibration, checked nanosecond conversion, one-wrap raw deltas,
  monotonic reads, snapshot flags, and overflow rejection.

All eleven matrices pass strict C99 Clang diagnostics and
AddressSanitizer/UndefinedBehaviorSanitizer. The combined immutable intake and
composition lane resolves `48 / 48` runtime and object probes, and the owning
torture-differential gate closes with `0 failing / 10 expected skips`.

The assembly-derived models freeze only hardware-blind contract shapes.
BIOS/EDD calls, durable publication, BSP/AP execution, device authority,
interrupt timing, and QEMU behavior remain `os-dev`-owned proof.

The first intake wave uses exact `simulation_kernel.c` policy from immutable
EDU-32 commit `274f955` (source SHA-256
`7c1d57bb3020705a4796fc7a96b0153536deb4daac9dd4a9dfb9663a88ccec15`).
Its 24-vector runtime matrix checks:

- the exact 104-byte Workload-v1 identity and little-endian field contract;
- all six finite-double admission positions;
- zero and above-maximum step rejection;
- independently pinned partition and final reduction results;
- changed input/seed/result contradictions;
- unaligned byte input and the valid one-step boundary;
- exact fisiCs/Clang stdout, stderr, and exit parity.

The paired object probe requires deterministic freestanding ELF emission,
the three exact exports, no undefined helpers, bounded relocations, no red
zone, hardware-blind instruction policy, and case-scoped scalar SSE2. These
source-intake probes are frontier evidence; any compiler defect still requires
a minimized stable owner in the numbered bucket suite.

The second intake wave uses the exact hardware-blind Result-v1 validator from
immutable EDU-33 commit `49e430415a4ec2bfa45823d8ecd1a98b6a28dd51`
(`queue_kernel.c` SHA-256
`1f4e6ddb6b905619e5ca1f00f1e63c90d4e2818677ebe6cbf023f6f9ce9f43aa`).
Its 31-vector matrix covers both accepted and fail-closed boundaries:

- queue slots 0 through 7 and nonzero generation/request/workload identity;
- the exact 80-byte Result-v1 header and 512-byte sector geometry;
- nested-record and whole-sector FNV-1a integrity;
- zero flags, reserved fields, and padding through byte 507;
- unaligned byte input and maximum-width nonzero identity fields;
- opaque partition/final result bit patterns, including zero and NaN-shaped
  payloads, when structural integrity remains valid;
- identity/result mutations without matching checksum republication.

The paired object probe requires the one exact export, no undefined helpers,
`PC32`-only relocations, no red zone, and the hardware-blind instruction
policy. The focused EDU-33 gate passes `2 resolved / 0 blocked / 0 skipped`,
and the owning torture-differential bucket remains at `0 failing / 10 expected
skips`.

The third intake wave snapshots the complete immutable EDU-34
`queue_kernel.c` at commit
`bf95c670b576b3b1f494463d5ec41c5af19cb5bc` (source SHA-256
`d1b99e24c6e554207fb3e779e342a713ee996e1d74ebe01e3e98900d6e07591e`).
Its 30-vector matrix exercises the exact embedded queue-entry validator
across:

- pending and running deadline placement at offset 96;
- terminal duration relocation to offset 72;
- timeout, logical-budget, pending-cancellation, and running-cancellation
  evidence;
- timeout at phase prefix zero and after one completed phase;
- zero build-fixture duration and the exact 60,000,000,000 ns upper bound;
- unaligned entry input, stale entry version, state/layout contradictions,
  and recomputed outer checksums.

This is a compiler-owned C snapshot, not a replacement for EDU-34's assembly
safe-point or QEMU ordering proof. It also preserves one exact implementation
behavior for visibility: terminal reason comparison reads the low 32 bits of
the 64-bit offset-96 slot, so nonzero upper bits remain admitted by the frozen
validator. That is an upstream policy observation, not a fisiCs blocker or a
claim that the contradiction is fail-closed.

The paired object probe compiles the complete 21 KB source and requires all
five exports, no undefined helpers, `PC32`-only relocations, no red zone, and
hardware-blind instructions. The EDU-34 focused gate passes
`2 resolved / 0 blocked / 0 skipped`; the combined EDU-32/33/34 intake passes
`6 resolved / 0 blocked / 0 skipped`; and the owning bucket remains at
`0 failing / 10 expected skips`.

The fourth intake wave uses the exact EDU-35 checkpoint validator from
immutable commit `5b39037b8c0a84ef0a80441061b8ed20c3e47693`
(`queue_kernel.c` SHA-256
`c26fbc69f48064bfe7b1f5ff0aed1286884d98ba3c94d3b741d1dce208240b7e`).
Its 47-vector matrix covers:

- exact snapshot type, owner slot/generation/request, execution policy, and
  compiler policy identity;
- phase-prefix three, requested/effective work agreement, worker width, and
  the exact relative-duration upper bound;
- a valid embedded Workload-v1 baseline and three repeated partition-zero and
  partition-one observations;
- workload, 152-byte state, 236-byte logical-record, and whole-sector FNV
  integrity boundaries;
- zero padding through byte 507, unaligned input, and semantic contradictions
  whose enclosing checksums are republished;
- the exact ownership boundary where the C validator binds embedded workload
  bytes structurally while the surrounding OS path owns complete Workload-v1
  semantic admission and authoritative correlation.

This compiler probe does not claim snapshot publication ordering, addressable
owner lookup, crash-orphan handling, or resume prohibition; those remain
assembly/QEMU and OS-policy proof. The paired object probe requires the one
exact export, no undefined helpers, `PC32`-only relocations, no red zone, and
hardware-blind instructions. EDU-35 passes
`2 resolved / 0 blocked / 0 skipped`; combined EDU-32 through EDU-35 intake
passes `8 resolved / 0 blocked / 0 skipped`; and the owning bucket remains at
`0 failing / 10 expected skips`.

The fifth intake wave uses the exact EDU-36 durable queue-entry validator from
immutable commit `d0e429b02223884309d2cdaac9d8070d64b0f8c3`
(`queue_kernel.c` SHA-256
`a9221eab3ce57bdd4516429b82fb15690d7be37350489bdbc1841cd2cfac9b1d`).
Its 38-vector matrix covers:

- valid resumed `RUNNING`, `COMPLETE`, reboot-interrupted `FAILED`, timed-out
  `FAILED`, and running-cancelled durable entry forms;
- exact agreement between Entry-v8 `RESUMED=0x4` and Trace-v3
  `RESUME_RESTORED=19` evidence;
- restored phase-prefix three, direct reduce continuation, exact consumed-work
  accounting, and separation from ordinary setup/compute/barrier traces;
- nonzero opaque restore evidence, current-epoch ordering, timeout/cancel
  evidence, version and workload identity, checksum integrity, and unaligned
  entry input;
- fail-closed missing, mismatched, duplicated, reordered, or zero-valued resume
  evidence and state/flag/work contradictions.

The compiler-owned matrix validates durable C admission, not snapshot lookup,
exact request/generation correlation, fresh resource acquisition, deadline
arming, idempotent dispatch, ACK/reuse authority removal, persistence
ordering, or reboot execution; those remain `os-dev`-owned policy,
assembly/QEMU, and integration proof. It also preserves one exact upstream
implementation observation: the frozen validator admits a resumed Trace-v3
entry with any in-range trace flag set, including zero, while the EDU-36
resume contract says the new compact epoch carries all four
truncated/recovered/multi-epoch/evidence-gap flags.

The paired object probe requires the exact validator export, no undefined
helpers, `PC32`-only relocations, no red zone, and hardware-blind
instructions. EDU-36 passes `2 resolved / 0 blocked / 0 skipped`; combined
EDU-32 through EDU-36 intake passes
`10 resolved / 0 blocked / 0 skipped`.

The sixth intake wave derives a bounded compiler-side contract mirror from
immutable EDU-37 commit `30e34df8ee4ddbc6ba8917c138dbd3403572a288`.
EDU-37 adds no generated-C policy: `queue_kernel.c` is unchanged from EDU-36
at SHA-256
`a9221eab3ce57bdd4516429b82fb15690d7be37350489bdbc1841cd2cfac9b1d`.
The authoritative two-owner implementation remains `queue64.asm` at SHA-256
`5a38a3b75a7ac8ed54821c606fb3a44fce3462b52b3ddd302a474512194938bb`.

The 36-vector compiler matrix links the mirror against the exact inherited
Workload-v1 and Snapshot-v1 validators and covers:

- two all-zero lanes, either single occupied lane, and two distinct owners;
- fail-closed partial-zero, corrupt, semantically invalid nested workload, and
  duplicate exact-owner records;
- exact-owner reuse before allocation, then lowest empty lane, then lowest
  reclaimable lane;
- protection of exact `RUNNING` owners and exact
  `FAILED/INTERRUPTED` prefix-three owners;
- reclamation after terminal state, queue emptying, generation/request
  mismatch, wrong interruption reason, or a non-three phase prefix;
- fail-closed two-protected-owner capacity and out-of-range owner identity;
- unaligned two-sector and eight-entry storage.

This is explicitly a compiler test of the frozen contract shape, not a claim
that the C mirror is OS source or that it proves ATA ordering, boot admission,
queue mutation, lookup addressability, persistence, reboot, or QEMU behavior.
The paired object probe requires the two mirror exports, only the exact two
inherited validator imports, bounded `PC32`/`PLT32` relocation, no red zone,
and hardware-blind instructions. EDU-37 passes
`2 resolved / 0 blocked / 0 skipped`; combined EDU-32 through EDU-37 intake
passes `12 resolved / 0 blocked / 0 skipped`; the complete bucket-15 probe
inventory passes `650 resolved / 0 blocked / 0 skipped`; and the stable owning
bucket passes `0 failing / 10 expected skips`.

The seventh intake wave anchors immutable EDU-38 commit
`59d622af0278e0a57a36285ea7b75839a352bc41`. The complete
`control_kernel.c` body is embedded byte-for-byte at SHA-256
`83fe45bc8023c0352e91cb63a30a7fcd9ed0846683699970acce6b0483aac86d`.
Its 56-vector matrix covers:

- exact Wire-v13 admission and fail-closed Wire-v12/Wire-v14 rejection;
- all twenty operation payload shapes, source-order checksum/format
  precedence, bounded identifiers, and zero reserved/padding bytes;
- worker, workload, duration, queue-index, trace-index, and artifact-chunk
  bounds;
- maximum-width request identity and unaligned request frames.

The paired exact-source object probe requires all three control-policy
exports, no undefined helpers, `PC32`-only relocations, no red zone, and
hardware-blind instructions.

EDU-38's runner contexts are assembly-owned, so the second half of the wave is
explicitly a compiler-side contract mirror rather than copied OS source. Its
45-vector matrix covers two fixed 160-byte context identities, slot-modulo-two
selection, zero or exactly one active record, duplicate-active corruption,
active identity/resource shape, retained terminal evidence, checkpoint lanes,
bounds, isolation, and unaligned records. The object probe requires exactly
the three mirror exports with no imports. The active execution limit remains
one; phase/result ownership and the AP mailbox remain singleton OS state and
are not claimed as instanced by these probes.

EDU-38 passes `4 resolved / 0 blocked / 0 skipped`; combined EDU-32 through
EDU-38 intake passes `16 resolved / 0 blocked / 0 skipped`; and the complete
bucket-15 probe inventory passes `654 resolved / 0 blocked / 0 skipped`.
Both runtime matrices also preserve their pinned transcripts under Clang
AddressSanitizer/UndefinedBehaviorSanitizer execution.

The eighth intake wave derives a compiler-side contract mirror from immutable
EDU-39 commit `6dd5cd27fe31221934d751220d5b36fa3f714b50`. EDU-39
adds no generated C and leaves Wire v13 unchanged. The authoritative phase
ownership implementation remains `smp64.asm` at SHA-256
`f0137844e6c666798e5af1b16cdc002d8d4a7f80370492f5bd4adee53c4fa1d9`.

Its 70-vector matrix covers:

- two independent 224-byte saved-owner records with immutable context IDs;
- exact queue slot/generation/request and complete Workload-v1 correlation;
- phase/reduction, width, length, checksum, workload semantics, retained
  invalid evidence, opaque results, and unaligned record boundaries;
- valid width-one, width-two, and resumed joined-barrier publication-counter
  shapes;
- saved width-two COMPUTE admission but fail-closed loading;
- rejection of both in-flight and completed-but-unjoined different-owner
  switches before the selected owner crosses its barrier.

The paired object probe requires eight exact mirror exports, only the inherited
`edu32_workload_valid` import, bounded `PC32`/`PLT32` relocation, no red zone,
and hardware-blind instructions. Two valid saved owners remain legal; they do
not mean two simultaneously active runners. The active limit, live SMP
working set, AP work mode/generation/completion/error state, and wakeup
protocol remain singleton and OS-owned.

EDU-39 passes `2 resolved / 0 blocked / 0 skipped`; combined EDU-32 through
EDU-39 intake passes `18 resolved / 0 blocked / 0 skipped`. Its runtime matrix
also preserves the pinned transcript under Clang
AddressSanitizer/UndefinedBehaviorSanitizer execution.

The sequential EDU-39 closeout replay passes the complete bucket-15 probe
inventory at `656 resolved / 0 blocked / 0 skipped` and the stable owning
bucket at `687 passed / 0 failing / 10 expected skips`.

The ninth intake wave derives a compiler-side contract mirror from immutable
EDU-40 commit `1efb1ac74e6b729982b8289abfc6879b13458b4b`. EDU-40
adds no generated C and leaves Wire v13 and every durable format unchanged.
The authoritative singleton mailbox implementation remains `smp64.asm` at
SHA-256
`94b80d346fdd94cc58c34f26c78bee561ae772dbe6faa36801d9ceedb4b21cd5`.

Its 78-vector matrix covers:

- the exact 112-byte volatile mailbox envelope and reset state;
- idle/begin preconditions, monotonic dispatch, final-generation admission,
  and generation-exhaustion rejection;
- exact queue-phase and reserved legacy dispatch-owner shapes;
- no-skip AP acceptance and distinct dispatch/completion owner tuples;
- complete successful publication and bounded partial publication only with
  a nonzero AP error and matching error generation;
- ownerless, stale, skipped, future, wrong-owner, partial-success, duplicate,
  and late-completion rejection;
- retirement that clears authority while preserving the last tuple and
  generation as read-only volatile evidence;
- width-one byte-for-byte mailbox silence and unaligned records.

The paired object probe requires nine exact exports, no undefined imports,
bounded `PC32`/`PLT32` relocation, no red zone, and hardware-blind
instructions. This remains a compiler test of the frozen assembly contract:
the mailbox is still singleton, the fixed AP is not instanced, and the
active-runner limit remains one.

EDU-40 passes `2 resolved / 0 blocked / 0 skipped`; combined EDU-32 through
EDU-40 intake passes `20 resolved / 0 blocked / 0 skipped`; the complete
bucket-15 probe inventory passes `658 resolved / 0 blocked / 0 skipped`; and
the stable owning bucket passes `687 passed / 0 failing / 10 expected skips`.
Its runtime matrix also preserves the pinned transcript under Clang
AddressSanitizer/UndefinedBehaviorSanitizer execution.

The tenth intake wave derives a compiler-side contract mirror from immutable
EDU-41 commit `695ec663cb419c1b6604c9fe8777a48ceb81d5cb`. EDU-41
adds no generated C and leaves Wire v13 and every durable format unchanged.
The authoritative two-active selection and mailbox-pinning implementations
remain `queue64.asm` and `smp64.asm` at SHA-256
`1b3e3b1b59f0110d8aae3f7be7d393c45c8514d0dc16804893b3b8175de85ec8`
and
`c044735c0a5bc462b7aa6ea7dc2f914c924ec35a29261978496785a38bb78f24`.

Its 79-vector matrix covers:

- zero, one, and exactly two active 160-byte runner records, malformed flags,
  and immutable context IDs;
- exact RUNNING slot, entry-address, queue-generation, and request
  correlation before an activation scan may continue;
- FIFO pending selection only when the slot-modulo-two context is inactive,
  including ineligible-owner skipping, two-active busy status, and
  third-activation rejection;
- idle-mailbox rotation between eligible owners and exact in-flight mailbox
  pinning that prevents the peer from stealing a width-two joined barrier;
- exact owner-local phase release and retirement that leaves the peer record
  byte-for-byte unchanged;
- cancel-before-deadline-before-budget-before-work precedence, bounds,
  corruption rejection, and unaligned records.

The paired object probe requires nine exact exports, no undefined imports,
bounded `PC32`/`PLT32` relocation, no red zone, and hardware-blind
instructions. It does not model a third runner, second mailbox, per-CPU run
queue, migration, preemption, device authority, or a general symmetric
scheduler.

EDU-41 passes `2 resolved / 0 blocked / 0 skipped` and its matrix preserves
the pinned transcript under Clang AddressSanitizer/UndefinedBehaviorSanitizer
execution. The owning stable bucket passes
`687 passed / 0 failing / 10 expected skips`. Across the combined
EDU-21-through-EDU-41 intake plus the composition slices, all 48 selected
runtime/object probes resolve in one aggregate replay.

The first two-owner composition slice links only the frozen EDU-40/41
mailbox/runner helpers. Its 36-vector matrix covers exact two-active identity,
singleton-mailbox pinning, wrong-owner dispatch and completion rejection,
independent cancellation/deadline/budget/corruption outcomes, exact phase
release, unaligned records, and retirement that leaves the peer byte-for-byte
unchanged. Its object probe permits only the ten named EDU-40/41 helper
imports.

The durable-owner-chain composition slice now joins the frozen EDU-26
generation/reuse, EDU-35 snapshot, EDU-37 checkpoint-store, EDU-39 phase-owner,
EDU-40 mailbox, and EDU-41 runner helpers. Its 46-vector matrix separates four
subwaves:

- owner-local retirement, exact ACK identity, safe next-generation admission,
  and byte-for-byte peer preservation;
- valid two-lane checkpoint storage, exact lane selection, and correlation to
  the active runner identity;
- exact phase-owner, running-context, workload, and mailbox-owner correlation;
- the complete metadata-to-generation-action-to-checkpoint-to-phase-to-mailbox
  dispatch chain, with stale generation, request, owner, workload, checksum,
  and dispatch contradictions rejected independently.

The paired object probe requires four exact composition exports and permits
only the fourteen named EDU-26/35/37/39/40/41 helper imports. The matrix passes
strict C99 Clang diagnostics and AddressSanitizer/UndefinedBehaviorSanitizer;
the focused pair resolves `2 / 2`; the full post-EDU-19 aggregate now resolves
`52 / 52`; and the owning torture-differential replay closes at
`0 failing / 10 expected skips`.

The stable promotion checkpoint is
`15-torture-differential-wave140-temporal-stable-oracles.json`. It adds four
reduced deterministic temporal oracles with exact stdout contracts and
reference-compiler parity: checkpoint-recovery identity, stale ACK rejection,
completion/retirement order, and peer-preserving owner loss. The refreshed
audit records `3577` mapped promotions, `21` intentional probe-only entries,
`0` missing candidates, `0` critical integrity errors, and `0` ambiguous stem
matches; the stable final suite contains `4988` tests, and its monitored
checkpoint closes at `4988` passes, `0` failures, and `36` expected skips.
The large temporal source matrices remain explicitly probe-only because these
smaller stable oracles own the compiler behavior.

One boundary remains explicit: the EDU-26 action helper binds request identity
only when an ACK is prepared. In the complete chain, checkpoint, phase,
runner, and mailbox correlation provide the required request binding before
dispatch. This probe remains hardware-blind and does not claim durable write
ordering or physical BSP/AP execution.

The next behavior-focused boundary is cross-model temporal composition: take
the now-stable temporal predicates through independent scheduler, queue, and
wire-history model boundaries, preserving the same fail-closed stale-evidence
and peer-preservation rules. New scheduler, transport, capacity, firmware, or
hardware claims remain separate frozen OS lessons.

The optional live `os-dev` canary remains a separate noncanonical layer. It
must use temporary outputs, record the exact external revision and dirty
state, compare candidate and accepted compiler identities, and never bypass
the downstream accepted-toolchain hash.

## Failure And Promotion Workflow

When OS-P exposes a defect:

1. preserve the exact OS-P case, report, and command;
2. classify the compiler stage and owning numbered bucket;
3. reduce the defect into a focused probe;
4. add the active blocker to the private active-only ledger;
5. stop probe mode before changing compiler/runtime implementation;
6. fix one blocker family in fix mode;
7. promote the stable reduced oracle into `tests/final/` or `tests/binary/`;
8. rerun the OS-P source scenario;
9. reserve `make final-monitored` for the meaningful promotion checkpoint.

OS-P is a composition lane, not bucket `16`. Language-level defects continue
to belong to the existing numbered compiler buckets.

## Verification Ladder

```sh
make
make integration-x86_64-freestanding-object
make os-policy-object
make os-policy-runtime
make os-policy-guest
make os-policy-contract
make os-policy
make test
```

Canonical `make os-policy` uses the runner's `--continue-on-failure` mode. It
executes every registered case, retains every per-case outcome in the JSON
report, and still exits nonzero if any case fails. Focused runs remain
fail-fast.

Add the owning bucket/probe commands when a case is introduced or reduced.
Use `make final-monitored` at a broad promotion checkpoint, not for every
policy-fixture edit.
