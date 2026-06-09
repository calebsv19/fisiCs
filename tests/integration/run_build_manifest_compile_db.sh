#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

ROOT="$(mktemp -d "${TMPDIR:-/tmp}/fisics_manifest_compile_db.XXXXXX")"
COMPILE_DB="$(mktemp)"
trap 'rm -rf "$ROOT"; rm -f "$COMPILE_DB"' EXIT

mkdir -p "$ROOT/src" "$ROOT/include" "$ROOT/build/fisics"
cat > "$ROOT/src/main.c" <<'EOF'
int lib(void);
int main(void) { return lib(); }
EOF
cat > "$ROOT/src/lib.c" <<'EOF'
int lib(void) { return 11; }
EOF

cat > "$ROOT/project.json" <<'EOF'
{
  "schema": "fisiCs.project",
  "version": 0,
  "name": "sample_compile_db",
  "root": ".",
  "build_dir": "build/fisics",
  "defaults": {
    "standard": "c99",
    "include_dirs": ["include"],
    "defines": ["FISICS_COMPILE_DB=1"],
    "overlays": ["physics-units"]
  },
  "translation_units": [
    {"source": "src/main.c", "object": "build/fisics/main.o"},
    {"source": "src/lib.c"}
  ]
}
EOF

"$BIN" --build-manifest "$ROOT/project.json" --emit-compile-db "$COMPILE_DB" >/dev/null

require_text() {
  local needle="$1"
  local label="$2"
  if ! grep -Fq -- "$needle" "$COMPILE_DB"; then
    echo "Missing ${label} in compile database" >&2
    cat "$COMPILE_DB" >&2
    exit 1
  fi
}

require_text '"directory":' "directory field"
require_text '"file":' "file field"
require_text '"output":' "output field"
require_text '"arguments":' "arguments field"
require_text '"command":' "command field"
require_text "$ROOT/src/main.c" "main source path"
require_text "$ROOT/src/lib.c" "lib source path"
require_text "$ROOT/build/fisics/main.o" "explicit object path"
require_text "$ROOT/build/fisics/lib.o" "derived object path"
require_text '"-std=c99"' "standard arg"
require_text '"-I"' "include arg flag"
require_text '"include"' "include arg value"
require_text 'FISICS_COMPILE_DB=1' "define arg"
require_text 'physics-units' "overlay arg"

python3 - "$COMPILE_DB" <<'PY'
import json
import sys
path = sys.argv[1]
with open(path, "r", encoding="utf-8") as f:
    data = json.load(f)
if len(data) != 2:
    raise SystemExit(f"expected 2 compile commands, got {len(data)}")
for entry in data:
    args = entry.get("arguments")
    if not isinstance(args, list) or "-c" not in args or "-o" not in args:
        raise SystemExit(f"bad arguments entry: {entry}")
PY

echo "build manifest compile database test passed."
