# CLI Release Workflow

This doc defines the `fisiCs` CLI release lane for portable CLI archives and
macOS signing/notarization.

## Release Targets

Run from the `fisiCs/` repo root.

1. `make release-contract`
2. `make release-archive`
3. `make release-verify`
4. `make release-sign APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)"`
5. `make release-notarize APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)" APPLE_NOTARY_PROFILE="<profile>"`
6. `make release-verify-authenticated`

`release-verify` only inspects the existing staged tree and archives; it does
not rebuild or replace them. Use `release-archive` (or `release-sign`) first
whenever new package bytes are required. This keeps the final verification
from silently replacing signed or notarized artifacts.

`release-notarize` now finalizes one exact manifest for each downloadable
artifact only after Apple returns `Accepted`. `release-verify-authenticated`
then independently requires the Developer ID signature, accepted notary
submission, archive-byte parity, checksums, and both format-specific manifests.
Raw CLI archives are notarized as submitted containers; unlike `.app` or
installer bundles, the CLI binary has no stapling surface, so its manifest
records `stapling=not_applicable`.

Optional installer lane:

1. `make release-pkg APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)" APPLE_INSTALLER_IDENTITY="Developer ID Installer: <Name> (<TEAMID>)"`

## Output Layout

All release artifacts are written under `build/release/`.

- Stage tree: `build/release/stage/fisiCs-<version>-<platform>-<arch>-<channel>/`
- Zip archive: `build/release/fisiCs-<version>-<platform>-<arch>-<channel>.zip`
- Tarball: `build/release/fisiCs-<version>-<platform>-<arch>-<channel>.tar.gz`
- Checksums: `build/release/fisiCs-<version>-<platform>-<arch>-<channel>.sha256`
- ZIP manifest: `build/release/fisiCs-<version>-<platform>-<arch>-<channel>.zip.manifest.txt`
- Tarball manifest: `build/release/fisiCs-<version>-<platform>-<arch>-<channel>.tar.gz.manifest.txt`
- Compatibility manifest: `build/release/fisiCs-<version>-<platform>-<arch>-<channel>.manifest.txt`
- Notary response JSON: `build/release/fisiCs-<version>-<platform>-<arch>-<channel>.notary.json`

## Required Variables

- `APPLE_SIGN_IDENTITY`:
  - `Developer ID Application: <Name> (<TEAMID>)`
- `APPLE_NOTARY_PROFILE`:
  - `notarytool` keychain profile name (for example `cosm-notary`)

Optional:

- `APPLE_INSTALLER_IDENTITY` for `.pkg` lane:
  - `Developer ID Installer: <Name> (<TEAMID>)`

## Naming Contract

Defaults:

- `RELEASE_VERSION=0.1.0`
- `RELEASE_CHANNEL=stable`
- `RELEASE_PLATFORM=macOS`
- `RELEASE_ARCH=arm64`

Override example:

```bash
make release-archive RELEASE_VERSION=0.1.1 RELEASE_CHANNEL=beta
```

Linux x86_64 example:

```bash
make release-all RELEASE_PLATFORM=linux RELEASE_ARCH=x86_64 RELEASE_CHANNEL=stable
```

The zip archive target uses `scripts/create_release_zip.sh`, which selects
Python's byte-preserving `zipfile` implementation before the system `zip` or
`ditto` fallbacks. The CLI package does not need macOS resource forks, and the
preferred path preserves the embedded Developer ID signature in source and
archive bytes.

## Verification Expectations

- `release-verify` checks:
  - staged binary exists and is executable
  - the macOS binary has no Homebrew- or machine-local dynamic-library paths;
    LLVM is linked into the release binary and only system-relative dynamic
    dependencies are accepted
  - strict `codesign --verify` for the staged macOS binary
  - `spctl --assess` is informational for CLI binaries and may report non-app rejection even when signing/notarization are correct
  - archive checksum matches recorded `.sha256`
  - both archives extract successfully, retain the exact staged binary bytes,
    pass strict signature and portability checks, and run `--version`/`--help`
- `release-notarize` fails closed unless Apple's JSON response reports
  `Accepted`; a completed submission with `Invalid` status is not a successful
  release gate
- authenticated manifests bind exactly one format, artifact basename, SHA-256,
  Developer ID identity/team, and notary submission identity
- generated `examples/**/build/` outputs are pruned from the release stage so
  stale unsigned demo binaries cannot enter the notarized archive

## Production Registry Broker Handoff

Credential visibility is execution-context-sensitive. A `security
find-identity` result produced by a sandboxed agent is informational only:
`0 valid identities found` is **not** evidence that the host credential is
absent and must not create a durable missing-identity blocker. The release
adapter automatically routes that state to the repository-owned host
named-profile status check. Only a host-context status that binds the configured
Developer ID label and fingerprint may authorize signing.

The current repository-owned host profile binds:

- release profile: `codework-apple-release`
- notary keychain profile: `cosm-notary`
- broker: `codework-apple-release-v1`

Host status and execution retain only the public certificate label/fingerprint,
the `Accepted` notary result and submission ID, exact artifact/checksum/manifest
bindings, and the sanitized detached broker attestation. They never retain a
credential path, private key, notary credential, or raw command output.

Decision-bound local preparation creates fresh unsigned ZIP and tar.gz
evidence first under
`build/release-preparations/<preparation-id>/`; this prevents a repaired intent
from overwriting an earlier failed artifact with the same public version.
A credential-bearing Release Control operator then runs the
named `codework-apple-release` signing/notary profile, retains both
authenticated artifacts as an exact replacement proof, and schedules the
authentication service against those replacement bytes. The allowlisted
`codework-apple-release-v1` broker independently re-extracts and verifies each
CLI archive, emits a sanitized attestation, and signs only that attestation
under the `codework-release-artifact-authentication-v1` namespace. Registry
retains no Apple credential, signing-key path, or command output.

Authenticated ZIP/tar.gz replacements must be written under
`build/release-authenticated/<preparation-id>/`, outside the immutable
package-evidence root. Retain their exact replacement proof before scheduling
authentication so each authenticated archive remains bound to the original
artifact shape and digest.

## CI Guardrail Lane

Run a single command for ABI-sync and packaging-safety checks:

```bash
make ci-guardrails
```

This lane verifies:

- frontend archive refresh for IDE-consumer artifacts (`frontend-rebuild-ide`)
- frontend contract/API test buckets
- required contract capability-lane checks (`frontend_api_contract_capabilities` source + matrix entry)
- required frontend API symbols in `libfisics_frontend.a`
- release artifact generation + verification (`manifest`, `zip`, `tar.gz`, `sha256`)

## Release Bridge (FC-RL4/5/6)

Prepare and verify a release bridge bundle for distribution, Homebrew, and IDE integration contracts:

```bash
make release-bridge
```

Outputs are created under:

- `build/release/bridge/fisiCs-<version>-macOS-<arch>-<channel>/`

Bundle contents include:

- `artifacts/` copied release files (`zip`, `tar.gz`, `manifest`, `sha256`, optional notary log)
- `release_index.json` with immutable versioned URLs + checksums
- `homebrew/fisics.rb` formula template pinned to the hosted tarball checksum
- `ide/fisics_compiler_discovery_contract.md` discovery-order contract
- `publish/upload_commands.sh` and `publish/release_publish_checklist.md`

Configurable bridge variables (defaults in `makefile`):

- `RELEASE_PUBLIC_BASE_URL` (example: `https://downloads.example.com/fisiCs`)
- `RELEASE_TAP_REPO` (example: `owner/homebrew-fisics`)
- `RELEASE_HOMEPAGE` (example: `https://github.com/owner/fisiCs`)
- `RELEASE_VPS_TARGET` (example: `user@example.com`)
- `RELEASE_VPS_ROOT` (example: `/var/www/downloads/fisiCs`)

## Release Boundary

This lane ends at signed/notarized local artifacts.
Publishing to domain/VPS and Homebrew tap updates use the generated bridge bundle/checklist.
