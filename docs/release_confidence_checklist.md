# Release Confidence Checklist

This is the named public release-confidence lane for `fisiCs`.

It is intentionally narrower than the full internal validation matrix. Use it
to decide whether a user-facing compiler checkpoint is coherent enough to
package, document, or hand to a contributor.

Run all commands from:

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
```

## 1. Build Gate

```bash
make
make release-contract
```

Pass rule:

- compiler builds cleanly
- release contract assets/staging metadata generate cleanly

## 2. Examples Gate

```bash
make examples
make examples-canaries
./compilation/run_single.sh ./fisics
./compilation/run_multi.sh ./fisics
```

Pass rule:

- hello-world path works
- public multi-TU smoke works
- practical libc/string and numeric/math canaries work
- physics-units pilot still builds in explicit overlay mode

## 3. Changed-Area Final Gate

Pick the narrowest relevant final lane for the change.

Examples:

```bash
make final-manifest MANIFEST=14-runtime-surface-wave323-header-math-loop-edge-runtime-promotion.json
make final-bucket BUCKET=runtime-surface
```

Pass rule:

- the directly touched compiler surface recloses before running broader gates

## 4. Representative Canary Gate

Choose one primary representative canary for the changed surface.

Runtime/header-oriented change:

```bash
make final-bucket BUCKET=runtime-surface
```

Project-shape or CLI integration change:

```bash
make realproj-stage-a REAL_PROJECT=fisiCs
```

Pass rule:

- at least one realistic non-trivial surface stays green beyond the narrowest unit of change

Selection rule:

- runtime/header-oriented compiler change:
  use `make final-bucket BUCKET=runtime-surface`
- CLI integration, repo-shape, or release-lane change:
  use `make realproj-stage-a REAL_PROJECT=fisiCs`
- memory-check overlay change:
  run `make memory-check-test`, then
  `make final-bucket BUCKET=runtime-surface` when runtime behavior or global
  final membership changes
- only run both canaries when the change clearly affects both categories

## 5. Broad Checkpoint Gate

Use this at real checkpoint boundaries, not after every tiny edit.

```bash
make final-monitored
```

Pass rule:

- broad compiler checkpoint remains green

## 6. Release Archive / Verify Gate

```bash
make release-archive
make release-verify
```

Pass rule:

- release artifacts are generated
- archive checksums and verification expectations close cleanly

## Notes

- Signed/notarized macOS distribution is a separate environment-bound lane.
- `spctl` may still report a non-app rejection for raw CLI binaries during
  local verification; treat codesign/archive verification as the required local
  gate and notarization as a separate release-host lane.
- This checklist does not replace the full internal suite; it is the public
  contributor-facing minimum release-confidence contract.
