# OS Policy Validation Lane

Status: OS-P3 stress closed; deterministic post-EDU-19 runtime intake promoted

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
| OS-P3 | complete | 95 deterministic mutation, raw-format, ABI, aggregate, and freestanding object probes closed with no blockers |
| OS-P4 | future | first versioned compiler-metadata accept/narrow/reject/ignore contract |

The next compiler-testing frontier is not OS-P4. OS-P4 remains reserved for a
real compiler-metadata schema. Current work is a post-EDU-19 source-intake
continuation: copy bounded policy from immutable `os-dev` tags into
self-contained probes, preserve exact source provenance, and separately
design an optional live-source canary that never replaces the OS repository's
accepted compiler.

The historical crosswalk now covers EDU-20 through EDU-31. EDU-20 is
assembly/platform discovery only and has no compiler-C probe. EDU-21 through
EDU-31 use eleven runtime matrices, 574 deterministic vectors, and eleven
freestanding object probes:

- exact EDU-21 Wire-v1 validation and its original six-operation boundary;
- exact EDU-22 Queue-metadata-v3, Entry-v2, Trace-v1, and
  validate-before-action sequencing;
- EDU-23 bounded parallelism admission, grants, values, and path evidence;
- EDU-24 result-artifact identity, cursor, chunk, and final-marker policy;
- exact Wire-v2 through Wire-v7 validation across EDU-24 through EDU-31;
- EDU-25 bounded kernel-loader geometry and EDD transfer plans;
- EDU-26 generation-safe reuse, tombstones, ACK identity, and wrap rejection;
- EDU-27 phase/event prefixes and impossible-history rejection;
- the exact EDU-28 artifact metadata validator;
- EDU-29/30 asynchronous activation and cooperative stop precedence;
- EDU-31 calibrated monotonic-time arithmetic and overflow rejection.

The exact-source and assembly-derived boundaries are labeled independently.
All eleven matrices pass strict C99 Clang diagnostics plus ASan/UBSan. With
the two composition slices, the aggregate resolves all 48 runtime/object
probes, and the stable torture-differential gate remains at
`0 failing / 10 expected skips`.

The first post-EDU-19 probe snapshots EDU-32 Workload-v1 validation. It covers
exact 104-byte parsing, little-endian reconstruction, six finite-double
admission points, step bounds, independently pinned partition/final results,
unaligned byte input, scalar-double calls, and fail-closed identity or result
contradictions. It also applies the freestanding object contract to the exact
policy source.

The second probe snapshots EDU-33 typed Result-v1 validation. Its 31-vector
matrix covers the exact 80-byte logical record inside a zero-padded 512-byte
sector, slots 0 through 7, nonzero identity fields, nested and whole-sector
FNV-1a checksums, reserved bytes, unaligned input, and stale mutations without
checksum republication. Partition and final result fields are intentionally
tested as opaque bits: structural integrity admits zero and NaN-shaped values
without assigning arithmetic meaning. Its paired object probe requires one
exact export, no undefined helpers, bounded relocation, no red zone, and the
hardware-blind instruction policy.

The third probe snapshots the complete EDU-34 `queue_kernel.c`. Its 30-vector
matrix covers pending/running deadline placement, terminal duration audit,
timeout/budget/cancellation evidence, preserved phase prefixes, exact duration
bounds, unaligned input, and recomputed-checksum contradictions. The frozen
validator compares only the low 32 bits of the 64-bit terminal-reason slot;
the matrix preserves that admission as an explicit upstream policy observation
rather than describing it as fail-closed. Assembly safe-point ordering and
QEMU timing remain `os-dev`-owned proof.

The fourth probe snapshots EDU-35 non-resumable checkpoint validation. Its
47-vector matrix covers exact owner and policy identity, phase-prefix three,
work/worker/duration bounds, repeated partition evidence, four nested checksum
boundaries, zero padding, unaligned input, and republished-checksum
contradictions. The baseline embeds a valid Workload-v1 object; one explicit
case preserves the validator boundary where embedded bytes are structurally
bound while the surrounding OS path owns their full semantic admission and
authoritative correlation. Publication ordering, crash ownership, lookup, and
the non-resume rule remain `os-dev`-owned proof.

The fifth probe snapshots EDU-36 durable checkpoint-resume entry validation.
Its 38-vector matrix covers resumed running, complete, interrupted, timeout,
and cancellation forms; exact `RESUMED` flag and `RESUME_RESTORED` event
agreement; restored phase-three and direct-reduce sequencing; consumed-work
accounting; ordinary-path separation; unaligned input; and fail-closed
identity, event, state, and checksum contradictions. The paired object probe
requires the exact validator export with no helpers, `PC32`-only relocation,
no red zone, and hardware-blind instructions.

The matrix preserves one upstream implementation observation: the frozen
Entry-v8 validator accepts a resumed Trace-v3 entry with trace flags `0`, even
though the EDU-36 resume contract assigns all four compact-epoch observation
flags. Snapshot selection and correlation, resource/deadline execution,
idempotence, ACK/reuse authority, persistence ordering, reboot execution, and
QEMU proof remain `os-dev`-owned boundaries.

The sixth probe is an EDU-37 two-owner checkpoint-store contract mirror.
EDU-37 changes assembly and storage ownership but adds no generated C:
`queue_kernel.c` remains byte-identical to EDU-36. The 36-vector compiler
matrix therefore labels its bounded C selection policy as derived rather than
claiming exact-source intake. It links the exact inherited Workload-v1 and
Snapshot-v1 validators and covers zero/valid/unique lane admission,
duplicate-owner and nested-workload rejection, exact-owner/empty/reclaimable
selection order, running and resumable-interrupted owner protection,
full-capacity failure, stale identity reclamation, and unaligned storage.

The paired object probe permits only the two inherited validator imports and
bounded `PC32`/`PLT32` relocation. ATA ordering, boot validation, assembly
selection, persistence, lookup, reboot, and QEMU behavior remain
`os-dev`-owned proof.

The seventh wave anchors immutable EDU-38. It embeds the complete
`control_kernel.c` source byte-for-byte and runs 56 vectors across Wire-v13,
the prior and next wire versions, all twenty operation payload contracts,
checksum/format precedence, bounds, padding, and unaligned request frames.
Its object contract requires the three exact control exports, no imports,
`PC32`-only relocation, no red zone, and hardware-blind instructions.

Runner contexts remain assembly-owned, so their companion probe is explicitly
an assembly-derived compiler contract mirror. Its 45 vectors cover two fixed
160-byte records, slot-modulo-two selection, inactive and sole-active scans,
duplicate-active corruption, active identity/resource shape, terminal
evidence, checkpoint lanes, bounds, isolation, and unaligned records. This
does not claim that authoritative OS execution is implemented in C: EDU-38
still permits only one active runner, with phase/results and the AP mailbox
remaining singleton OS state.

The focused EDU-38 wave passes `4 resolved / 0 blocked / 0 skipped`; combined
EDU-32-through-EDU-38 intake passes
`16 resolved / 0 blocked / 0 skipped`; and the complete bucket-15 inventory
passes `654 resolved / 0 blocked / 0 skipped`. Both new matrices also pass
Clang AddressSanitizer/UndefinedBehaviorSanitizer execution.

The eighth wave derives a bounded compiler contract mirror from immutable
EDU-39. No generated C or wire-format change occurred; `smp64.asm` remains the
authoritative implementation. The 70-vector matrix covers two independent
224-byte saved owners, exact queue and Workload-v1 correlation, phase and
reduction shape, load boundaries, retained invalid evidence, unaligned
records, owner-local publication counters, and different-owner switch
rejection both while a width-two AP batch is in flight and after completion
but before its joined barrier.

The paired object probe permits only the inherited Workload-v1 validator
import and bounded `PC32`/`PLT32` relocation. Two saved valid phase owners do
not establish two active jobs: the active-runner limit, live SMP working set,
and AP mailbox remain singleton and OS-owned. EDU-39 passes
`2 resolved / 0 blocked / 0 skipped`; combined EDU-32-through-EDU-39 intake
passes `18 resolved / 0 blocked / 0 skipped`, and its matrix also passes Clang
AddressSanitizer/UndefinedBehaviorSanitizer execution. The sequential EDU-39
closeout replay passes the complete bucket-15 probe inventory at
`656 resolved / 0 blocked / 0 skipped` and the stable owning bucket at
`687 passed / 0 failing / 10 expected skips`.

The ninth wave derives a bounded compiler contract mirror from immutable
EDU-40. No generated C, wire, or durable-format change occurred; `smp64.asm`
remains authoritative. Its 78 vectors cover the 112-byte volatile mailbox,
reset and begin rules, monotonic/exhausted generations, queue-phase and
reserved legacy owners, no-skip AP acceptance, distinct dispatch/completion
tuples, successful and correlated-error result publication, retained
retirement evidence, width-one silence, and unaligned records.

The paired object probe is import-free and permits only bounded
`PC32`/`PLT32` relocation. This does not instance the physical AP or mailbox
and does not raise the one-active-runner limit. EDU-40 passes
`2 resolved / 0 blocked / 0 skipped`; combined EDU-32-through-EDU-40 intake
passes `20 resolved / 0 blocked / 0 skipped`; the complete bucket-15 inventory
passes `658 resolved / 0 blocked / 0 skipped`; and the stable owning bucket
passes `687 passed / 0 failing / 10 expected skips`. Its matrix also passes
Clang AddressSanitizer/UndefinedBehaviorSanitizer execution.

The tenth wave derives a bounded compiler contract mirror from immutable
EDU-41. No generated C, wire, or durable-format change occurred;
`queue64.asm` and `smp64.asm` remain authoritative. Its 79 vectors cover
bounded two-active counting, exact RUNNING identity, eligible FIFO activation,
third-owner rejection, alternating idle-mailbox turns, exact in-flight
mailbox pinning, owner-local release, cancel/deadline/budget/work precedence,
peer-preserving retirement, corruption rejection, and unaligned records.

The paired object probe is import-free and requires nine exact exports with
bounded `PC32`/`PLT32` relocation. This does not instance the physical
BSP/AP appliance or claim a general scheduler. EDU-41 passes
`2 resolved / 0 blocked / 0 skipped`; its matrix passes Clang
AddressSanitizer/UndefinedBehaviorSanitizer execution; and the stable owning
bucket passes `687 passed / 0 failing / 10 expected skips`.

The first two-owner composition slice adds 36 vectors over the frozen EDU-40
mailbox and EDU-41 runner helpers. It checks two-active identity, owner-pinned
dispatch/completion, wrong-owner rejection, independent policy outcomes,
owner-local phase release, unaligned data, and peer-preserving retirement.
All 48 intake/composition runtime and object probes resolve in one aggregate
replay.

The durable-owner-chain composition slice adds 46 vectors over the frozen
EDU-26 generation/reuse, EDU-35 snapshot, EDU-37 checkpoint-store, EDU-39
phase-owner, EDU-40 mailbox, and EDU-41 runner helpers. It proves:

- exact ACK identity, safe next-generation admission, owner-local retirement,
  and byte-for-byte peer preservation;
- valid checkpoint storage plus exact lane/running-owner correlation;
- exact phase-owner, workload, running-context, and mailbox-owner correlation;
- a complete metadata-to-dispatch chain that rejects stale generation,
  request, owner, workload, checksum, lane, and dispatch contradictions.

Its paired object probe permits only the fourteen named frozen helper imports.
The runtime matrix passes strict C99 Clang diagnostics and ASan/UBSan; its
focused runtime/object pair resolves `2 / 2`; the post-EDU-19 aggregate
resolves `48 / 48`; and the stable torture-differential replay closes at
`0 failing / 10 expected skips`.

Stable torture-differential wave `139` promotes 88 deterministic OS-P3 and
post-EDU-19 runtime cases with exact stdout oracles and reference-compiler
parity. The refreshed promotion audit records `3577` promoted entries, `19`
intentional probe-only entries, and no missing, critical, or ambiguous
promotion candidates. The monitored broad checkpoint closes at `4984` passes,
`0` failures, and `36` expected skips. The four remaining post-EDU-19
policy/source mirrors stay explicitly probe-only because smaller stable cases
own their compiler behavior.

The next behavior-focused wave should exercise temporal fault sequences around
this chain: interruption before ACK, restart after checkpoint selection,
owner loss during a phase, and retirement after mailbox completion. Each
sequence should reject stale re-admission while preserving the unaffected
peer. Physical durability and BSP/AP execution remain OS-owned proof.

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

## Post-EDU-19 Stable And Probe Lanes

Run the promoted deterministic runtime closure with:

```sh
make final-manifest \
  MANIFEST=15-torture-differential-wave139-os-policy-probe-promotion-closure.json
```

Run the complete immutable frontier intake with:

```sh
PROBE_FILTER=15__probe_os_post_edu19_ \
  python3 tests/final/probes/run_probes.py
```

This probe prefix remains noncanonical frontier evidence. Deterministic runtime
behavior with durable oracles belongs in stable wave `139` (or its narrower
owning compiler bucket). Large source-snapshot or policy matrices may remain
explicitly probe-only when a smaller stable oracle owns the compiler behavior.
