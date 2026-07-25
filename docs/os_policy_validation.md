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
11. deterministic bounded mutation/property matrices;
12. the optional current-`os-dev` canary.

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
