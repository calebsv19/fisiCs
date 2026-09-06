#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="${1:-$ROOT/fisics}"
CLANG_BIN="${CLANG:-clang}"
OUT="${2:-$ROOT/build/examples/semantic_metadata/portable}"
SOURCE="$ROOT/examples/semantic_metadata/portable/main.c"

mkdir -p "$OUT"

FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units,semantic-metadata \
  -I"$ROOT/include" \
  --emit-semantic-metadata-json "$OUT/metadata.json" \
  --emit-units-metadata-json "$OUT/units.json" \
  -c "$SOURCE" -o "$OUT/fisics-metadata.o"

FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units,semantic-metadata \
  -I"$ROOT/include" \
  "$SOURCE" -o "$OUT/fisics-bin"
"$BIN" --version >"$OUT/fisics-version.txt"
"$OUT/fisics-bin" >"$OUT/fisics.stdout"

"$CLANG_BIN" -std=c99 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" \
  "$SOURCE" -o "$OUT/clang-bin"
"$CLANG_BIN" --version >"$OUT/clang-version.txt"
"$OUT/clang-bin" >"$OUT/clang.stdout"

cmp "$OUT/fisics.stdout" "$OUT/clang.stdout"
grep -qx 'portable temperature=294.0 K' "$OUT/fisics.stdout"

python3 - "$OUT/metadata.json" "$OUT/units.json" <<'PY'
import json
import pathlib
import sys

metadata = json.loads(pathlib.Path(sys.argv[1]).read_text())
units = json.loads(pathlib.Path(sys.argv[2]).read_text())
ids = {record["authored_id"] for record in metadata["records"]
       if record["authored_id"] is not None}
if "signal.portable.temperature" not in ids:
    raise SystemExit("portable fisiCs build did not retain semantic identity")
bindings = units.get("bindings", units.get("attachments", []))
if not bindings:
    raise SystemExit("portable fisiCs build did not retain units evidence")
PY

echo "Clang/fisiCs portable extension comparison passed"
