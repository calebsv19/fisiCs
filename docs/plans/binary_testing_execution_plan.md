# Binary Testing Execution Plan

Date: 2026-04-03

## Goal

Introduce a safe, deterministic binary-testing lane that starts small and
expands in controlled levels, so we can validate real executable behavior
without risking filesystem damage or unstable harness behavior.

## Current State (Grounded Snapshot)

- `make test` and `make final` are currently green.
- `make test-binary-abi` and `make test-binary` are green.
- `make test-binary-corpus`, `make test-binary-wave`, and `make test-binary-diff` are green.
- Binary harness inventory snapshot:
  - total tests: `184`
  - by category: `runtime=136`, `compile_only=19`, `compile_fail=14`, `link_fail=9`, `link_only=6`
  - by level: `smoke=17`, `io=4`, `link=3`, `stdio=5`, `fortify=3`, `abi=58`, `corpus=26`, `diff=68`
- Binary-adjacent checks already exist:
  - `tests/integration/compile_only.sh`
  - `tests/integration/compile_and_link.sh`
  - runtime execution in `tests/final/run_final.py` for `run: true` tests
- Recent fix:
  - resolved `ptr_to_agg` call-shape compile hang in semantic assignment checks.
  - `binary__compile_fail__abi_ptr_to_agg_timeout` now validates as a normal
    compile-fail lane (`expect_output_contains`) instead of timeout sentinel mode.

## Coverage Audit (2026-04-03)

- Runtime baseline set (non-diff runtime IDs): `68`
- Differential parity set (`diff_clang` IDs): `68`
- Effective mapped parity coverage: `68 / 68`
- Remaining baseline runtime IDs without direct diff parity: `0`

## SDL Readiness Audit (2026-04-03)

- `sdl2-config` available: `/opt/homebrew/bin/sdl2-config`
- `pkg-config` available: `/opt/homebrew/bin/pkg-config`
- SDL2 detected: `2.32.10`
- Toolchain flags available via `pkg-config --cflags --libs sdl2`
- Harness already has required controls for safe SDL expansion:
  - isolated per-test run directory (`build/tests/binary/<test_id>/`)
  - runtime env overrides (`run_env`) and tool-gated skips (`skip_if.missing_tools`)
  - timeout/resource profiles (`resource_profile`, `timeout_sec`)
- Not yet implemented: dedicated SDL lane/manifests and make target(s).

## Safety Model (Fail-Closed)

Absolute zero-risk cannot be guaranteed for arbitrary native executables, but
impact can be tightly contained.

Required controls before broad binary expansion:

- Run each binary in an isolated per-test run directory under `build/tests/`.
- Force per-test `cwd` to that run directory.
- Use a strict env allowlist (`PATH`, locale vars, explicit test vars only).
- Set per-test timeouts for compile, link, and runtime phases.
- Set resource limits (`ulimit`) for CPU time, file size, and open files.
- Capture and compare `stdout`, `stderr`, and exit code.
- Kill and fail on timeout/hang.
- Keep all generated artifacts out of repo source tree.
- Default mode never rewrites expectations.

Optional hardening (phase-later):

- OS sandbox profile for runtime process (filesystem/network restrictions).
- Sanitizer lane for harness/helper binaries.

## Test Levels (Progressive Rollout)

## Level 0: Compile-Only Binary Surface

- Compile single-file and multi-file programs to object or executable.
- No runtime execution.
- Purpose: driver/link pipeline confidence with minimal risk.

## Level 1: Runtime Smoke (Pure Deterministic)

- Run tiny deterministic binaries (no filesystem writes).
- Compare exact `stdout` + exit code.
- Short timeout and strict resource limits.

## Level 2: Runtime I/O Surface (Controlled)

- Add controlled `stdin`, argv, and explicit env input tests.
- Keep writes restricted to per-test temp dir.
- Validate `stdout`, `stderr`, exit code.

## Level 3: Multi-TU and Linkage Matrix

- Multi-translation-unit binaries with extern/static/tentative definitions.
- Link-error expectation lane (negative link tests).
- Ensure precise compile-vs-link failure classification.

## Level 4: ABI and Calling Convention Frontier

- Many-arg calls, struct returns, variadics, function pointers.
- Architecture-sensitive cases tagged `abi_sensitive`.
- Differential runtime checks only where behavior is stable.

## Level 5: External Mini-Corpus (Compile First)

- Curated small real-world fragments.
- Start compile-only, then selective execute-safe binaries.
- No mass execution of unknown binaries until earlier levels are stable.

## Harness Architecture

Proposed new harness:

- `tests/binary/run_binary.py` (manifest-driven, fail-closed)
- Inputs:
  - `tests/binary/meta/*.json`
  - `tests/binary/meta/index.json` registry
- Sources:
  - `tests/binary/cases/`
- Expectations:
  - `tests/binary/expect/`
- Artifacts:
  - `build/tests/binary/<test_id>/`

Per-test metadata minimum:

- `id`
- `category`: `compile_only | compile_fail | link_fail | link_only | runtime`
- `inputs` (one or more source files)
- `args` (compiler args)
- `run` (bool)
- `expect_exit`
- `expected_stdout`
- `expected_stderr`
- `timeout_sec`
- `resource_profile` (`smoke | default | heavy`)
- `ub` / `impl_defined` tags
- `skip_if` rules
- `expect_compile_timeout` (optional; for known compile-hang tracking lanes)
- `differential_with` (optional; currently `clang`)

## Make Entrypoints

Implemented:
- `make test-binary` (all binary levels currently enabled)
- `make test-binary-smoke` (Level 0-1)
- `make test-binary-io` (Level 2)
- `make test-binary-link` (Level 3)
- `make test-binary-stdio` (stdio formatting lane)
- `make test-binary-fortify` (fortified builtin lowering lane)
- `make test-binary-abi` (Level 4)
- `make test-binary-corpus` (Level 5 compile-focused initial slice)
- `make test-binary-diff` (initial `fisics` vs `clang` runtime parity shard)
- `make test-binary-id ID=<test_id>`
- `make test-binary-wave WAVE=<n> [BINARY_WAVE_BUCKET=<bucket-prefix>]`

Planned:
- Level 5 corpus scale-out waves (beyond wave 1 initial slice)

Expectation update path (explicit only):

- `make binary-regen TEST=<id> CONFIRM=YES`
- No default/global binary golden rewrite path.

## Failure Taxonomy (Required Reporting)

Each failure must be classified as one of:

- compile_fail_unexpected
- compile_succeeded_unexpected
- link_fail_unexpected
- runtime_exit_mismatch
- runtime_stdout_mismatch
- runtime_stderr_mismatch
- runtime_timeout
- runtime_resource_limit
- differential_mismatch
- harness_error

## Promotion Gates

Advance levels only when all are true:

- level slice is green for 3 consecutive runs
- no unresolved `harness_error` or `runtime_timeout`
- no expectation drift without explicit reviewed update
- docs run-log updated with promoted tests and blocked IDs

## First Implementation Slice

Phase A (small, safe):

- Add `tests/binary/run_binary.py` with timeout + cwd isolation + env allowlist.
- Add Level 0-1 manifests (10-20 tests).
- Add `make test-binary-smoke`.
- Keep execution limited to deterministic no-write smoke binaries.

Phase B:

- Add Level 2-3 manifests.
- Add link-failure classification and stronger reporting.

Phase C:

- Add Level 4 ABI frontier lane and selective Level 5 corpus compile-only lane.

## Progress Snapshot

- Phase A implemented:
  - `tests/binary/run_binary.py`
  - `make test-binary-smoke`, `make test-binary-id`, `make binary-regen`
  - initial smoke manifest and cases
- Phase B complete:
  - Level 2 (`io`) manifest scaffold
  - Level 3 (`link`) manifest scaffold
  - harness classification for `link_fail` category
  - stdio format lane added (`printf`/`fprintf`/`snprintf`/`vsnprintf`)
  - `fprintf(stderr, ...)` crash resolved by extern-global codegen fix for stdio globals
  - fortified builtin lowering expanded for `__builtin___*chk` wrappers:
    - `snprintf` / `vsnprintf` / `sprintf`
    - `memcpy` / `memmove` / `memset`
    - `strcpy` / `strcat` / `strncpy` / `strncat`
  - added `fortify` binary lane and manifest (`binary-fortify-wave5.json`)
  - hardened binary harness text decoding (`errors=replace`) to fail with diffs instead of crashing on non-UTF8 output
- Phase C in progress (late):
  - harness extended with `compile_fail` category (compile-stage failure expectation with optional output fragments)
  - added `abi` binary lane and manifest (`binary-abi-wave6.json`)
  - added initial ABI/runtime anchors:
    - `binary__runtime__abi_many_int_args`
    - `binary__runtime__abi_variadic_promotions`
    - `binary__runtime__abi_fnptr_dispatch`
    - `binary__runtime__abi_struct_return_large`
  - added ABI wave 7 stress shard (`binary-abi-wave7.json`) with:
    - `binary__runtime__abi_mixed_width_args`
    - `binary__runtime__abi_variadic_forwarder`
    - `binary__runtime__abi_multitu_call_chain`
    - `binary__runtime__abi_struct_roundtrip`
  - added ABI wave 8 floating/varargs stress shard (`binary-abi-wave8.json`) with:
    - `binary__runtime__abi_fp_param_mix`
    - `binary__runtime__abi_fp_return_chain`
    - `binary__runtime__abi_varargs_float_promotion`
    - `binary__runtime__abi_varargs_mixed_scalars`
  - added ABI wave 9 aggregate ABI stress shard (`binary-abi-wave9.json`) with:
    - `binary__runtime__abi_struct_float_param`
    - `binary__runtime__abi_struct_float_return`
    - `binary__runtime__abi_multitu_struct_float`
    - `binary__runtime__abi_nested_struct_roundtrip`
  - added ABI wave 10 pointer-heavy ABI stress shard (`binary-abi-wave10.json`) with:
    - `binary__runtime__abi_ptr_struct_mix`
    - `binary__runtime__abi_multitu_fnptr`
    - `binary__runtime__abi_pointer_return_roundtrip`
    - `binary__runtime__abi_fnptr_array_chain`
  - added ABI wave 11 fail-closed negative shard (`binary-abi-wave11.json`) with:
    - `binary__link_fail__abi_undefined_fnptr`
    - `binary__link_fail__abi_undefined_struct_return`
    - `binary__link_fail__abi_duplicate_fn_dispatch`
    - `binary__link_fail__abi_duplicate_fn_table`
  - added ABI wave 12 large-aggregate + mixed varargs multitu shard (`binary-abi-wave12.json`) with:
    - `binary__runtime__abi_large_struct_param`
    - `binary__runtime__abi_large_struct_multitu`
    - `binary__runtime__abi_varargs_mixed_multitu`
    - `binary__runtime__abi_varargs_forward_multitu`
  - added ABI wave 13 deep-nesting + cross-TU chain shard (`binary-abi-wave13.json`) with:
    - `binary__runtime__abi_nested_ptr_aggregate`
    - `binary__runtime__abi_multitu_chain_depth`
    - `binary__runtime__abi_mixed_return_pressure`
    - `binary__runtime__abi_multitu_nested_struct_ptr`
  - added ABI wave 14 compile-shape negative shard (`binary-abi-wave14.json`) with:
    - `binary__compile_fail__abi_too_few_args`
    - `binary__compile_fail__abi_too_many_args`
    - `binary__compile_fail__abi_fnptr_incompatible_assign`
    - `binary__compile_fail__abi_struct_arg_mismatch`
  - added ABI wave 15 fnptr-table + data-symbol negative shard (`binary-abi-wave15.json`) with:
    - `binary__compile_fail__abi_fnptr_table_incompatible_signature`
    - `binary__compile_fail__abi_fnptr_table_too_few_args`
    - `binary__compile_fail__abi_fnptr_table_too_many_args`
    - `binary__compile_fail__abi_fnptr_struct_arg_incompatible`
    - `binary__link_fail__abi_undefined_global_struct`
    - `binary__link_fail__abi_duplicate_global_struct`
  - added ABI wave 16 long-double + mixed signedness shard (`binary-abi-wave16.json`) with:
    - `binary__runtime__abi_long_double_variadic_bridge`
    - `binary__runtime__abi_long_double_struct_return`
    - `binary__runtime__abi_signed_unsigned_matrix`
    - `binary__runtime__abi_varargs_signed_unsigned_bridge`
  - added ABI wave 17 deep long-double pointer/call-shape shard (`binary-abi-wave17.json`) with:
    - `binary__runtime__abi_long_double_ptr_aggregate`
    - `binary__runtime__abi_multitu_long_double_ptr_chain`
    - `binary__runtime__abi_multitu_long_double_fnptr_dispatch`
    - `binary__runtime__abi_varargs_long_double_signed_unsigned`
    - `binary__compile_fail__abi_long_double_fnptr_too_few`
    - `binary__compile_fail__abi_long_double_arg_incompatible`
  - added ABI wave 18 deep varargs-forwarding + pointer/aggregate call-shape shard (`binary-abi-wave18.json`) with:
    - `binary__runtime__abi_multitu_varargs_forward_chain`
    - `binary__runtime__abi_multitu_varargs_deep_chain`
    - `binary__compile_fail__abi_struct_ptr_called_with_int`
    - `binary__compile_fail__abi_int_ptr_called_with_struct_ptr`
    - `binary__compile_fail__abi_void_ptr_called_with_struct_value`
  - added ABI wave 19 compile-fail lane (`binary-abi-wave19.json`) with:
    - `binary__compile_fail__abi_ptr_to_agg_timeout`
  - added Level 5 corpus wave 1 compile-only shard (`binary-corpus-wave1.json`) with:
    - `binary__compile_only__corpus_fragment_map_parser`
    - `binary__compile_only__corpus_fragment_ring_pipeline`
    - `binary__compile_only__corpus_fragment_ast_table`
  - added Level 5 corpus wave 2 compile/link-only shard (`binary-corpus-wave2.json`) with:
    - `binary__compile_only__corpus_fragment_token_trie`
    - `binary__compile_only__corpus_fragment_cfg_liveness`
    - `binary__link_only__corpus_fragment_ir_graph_pipeline`
    - `binary__link_only__corpus_fragment_event_router`
  - added Level 5 corpus wave 3 compile/link-only shard (`binary-corpus-wave3.json`) with:
    - `binary__compile_only__corpus_fragment_dataflow_scheduler`
    - `binary__compile_only__corpus_fragment_string_arena`
    - `binary__link_only__corpus_fragment_pass_registry`
    - `binary__link_only__corpus_fragment_const_table_merge`
  - added Level 5 corpus wave 4 compile/link-only shard (`binary-corpus-wave4.json`) with:
    - `binary__compile_only__corpus_fragment_macro_fold`
    - `binary__compile_only__corpus_fragment_header_span`
    - `binary__link_only__corpus_fragment_symbol_pool`
  - added Level 5 corpus wave 5 compile/link-only shard (`binary-corpus-wave5.json`) with:
    - `binary__compile_only__corpus_fragment_pp_dispatch_table`
    - `binary__compile_only__corpus_fragment_pack_index`
    - `binary__link_only__corpus_fragment_job_graph`
  - added Level 5 corpus wave 6 include-chain compile-only shard (`binary-corpus-wave6.json`) with:
    - `binary__compile_only__corpus_fragment_include_chain_dispatch`
    - `binary__compile_only__corpus_fragment_guarded_reinclude`
    - `binary__compile_only__corpus_fragment_macro_include_toggle`
  - added Level 5 corpus wave 7 bounded runtime shard (`binary-corpus-wave7.json`) with:
    - `binary__runtime__corpus_fragment_token_score`
    - `binary__runtime__corpus_fragment_symbol_pool_runtime`
  - added Level 5 corpus wave 8 bounded runtime shard (`binary-corpus-wave8.json`) with:
    - `binary__runtime__corpus_fragment_pass_registry_runtime`
    - `binary__runtime__corpus_fragment_job_graph_runtime`
  - added Level 5 corpus wave 9 bounded runtime shard (`binary-corpus-wave9.json`) with:
    - `binary__runtime__corpus_fragment_const_table_runtime`
    - `binary__runtime__corpus_fragment_include_chain_runtime`
  - added differential wave 1 runtime shard (`binary-diff-wave1.json`) with:
    - `binary__runtime__diff_clang_stdout_sum`
    - `binary__runtime__diff_clang_struct_pass_return`
    - `binary__runtime__diff_clang_abi_fnptr_dispatch`
  - added differential wave 2 runtime shard (`binary-diff-wave2.json`) with:
    - `binary__runtime__diff_clang_argv_count`
    - `binary__runtime__diff_clang_stdin_byte_count`
    - `binary__runtime__diff_clang_env_echo`
  - added differential wave 3 runtime shard (`binary-diff-wave3.json`) with:
    - `binary__runtime__diff_clang_multitu_link`
    - `binary__runtime__diff_clang_io_stderr_stdout_mix`
    - `binary__runtime__diff_clang_io_file_roundtrip`
  - added differential wave 4 runtime shard (`binary-diff-wave4.json`) with:
    - `binary__runtime__diff_clang_stdio_printf_basic`
    - `binary__runtime__diff_clang_stdio_fprintf_stderr_basic`
    - `binary__runtime__diff_clang_stdio_snprintf_truncation`
    - `binary__runtime__diff_clang_stdio_vsnprintf_wrapper`
    - `binary__runtime__diff_clang_stdio_longlong_format`
  - added differential wave 5 runtime shard (`binary-diff-wave5.json`) with:
    - `binary__runtime__diff_clang_abi_many_int_args`
    - `binary__runtime__diff_clang_abi_struct_roundtrip`
    - `binary__runtime__diff_clang_abi_fnptr_array_chain`
    - `binary__runtime__diff_clang_abi_mixed_width_args`
    - `binary__runtime__diff_clang_abi_multitu_call_chain`
  - added differential wave 6 runtime shard (`binary-diff-wave6.json`) with:
    - `binary__runtime__diff_clang_fortify_strcpy_strcat`
    - `binary__runtime__diff_clang_fortify_memcpy_memmove_memset`
    - `binary__runtime__diff_clang_io_append_counter`
    - `binary__runtime__diff_clang_io_stdin_to_file`
    - `binary__runtime__diff_clang_global_char_array_string_init`
  - added differential wave 7 runtime shard (`binary-diff-wave7.json`) with:
    - `binary__runtime__diff_clang_abi_fp_param_mix`
    - `binary__runtime__diff_clang_abi_fp_return_chain`
    - `binary__runtime__diff_clang_abi_varargs_float_promotion`
    - `binary__runtime__diff_clang_abi_varargs_mixed_scalars`
    - `binary__runtime__diff_clang_abi_variadic_promotions`
  - added differential wave 8 runtime shard (`binary-diff-wave8.json`) with:
    - `binary__runtime__diff_clang_abi_multitu_chain_depth`
    - `binary__runtime__diff_clang_abi_multitu_nested_struct_ptr`
    - `binary__runtime__diff_clang_abi_multitu_fnptr`
    - `binary__runtime__diff_clang_abi_multitu_varargs_forward_chain`
    - `binary__runtime__diff_clang_abi_multitu_varargs_deep_chain`
  - added differential wave 9 runtime shard (`binary-diff-wave9.json`) with:
    - `binary__runtime__diff_clang_corpus_fragment_token_score`
    - `binary__runtime__diff_clang_corpus_fragment_symbol_pool_runtime`
    - `binary__runtime__diff_clang_corpus_fragment_pass_registry_runtime`
    - `binary__runtime__diff_clang_corpus_fragment_job_graph_runtime`
    - `binary__runtime__diff_clang_corpus_fragment_const_table_runtime`
  - added differential wave 10 runtime shard (`binary-diff-wave10.json`) with:
    - `binary__runtime__diff_clang_abi_long_double_variadic_bridge`
    - `binary__runtime__diff_clang_abi_long_double_struct_return`
    - `binary__runtime__diff_clang_abi_signed_unsigned_matrix`
    - `binary__runtime__diff_clang_abi_varargs_signed_unsigned_bridge`
    - `binary__runtime__diff_clang_corpus_fragment_include_chain_runtime`
  - added differential wave 11 runtime shard (`binary-diff-wave11.json`) with:
    - `binary__runtime__diff_clang_abi_struct_return_large`
    - `binary__runtime__diff_clang_abi_large_struct_param`
    - `binary__runtime__diff_clang_abi_large_struct_multitu`
    - `binary__runtime__diff_clang_abi_struct_float_param`
    - `binary__runtime__diff_clang_abi_struct_float_return`
  - added differential wave 12 runtime shard (`binary-diff-wave12.json`) with:
    - `binary__runtime__diff_clang_abi_long_double_ptr_aggregate`
    - `binary__runtime__diff_clang_abi_multitu_long_double_ptr_chain`
    - `binary__runtime__diff_clang_abi_multitu_long_double_fnptr_dispatch`
    - `binary__runtime__diff_clang_abi_varargs_long_double_signed_unsigned`
    - `binary__runtime__diff_clang_abi_varargs_forward_multitu`
  - added differential wave 13 runtime shard (`binary-diff-wave13.json`) with:
    - `binary__runtime__diff_clang_return_zero`
    - `binary__runtime__diff_clang_exit_nonzero`
    - `binary__runtime__diff_clang_local_char_array_string_init`
    - `binary__runtime__diff_clang_local_char_array_string_init_exact_fit`
    - `binary__runtime__diff_clang_local_char_array_string_init_zero_fill`
  - added differential wave 14 runtime shard (`binary-diff-wave14.json`) with:
    - `binary__runtime__diff_clang_abi_ptr_struct_mix`
    - `binary__runtime__diff_clang_abi_pointer_return_roundtrip`
    - `binary__runtime__diff_clang_abi_nested_ptr_aggregate`
    - `binary__runtime__diff_clang_abi_nested_struct_roundtrip`
    - `binary__runtime__diff_clang_abi_multitu_struct_float`
  - added differential wave 15 runtime shard (`binary-diff-wave15.json`) with:
    - `binary__runtime__diff_clang_abi_mixed_return_pressure`
    - `binary__runtime__diff_clang_abi_varargs_mixed_multitu`
    - `binary__runtime__diff_clang_abi_variadic_forwarder`
    - `binary__runtime__diff_clang_fortify_strncpy_strncat`
  - wired `make test-binary-corpus` and included it in `make test-binary`
  - wired `make test-binary-diff` and included it in `make test-binary`
  - added `make test-binary-wave` selector using `BINARY_WAVE_BUCKET` (avoids collision with final-suite `WAVE_BUCKET`)
  - harness extended with `link_only` category (compile+link pass without runtime execution)
  - harness extended with optional differential runtime checks (`differential_with: clang`)

## Current Phase Marker

- Current phase: **Phase D (Level 5 expansion in progress)**.
- Phase C baseline is complete and stable through ABI wave 19.
- Level 5 corpus lane is active through wave 9 (compile/link + bounded runtime).
- Differential lane is active through diff wave 15 with full runtime parity (`68/68`).

## Next Phase Work

1. SDL Phase E (new lane): add headless SDL binary tests in controlled rollout.
   - E1 compile/link-only SDL smoke (`skip_if.missing_tools: [pkg-config]`).
   - E2 headless runtime init/teardown (`run_env` with
     `SDL_VIDEODRIVER=dummy`, `SDL_AUDIODRIVER=dummy`).
   - E3 bounded event-loop + software surface update checks (no GPU hard dependency).
