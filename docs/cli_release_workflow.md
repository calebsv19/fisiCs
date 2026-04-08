# CLI Release Workflow (macOS)

This doc defines the `fisiCs` CLI release lane for macOS without `.app` packaging.

## Release Targets

Run from the `fisiCs/` repo root.

1. `make release-contract`
2. `make release-archive`
3. `make release-verify`
4. `make release-sign APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)"`
5. `make release-notarize APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)" APPLE_NOTARY_PROFILE="<profile>"`

Optional installer lane:

1. `make release-pkg APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)" APPLE_INSTALLER_IDENTITY="Developer ID Installer: <Name> (<TEAMID>)"`

## Output Layout

All release artifacts are written under `build/release/`.

- Stage tree: `build/release/stage/fisiCs-<version>-macOS-<arch>-<channel>/`
- Zip archive: `build/release/fisiCs-<version>-macOS-<arch>-<channel>.zip`
- Tarball: `build/release/fisiCs-<version>-macOS-<arch>-<channel>.tar.gz`
- Checksums: `build/release/fisiCs-<version>-macOS-<arch>-<channel>.sha256`
- Manifest: `build/release/fisiCs-<version>-macOS-<arch>-<channel>.manifest.txt`
- Notary response JSON: `build/release/fisiCs-<version>-macOS-<arch>-<channel>.notary.json`

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

## Verification Expectations

- `release-verify` checks:
  - staged binary exists and is executable
  - `codesign --verify` (passes once signed)
  - `spctl --assess` is informational for CLI binaries and may report non-app rejection even when signing/notarization are correct
  - archive checksum matches recorded `.sha256`

## Release Boundary

This lane ends at signed/notarized local artifacts.
Publishing to domain/VPS and Homebrew tap updates are separate downstream steps.
