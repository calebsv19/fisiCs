#!/usr/bin/env bash
set -euo pipefail

stage_dir="${1:?usage: verify_release_archives.sh <stage-dir> <zip> <tar.gz> <platform>}"
zip_artifact="${2:?usage: verify_release_archives.sh <stage-dir> <zip> <tar.gz> <platform>}"
tgz_artifact="${3:?usage: verify_release_archives.sh <stage-dir> <zip> <tar.gz> <platform>}"
platform="${4:?usage: verify_release_archives.sh <stage-dir> <zip> <tar.gz> <platform>}"

stage_binary="$stage_dir/bin/fisics"
stage_runtime="$stage_dir/lib/fisics/libfisics_memcheck_runtime.a"
release_basename="$(basename "$stage_dir")"
tmp_root="$(mktemp -d "${TMPDIR:-/tmp}/fisics-release-verify.XXXXXX")"
trap 'rm -rf "$tmp_root"' EXIT

mkdir -p "$tmp_root/zip" "$tmp_root/tgz"
unzip -q "$zip_artifact" -d "$tmp_root/zip"
tar -xzf "$tgz_artifact" -C "$tmp_root/tgz"

for archive_kind in zip tgz; do
    extracted_binary="$tmp_root/$archive_kind/$release_basename/bin/fisics"
    extracted_runtime="$tmp_root/$archive_kind/$release_basename/lib/fisics/libfisics_memcheck_runtime.a"
    if [[ ! -x "$extracted_binary" ]]; then
        echo "ERROR: $archive_kind archive binary is missing or not executable: $extracted_binary" >&2
        exit 1
    fi
    if ! cmp -s "$stage_binary" "$extracted_binary"; then
        echo "ERROR: $archive_kind archive changed the staged binary bytes" >&2
        exit 1
    fi
    if [[ ! -f "$extracted_runtime" ]]; then
        echo "ERROR: $archive_kind archive memory-check runtime is missing: $extracted_runtime" >&2
        exit 1
    fi
    if ! cmp -s "$stage_runtime" "$extracted_runtime"; then
        echo "ERROR: $archive_kind archive changed the staged memory-check runtime bytes" >&2
        exit 1
    fi
    bash scripts/verify_release_portability.sh "$extracted_binary" "$platform"
    if [[ "$platform" == "macOS" ]]; then
        codesign --verify --deep --strict --verbose=2 "$extracted_binary"
    fi
    "$extracted_binary" --version
    "$extracted_binary" --help >/dev/null
done

echo "release archive extraction verification complete."
