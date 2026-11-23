# CLI process guard

The CLI defaults to at most one active guarded compiler process for the current
effective user. `FISICS_MAX_PROCS` sets the limit; `0` explicitly disables process
admission. The watchdog remains separately controlled by `FISICS_TIMEOUT_SEC`
(default 180 seconds; `0` disables it).

The default guard directory is `/tmp/fisics_proc_guard-<effective-uid>`, shared
across invocations regardless of `TMPDIR`. The directory must be owned by the
user with mode 0700 and must not be a symlink. PID entries and the accounting
lock must be owner-only regular files. Cleanup and counting use directory
descriptors; a private lock serializes stale cleanup and entry creation.

`FISICS_PROC_GUARD_DIR` explicitly selects an absolute guard directory for a
restricted sandbox or intentionally separate accounting scope. All invocations
that should share a limit must use the same value. This setting is independent
of per-task temporary storage; changing only `TMPDIR` never selects a new scope.
The selected directory receives the same ownership/mode/type validation.

Admission is best effort: unavailable or unsafe directory/lock setup emits a
warning and allows compilation, preserving the existing availability policy.
This is a convenience concurrency guard, not a security quota against the same
user, who can explicitly disable it. Older compiler versions using the legacy
shared `/tmp/fisics_proc_guard` do not participate in the new per-user counter.

Run `make integration-process-guard-security` for poisoned-directory/entry,
cross-TMPDIR, simultaneous-start, stale/exit cleanup, watchdog and normal compile
contracts. Tests use their own explicitly selected private guard directory;
they do not populate or remove the user's default live namespace.
