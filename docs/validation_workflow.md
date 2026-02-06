# Full-Program Compilation Validation Workflow

Purpose: Validate fisics against real projects by compiling every translation unit, triaging failures, fixing issues in focused batches, and tracking progress in a master log.

## Workflow (per project)
1) **Inventory translation units**
   - Enumerate all `.c` files in the project.
   - Decide the include roots (`-I<project>/include`, `-I<project>/src`, etc.).

2) **Compile each file and capture errors**
   - Run `./fisics -c <file.c>` with the selected include paths.
   - Record the first error (or full log if needed) per file.
   - Store results in `FAILING.md` (or a dedicated report file) with:
     - File path
     - Error class (missing header / parser / semantic / codegen / crash)
     - Key message

3) **Cluster by root cause**
   - Group failures by likely shared cause.
   - Rank clusters by impact (largest count first, then easiest fixes).

4) **Fix by cluster, then re-run**
   - Apply the minimal compiler fixes for a cluster.
   - Re-compile the same subset of files to confirm the fix.
   - Update the failure log, moving files to `[PASS]` as they succeed.

5) **Progressive project validation**
   - Repeat until all files compile or remaining failures are clearly scoped.
   - For stubborn failures, mark `[PARTIAL]` with concrete blockers.

6) **Regression checks**
   - Run `make tests` and `make final`.
   - Update goldens only for tests impacted by the fixes.

## Notes
- Keep fixes scoped to the active cluster to avoid unrelated regressions.
- Always prefer fixes that unblock many files at once.
- If a failure is due to missing third-party headers, document it clearly and skip unless the headers are available.
- Keep `FAILING.md` as the live tracking doc while a project is in progress.
