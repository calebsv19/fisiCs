# OS Policy Validation Lane

Status: OS-P2 deterministic family matrix complete; OS-P3 stress planned

The OS Policy lane (`OS-P`) is the compiler-owned trust boundary for
freestanding C patterns used by operating-system policy code. It complements,
but does not replace, the downstream `os-dev` build and QEMU proof.

## What This Lane Owns

OS-P owns deterministic, reviewable compiler evidence for:

- host-runtime semantic parity between `fisiCs`, Clang, and an explicit
  expected transcript;
- repeated `x86_64-unknown-none` object identity;
- ELF64/System V/`EM_X86_64` target identity;
- required exported symbols and bounded undefined-symbol surfaces;
- relocation allowlists;
- no red-zone access;
- instruction-policy bans for hardware-blind C.

OS-P does not claim:

- kernel or hardware correctness;
- SMP memory-model correctness;
- interrupt, page-table, port-I/O, or device behavior;
- real hardware execution or downstream `os-dev` kernel integration from the
  compiler-owned QEMU harness alone;
- acceptance of compiler-to-kernel metadata that has not been specified and
  implemented.

Assembly and the OS remain authoritative for architectural state. Compiler
metadata remains a bounded, rejectable request rather than machine authority.

## Slice Model

| Slice | State | Contract |
|---|---|---|
| OS-P0 | complete | manifest, runner, host differential, repeated x86 object inspection, one core policy case |
| OS-P1 | complete | minimal legacy-BIOS/long-mode QEMU harness with exact serial/exit/artifact parity |
| OS-P2 | complete | ten deterministic families complete; strict and reduced scalar-double cases pass every current tier |
| OS-P3 | planned | deterministic bounded mutation/property corpus and current `os-dev` canary |
| OS-P4 | future | first versioned compiler-metadata accept/narrow/reject/ignore contract |

## Commands

From the `fisiCs` repository root:

```sh
make os-policy-object
make os-policy-runtime
make os-policy-guest
make os-policy-contract
make os-policy
```

Focused investigation:

```sh
python3 tests/os_policy/run_os_policy.py --tier object --case osp0_core_policy
python3 tests/os_policy/run_os_policy.py --tier runtime --case osp0_core_policy
python3 tests/os_policy/run_os_policy.py --tier guest --case osp0_core_policy
python3 tests/os_policy/run_os_policy.py --tier all --case osp2_elf_admission
python3 tests/os_policy/run_os_policy.py --tier all --case osp2_job_admission
python3 tests/os_policy/run_os_policy.py --tier all --case osp2_queue_transition
python3 tests/os_policy/run_os_policy.py --tier all --case osp2_scheduler_transition
python3 tests/os_policy/run_os_policy.py --tier all --case osp2_sync_rank
python3 tests/os_policy/run_os_policy.py --tier all --case osp2_pmm_extent
python3 tests/os_policy/run_os_policy.py --tier all --case osp2_kernel_object
python3 tests/os_policy/run_os_policy.py --tier all --case osp2_simulation_abi
python3 tests/os_policy/run_os_policy.py --tier all --case osp2_process_lifecycle
```

The unfiltered commands are canonical for the current manifest. A filtered run
is useful evidence but is reported as noncanonical. Canonical `make os-policy`
adds `--continue-on-failure`, so every registered case runs and is recorded
before the aggregate result is returned.

Artifacts and the latest JSON report are written below
`build/os_policy/`. They record compiler/source/object identities, tool
versions, selection scope, commands, and per-case results.

## Guest Contract

OS-P1 owns a small, auditable harness under `guest/`:

- the boot sector establishes its own segments, stack, A20 state, GDT,
  protected-mode transition, three-level page tables, and 64-bit long mode;
- assembly owns serial I/O and QEMU debug-exit authority;
- policy C remains hardware-blind;
- one Clang-built adapter and one NASM entry object are shared between both
  images, while only the policy object changes between the fisiCs and Clang
  variants;
- each policy object and linked kernel is reproduced byte-for-byte;
- each image executes twice under `pc,accel=tcg`, `qemu64`, one CPU, and 16 MiB;
- exit status must follow `(debug_exit_value << 1) | 1`;
- every serial transcript must match the explicit expected artifact and its
  cross-compiler/repeat SHA-256 parity contract.

Guest images, linked kernels, maps, serial logs, and the latest report remain
under ignored `build/os_policy/` outputs.

## Case Contract

Each manifest case must provide:

- a stable case id;
- source and runtime-driver paths;
- an exact expected stdout file and exit code;
- provenance naming the originating policy family and source snapshot;
- required exports;
- allowed undefined symbols and relocations;
- explicit instruction bans;
- red-zone policy.
- an introduction slice and exact guest adapter, serial, exit, timeout,
  repetition, and parity-artifact contract.

Cases must be deterministic and free of intentional undefined behavior.
Reference fixtures should use fixed-width assumptions explicitly and should
gain a Clang sanitizer gate when their behavior becomes complex enough to make
fixture validity uncertain.

The `osp2_elf_admission` case uses one shared `elf31-v1` vector corpus for its
host and guest adapters. It covers decoded ELF64 header fields, bounded program
header geometry, user-entry bounds, load-segment file and virtual ranges,
permission masks, W^X rejection, page-offset congruence, ignored non-load
records, BSS-only loads, and the one-through-four load-count policy. Its
seven- and eleven-argument calls also exercise SysV stack arguments across the
Clang-adapter/fisiCs-policy guest boundary.

The `osp2_job_admission` case uses one shared `job27-v1` corpus for its host
and guest adapters. Its 27 vectors preserve the source policy's exact
fail-closed precedence across header identity, job shape, checkpoint/result
fields, compiler/lesson identity, resource requests, available-resource
bounds, and the minimum XSAVE size. The eighteen-argument policy call also
exercises a deeper SysV stack-argument boundary in both host binaries and the
Clang-adapter/fisiCs-policy guest.

This is job-record and resource admission only. Raw sector hashing, CPU
discovery, persistent state, queue ownership, dispatch, and interrupted-work
recovery remain outside the family.

The `osp2_queue_transition` case uses one shared `queue44-v1` corpus for its
host and guest adapters. Its 44 vectors exercise raw 512-byte metadata and
entry records, little-endian field reads, complete-record FNV-1a checksums,
entry-index bounds, pending-only admission, cancellation-before-resource
precedence, positive resource headroom, and persisted-state/terminal
transition decisions. In particular, persisted `running` work becomes
`failed(interrupted)` with leases cleared and is never classified for compute.

Storage reads/writes, durable flush/readback, page allocation/release, actual
compute dispatch, counters, and multi-boot integration remain assembly- or
downstream-OS-owned behavior. This compiler family proves only the raw-record
C policy and its reduced transition decisions.

The `osp2_scheduler_transition` case uses one shared `sched60-v1` corpus for
its host and guest adapters. Its 60 assertions exercise initialization,
runnable/sleeping/terminated/faulted states, wake deadlines, yield and timer
selection, sleep-before-due and wake-at-due behavior, exit/fault fallback,
reactivation, invalid requests, and exact switch/yield/sleep/preemption
counters across a bounded two-task policy.

This is the stateful C selection policy only. Interrupt entry, raw 176-byte
frames, stack ownership, timer programming, context save/restore, and `IRETQ`
remain assembly- and downstream-OS-owned. The case-specific
`R_X86_64_32S` allowance is limited to local BSS state accesses and matches
the independently compiled Clang object.

The `osp2_sync_rank` case uses one shared `sync51-v1` corpus for its host and
guest adapters. Its 51 assertions cover the exact six-field two-CPU
contention-observation validator, source-order error precedence, the complete
held/requested rank matrix over values zero through four, and extreme unsigned
boundaries. Only queue(1) to PMM(2) to device(3) increasing nesting is
accepted; zero, out-of-range, equal, and descending requests fail closed.

This is observation and ordering policy only. Atomic exchange, interrupt
save/restore, IPI delivery, lock mutation, contention production, ownership
enforcement, recursion, sleeping, and fairness remain assembly- and
downstream-OS-owned.

The `osp2_pmm_extent` case uses one shared `extent62-v1` corpus for its host
and guest adapters. Its 62 assertions cover request count/owner admission,
exact failure precedence, live transaction observations, final release/reuse
and generation decisions, IRQ progress, and fixed extent geometry.

Actual allocator mutation, page/handle state, token publication and
stale-token enforcement, zeroing, locking, interrupt state, and rollback
execution remain assembly- and downstream-OS-owned.

The `osp2_kernel_object` case uses one shared `kobj51-v1` corpus for its host
and guest adapters. Its 51 assertions cover the fixed 64-byte cache request,
source-order failures, live two-object observations, final cache/reuse/
generation/poison/IRQ decisions, and fixed 64-object/one-page geometry.

Bitmap and generation mutation, token encoding, pointer validation, zeroing,
poisoning, lock behavior, allocation/free execution, and stale/double-free
rejection remain assembly- and downstream-OS-owned.

The strict `osp2_simulation_abi` and current-threshold
`osp2_simulation_abi_reduced` cases share `sim38-v1`. The corpus covers the
EDU-12 scalar-double SysV surface, exact dyadic integration around 64 steps,
repeated canonical calls, unsigned wraparound, and the published deterministic
reduction result.

`guest_contract.scalar_sse2=true` enables SSE2 in the assembly-owned guest
entry and selects Clang `-msse2` only for these cases. The scalar-zero lowering
fix emits typed stores instead of `llvm.memset`; the strict and countdown
companions both pass ABI, arithmetic, sanitizer, no-helper object, and repeated
QEMU surfaces.

The `osp2_process_lifecycle` case uses one shared `process34-v1` corpus for
its host and guest adapters. Its 34 assertions cover create-from-empty,
destroy-from-exited/faulted, task-0 and one-slot admission, complete opaque
token/pointer authority, stale replay after authority is cleared, exact
four-create/four-destroy/two-exit/two-fault/32-page totals, restored PMM/cache
baselines, and final empty-slot observation.

The nine-argument lifecycle validator exercises SysV stack arguments across
the Clang-adapter/fisiCs-policy guest boundary. Actual allocation, mapping,
zero-fill, scheduler mutation, page-table teardown, object release, CPL-3
execution, and fault production remain assembly- and downstream-OS-owned.

## Failure Routing

1. Preserve the failing OS-P case and exact command.
2. Classify the owning compiler bucket.
3. Reduce the defect into `tests/final/` or the appropriate stable binary lane.
4. Record the unresolved blocker in the active-only private ledger.
5. Leave probe mode before changing compiler/runtime implementation.
6. Re-run the OS-P case after the owner fix, then promote at a meaningful
   checkpoint.

A downstream `os-dev` failure must not be hidden by changing the accepted
compiler hash or bypassing its toolchain identity gate.
