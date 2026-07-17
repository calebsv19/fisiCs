#!/usr/bin/env bash
set -euo pipefail

binary="${1:?usage: verify_release_portability.sh <binary> <platform>}"
platform="${2:?usage: verify_release_portability.sh <binary> <platform>}"

if [[ ! -x "$binary" ]]; then
    echo "ERROR: release binary is missing or not executable: $binary" >&2
    exit 2
fi

if [[ "$platform" != "macOS" ]]; then
    echo "release portability check: no platform-specific inspection for $platform"
    exit 0
fi

if ! command -v otool >/dev/null 2>&1; then
    echo "ERROR: otool is required to verify a macOS release binary" >&2
    exit 2
fi

dependencies="$(otool -L "$binary" | tail -n +2 | awk '{print $1}')"
unexpected="$(printf '%s\n' "$dependencies" | grep -Ev '^(@|/usr/lib/|/System/Library/)' || true)"

if [[ -n "$unexpected" ]]; then
    echo "ERROR: macOS release binary has non-portable dynamic dependencies:" >&2
    printf '  %s\n' "$unexpected" >&2
    exit 1
fi

echo "release portability check: macOS dependencies are system-relative"
