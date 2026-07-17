#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 3 ]; then
  echo "usage: $0 <stage-root> <release-basename> <artifact-zip>" >&2
  exit 2
fi

stage_root="$1"
release_basename="$2"
artifact_zip="$3"

if [ ! -d "$stage_root/$release_basename" ]; then
  echo "release zip error: staged release tree missing: $stage_root/$release_basename" >&2
  exit 1
fi

mkdir -p "$(dirname "$artifact_zip")"

if command -v python3 >/dev/null 2>&1; then
  python3 - "$stage_root" "$release_basename" "$artifact_zip" <<'PY'
import os
import sys
import zipfile

stage_root, release_basename, artifact_zip = sys.argv[1:]
release_root = os.path.join(stage_root, release_basename)

with zipfile.ZipFile(artifact_zip, "w", compression=zipfile.ZIP_DEFLATED) as archive:
    for root, dirs, files in os.walk(release_root):
        dirs.sort()
        files.sort()
        rel_root = os.path.relpath(root, stage_root)
        if not dirs and not files:
            archive.writestr(rel_root.rstrip("/") + "/", "")
        for name in files:
            path = os.path.join(root, name)
            archive.write(path, os.path.relpath(path, stage_root))
PY
  exit 0
fi

if command -v zip >/dev/null 2>&1; then
  (cd "$stage_root" && zip -qr "$artifact_zip" "$release_basename")
  exit 0
fi

if command -v ditto >/dev/null 2>&1; then
  (cd "$stage_root" && ditto -c -k --keepParent "$release_basename" "$artifact_zip")
  exit 0
fi

echo "release zip error: need one of python3, zip, or ditto" >&2
exit 1
