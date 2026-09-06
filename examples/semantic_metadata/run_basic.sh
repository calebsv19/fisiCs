#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="${1:-$ROOT/fisics}"
OUT="${2:-$ROOT/build/examples/semantic_metadata/basic}"
SOURCE="$ROOT/examples/semantic_metadata/basic/main.c"

mkdir -p "$OUT"

FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata \
  -I"$ROOT/include" \
  --emit-semantic-metadata-json "$OUT/metadata.json" \
  -c "$SOURCE" -o "$OUT/main.o"

FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata \
  -I"$ROOT/include" \
  "$SOURCE" -o "$OUT/basic"
"$OUT/basic" >"$OUT/stdout.txt"

python3 - "$OUT/metadata.json" <<'PY'
import json
import pathlib
import sys

data = json.loads(pathlib.Path(sys.argv[1]).read_text())
ids = {record["authored_id"] for record in data["records"]
       if record["authored_id"] is not None}
required = {
    "signal.room.temperature",
    "method.room.observe",
    "method.room.control",
}
if not required.issubset(ids):
    raise SystemExit("semantic metadata example is missing expected authored IDs")
if len(data["edges"]) != 3:
    raise SystemExit("semantic metadata example expected exactly three relationships")
PY

grep -qx 'temperature=294.0 fan_target=0' "$OUT/stdout.txt"
echo "semantic metadata basic example passed"
