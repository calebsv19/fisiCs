#!/usr/bin/env bash
set -euo pipefail

require_file() {
  local path="$1"
  if [ ! -f "$path" ]; then
    echo "release-bridge error: required file missing: $path" >&2
    exit 1
  fi
}

RELEASE_ROOT="${RELEASE_ROOT:-build/release}"
RELEASE_BASENAME="${RELEASE_BASENAME:?RELEASE_BASENAME is required}"
RELEASE_VERSION="${RELEASE_VERSION:?RELEASE_VERSION is required}"
RELEASE_CHANNEL="${RELEASE_CHANNEL:?RELEASE_CHANNEL is required}"
RELEASE_PLATFORM="${RELEASE_PLATFORM:?RELEASE_PLATFORM is required}"
RELEASE_ARCH="${RELEASE_ARCH:?RELEASE_ARCH is required}"
RELEASE_ARTIFACT_ZIP="${RELEASE_ARTIFACT_ZIP:?RELEASE_ARTIFACT_ZIP is required}"
RELEASE_ARTIFACT_TGZ="${RELEASE_ARTIFACT_TGZ:?RELEASE_ARTIFACT_TGZ is required}"
RELEASE_MANIFEST="${RELEASE_MANIFEST:?RELEASE_MANIFEST is required}"
RELEASE_SHA256="${RELEASE_SHA256:?RELEASE_SHA256 is required}"
RELEASE_NOTARY_LOG="${RELEASE_NOTARY_LOG:-}"

RELEASE_PUBLIC_BASE_URL="${RELEASE_PUBLIC_BASE_URL:-https://downloads.example.com/fisiCs}"
RELEASE_TAP_REPO="${RELEASE_TAP_REPO:-owner/homebrew-fisics}"
RELEASE_HOMEPAGE="${RELEASE_HOMEPAGE:-https://github.com/owner/fisiCs}"
RELEASE_VPS_TARGET="${RELEASE_VPS_TARGET:-user@example.com}"
RELEASE_VPS_ROOT="${RELEASE_VPS_ROOT:-/var/www/downloads/fisiCs}"

require_file "$RELEASE_ARTIFACT_ZIP"
require_file "$RELEASE_ARTIFACT_TGZ"
require_file "$RELEASE_MANIFEST"
require_file "$RELEASE_SHA256"
if [ -n "$RELEASE_NOTARY_LOG" ] && [ ! -f "$RELEASE_NOTARY_LOG" ]; then
  echo "release-bridge warning: notary log missing at $RELEASE_NOTARY_LOG; continuing"
fi

BRIDGE_DIR="$RELEASE_ROOT/bridge/$RELEASE_BASENAME"
ARTIFACT_DIR="$BRIDGE_DIR/artifacts"
HOMEBREW_DIR="$BRIDGE_DIR/homebrew"
IDE_DIR="$BRIDGE_DIR/ide"
PUBLISH_DIR="$BRIDGE_DIR/publish"

mkdir -p "$ARTIFACT_DIR" "$HOMEBREW_DIR" "$IDE_DIR" "$PUBLISH_DIR"

cp -f "$RELEASE_ARTIFACT_ZIP" "$ARTIFACT_DIR/"
cp -f "$RELEASE_ARTIFACT_TGZ" "$ARTIFACT_DIR/"
cp -f "$RELEASE_MANIFEST" "$ARTIFACT_DIR/"
cp -f "$RELEASE_SHA256" "$ARTIFACT_DIR/"
if [ -n "$RELEASE_NOTARY_LOG" ] && [ -f "$RELEASE_NOTARY_LOG" ]; then
  cp -f "$RELEASE_NOTARY_LOG" "$ARTIFACT_DIR/"
fi

ZIP_SHA="$(shasum -a 256 "$RELEASE_ARTIFACT_ZIP" | awk '{print $1}')"
TGZ_SHA="$(shasum -a 256 "$RELEASE_ARTIFACT_TGZ" | awk '{print $1}')"
VERSION_URL="$RELEASE_PUBLIC_BASE_URL/$RELEASE_VERSION"
ZIP_NAME="$(basename "$RELEASE_ARTIFACT_ZIP")"
TGZ_NAME="$(basename "$RELEASE_ARTIFACT_TGZ")"
MANIFEST_NAME="$(basename "$RELEASE_MANIFEST")"
SHA_NAME="$(basename "$RELEASE_SHA256")"

cat > "$BRIDGE_DIR/release_index.json" <<EOF
{
  "name": "fisiCs",
  "version": "$RELEASE_VERSION",
  "channel": "$RELEASE_CHANNEL",
  "platform": "$RELEASE_PLATFORM",
  "arch": "$RELEASE_ARCH",
  "base_url": "$VERSION_URL",
  "artifacts": {
    "zip": {
      "file": "$ZIP_NAME",
      "url": "$VERSION_URL/$ZIP_NAME",
      "sha256": "$ZIP_SHA"
    },
    "tar_gz": {
      "file": "$TGZ_NAME",
      "url": "$VERSION_URL/$TGZ_NAME",
      "sha256": "$TGZ_SHA"
    },
    "manifest": {
      "file": "$MANIFEST_NAME",
      "url": "$VERSION_URL/$MANIFEST_NAME"
    },
    "sha256_file": {
      "file": "$SHA_NAME",
      "url": "$VERSION_URL/$SHA_NAME"
    }
  }
}
EOF

cat > "$HOMEBREW_DIR/fisics.rb" <<EOF
class Fisics < Formula
  desc "fisiCs experimental C compiler"
  homepage "$RELEASE_HOMEPAGE"
  url "$VERSION_URL/$TGZ_NAME"
  sha256 "$TGZ_SHA"
  license "Apache-2.0"

  def install
    bin.install "bin/fisics"
    doc.install "README.md", "LICENSE"
  end

  test do
    system "\#{bin}/fisics", "--help"
  end
end
EOF

cat > "$IDE_DIR/fisics_compiler_discovery_contract.md" <<EOF
# fisiCs Compiler Discovery Contract (Release Bridge)

Version: $RELEASE_VERSION
Channel: $RELEASE_CHANNEL

This contract defines the release-facing compiler discovery order for IDE/runtime integration lanes.

## Discovery Order

1. \`FISICS_BIN\` environment override (absolute path recommended).
2. Bundled release path (if IDE or wrapper provisions one).
3. \`PATH\` lookup (\`fisics\`).

## Expectations

- Consumers should validate executable presence and run \`fisics --help\` as a smoke check.
- If resolution fails, consumers should degrade gracefully and show actionable diagnostics.
- This contract is terminal-first and remains compatible with manual workflows.
EOF

cat > "$PUBLISH_DIR/upload_commands.sh" <<EOF
#!/usr/bin/env bash
set -euo pipefail

# Edit target/root as needed for your host.
TARGET="$RELEASE_VPS_TARGET"
ROOT="$RELEASE_VPS_ROOT"
VERSION="$RELEASE_VERSION"

REMOTE_DIR="\$ROOT/\$VERSION"
LOCAL_DIR="\$(cd "\$(dirname "\${BASH_SOURCE[0]}")/.." && pwd)/artifacts"

ssh "\$TARGET" "mkdir -p \"\$REMOTE_DIR\""
rsync -av --progress "\$LOCAL_DIR/" "\$TARGET:\$REMOTE_DIR/"

echo "Uploaded release artifacts to \$TARGET:\$REMOTE_DIR"
echo "Run remote checksums (example):"
echo "  ssh \$TARGET 'cd \"\$REMOTE_DIR\" && shasum -a 256 -c *.sha256'"
EOF
chmod +x "$PUBLISH_DIR/upload_commands.sh"

cat > "$PUBLISH_DIR/release_publish_checklist.md" <<EOF
# Release Publish Checklist ($RELEASE_BASENAME)

## FC-RL4 Distribution (VPS/domain)

1. Run \`make release-bridge\`.
2. Execute \`publish/upload_commands.sh\` from this bridge bundle.
3. Verify remote checksums:
   - \`shasum -a 256 -c *.sha256\`
4. Verify direct download URLs:
   - \`$VERSION_URL/$ZIP_NAME\`
   - \`$VERSION_URL/$TGZ_NAME\`
   - \`$VERSION_URL/$MANIFEST_NAME\`

## FC-RL5 Homebrew

1. Copy \`homebrew/fisics.rb\` into tap repo \`$RELEASE_TAP_REPO\`.
2. Commit formula update in tap repository.
3. Validate install locally:
   - \`brew update\`
   - \`brew install --build-from-source $RELEASE_TAP_REPO/fisics\`
   - \`fisics --help\`

## FC-RL6 IDE Integration Contract

1. Follow discovery order documented in \`ide/fisics_compiler_discovery_contract.md\`.
2. Validate explicit override:
   - \`FISICS_BIN=/abs/path/to/fisics <consumer-command>\`
3. Validate fallback PATH behavior:
   - ensure \`fisics\` is discoverable on \`PATH\`.
EOF

echo "release-bridge prepare complete: $BRIDGE_DIR"
