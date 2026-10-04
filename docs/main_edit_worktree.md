# Compiler Main Edit worktree

The persistent development checkout is `<workspace>/_worktrees/fisics_main_edit`
on `codex/fisics-main-edit`. The accepted checkout is `<workspace>/fisiCs` on
`main`. Make compiler edits in Main Edit and retain that worktree after adoption.
The initial accepted base is `6453e25ae67aa809b1a750ad8330ca8110aff12e`.

## Before editing

Read both checkout statuses, branches, heads, and the registered worktrees:

```bash
git -C <workspace>/fisiCs status --short
git -C <workspace>/_worktrees/fisics_main_edit status --short
git -C <workspace>/fisiCs worktree list --porcelain
git -C <workspace>/fisiCs rev-list --left-right --count main...codex/fisics-main-edit
```

When the workspace helper is available, also use
`python3 <workspace>/shared/scripts/mew1/mew1.py lane-state --canonical-root
<workspace>/fisiCs --main-edit-root <workspace>/_worktrees/fisics_main_edit`.
A second writer, unknown dirt, missing checkout, or unexpected branch requires
coordination. Preserve specialist and detached worktrees.

## Build and prove the CLI

Run from Main Edit:

```bash
make -j4
./fisics --version
./fisics --help
make examples examples-canaries
./compilation/run_single.sh ./fisics
./compilation/run_multi.sh ./fisics
make frontend-contract-test
make final-manifest MANIFEST=14-runtime-surface-fp-contract-off.json
```

Each checkout owns its `fisics`, `libfisics_frontend.a`, objects and generated
examples. Do not link a test against a library from another checkout. Build
outputs remain ignored. Select additional checks with
[compiler test confidence tiers](compiler_test_confidence_tiers.md); a full
trust run is required before a broad stabilization adoption.

fisiCs is a CLI compiler: the executable in this checkout is its Main Edit
product. There is no app bundle, Desktop launcher, installed app namespace,
package refresh, or app self-test requirement for this lane. The compiler
version remains the source version; checkout identity comes from Git and the
local proof receipt, rather than a version bump.

## Adopt proven commits

Commit bounded changes in Main Edit. Verify the exact candidate head, ensure
canonical `main` is its ancestor, and classify all canonical dirt first. Adoption
requires maintainer authorization. Preserve and reconcile earlier work before
clearing any redundant local delta; never reset or clean a checkout to force a
merge. On a clean canonical tree:

```bash
git -C <workspace>/fisiCs merge --ff-only codex/fisics-main-edit
make -C <workspace>/fisiCs -j4
```

Rerun the public examples and changed-area gate with the canonical compiler.
Read both heads and statuses independently; accepted source and rebuilt binary
are separate evidence. Keep Main Edit on its retained branch for the next edit.
Push, release archives, VERSION changes, signing, publication, Registry changes
and deployment require their own authorization.
