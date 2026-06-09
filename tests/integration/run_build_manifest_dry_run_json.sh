#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

ROOT="$(mktemp -d "${TMPDIR:-/tmp}/fisics_manifest_dry_run.XXXXXX")"
OUT_JSON="$(mktemp)"
BAD_JSON="$(mktemp)"
trap 'rm -rf "$ROOT"; rm -f "$OUT_JSON" "$BAD_JSON"' EXIT

mkdir -p "$ROOT/src" "$ROOT/include" "$ROOT/build/fisics"
cat > "$ROOT/src/main.c" <<'EOF'
int lib(void);
int main(void) { return lib(); }
EOF
cat > "$ROOT/src/lib.c" <<'EOF'
int lib(void) { return 7; }
EOF

cat > "$ROOT/project.json" <<'EOF'
{
  "schema": "fisiCs.project",
  "version": 0,
  "name": "sample",
  "root": ".",
  "build_dir": "build/fisics",
  "defaults": {
    "standard": "c99",
    "include_dirs": ["include"],
    "defines": ["FISICS_MANIFEST_DRY_RUN=1"],
    "overlays": ["physics-units"]
  },
  "translation_units": [
    {"source": "src/main.c", "object": "build/fisics/main.o"},
    {"source": "src/lib.c"}
  ],
  "link": {
    "output": "build/fisics/sample",
    "libraries": ["m"],
    "library_dirs": [],
    "args": ["-Wl,-dead_strip"]
  }
}
EOF

"$BIN" --build-manifest "$ROOT/project.json" --dry-run --json \
  --emit-build-graph-json "$OUT_JSON" >/dev/null

require_text() {
  local needle="$1"
  local label="$2"
  if ! grep -Fq -- "$needle" "$OUT_JSON"; then
    echo "Missing ${label} in manifest dry-run JSON" >&2
    cat "$OUT_JSON" >&2
    exit 1
  fi
}

require_text '"schema":"fisiCs.build_graph"' "schema"
require_text '"mode":"dry_run"' "dry-run mode"
require_text '"schema":"fisiCs.build_plan"' "plan schema"
require_text '"dry_run":true' "dry-run flag"
require_text '"kind":"compile"' "compile action"
require_text '"kind":"link"' "link action"
require_text '"will_execute":false' "non-execution marker"
require_text 'src/main.c' "main source"
require_text 'build/fisics/main.o' "explicit object"
require_text 'build/fisics/lib.o' "derived object"
require_text 'FISICS_MANIFEST_DRY_RUN=1' "manifest define"
require_text 'physics-units' "manifest overlay"
require_text '"libraries":["m"]' "link libraries"
require_text '-Wl,-dead_strip' "link args"
require_text '"input_exists":true' "source existence"
require_text '"diagnostic_summary"' "diagnostic summary"

python3 - "$OUT_JSON" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as handle:
    graph = json.load(handle)

expected = {
    "available": False,
    "total": 0,
    "errors": 0,
    "warnings": 0,
    "notes": 0,
    "partial": False,
    "fatal": False,
}
assert graph["diagnostic_summary"] == expected, graph["diagnostic_summary"]
for tu in graph["translation_units"]:
    assert tu["diagnostic_summary"] == expected, tu
for action in graph["plan"]["actions"]:
    if action["kind"] == "compile":
        assert action["diagnostic_summary"] == expected, action
PY

cat > "$ROOT/bad_project.json" <<'EOF'
{
  "schema": "fisiCs.project",
  "version": 0,
  "name": "bad",
  "translation_units": [
    {"source": "src/missing.c"}
  ]
}
EOF

if "$BIN" --build-manifest "$ROOT/bad_project.json" --dry-run --json \
  --emit-build-graph-json "$BAD_JSON" >/dev/null 2>"$ROOT/bad.err"; then
  echo "bad manifest unexpectedly succeeded" >&2
  cat "$BAD_JSON" >&2 || true
  exit 1
fi

if ! grep -Fq -- 'source does not exist' "$ROOT/bad.err"; then
  echo "missing-source manifest diagnostic was not clear" >&2
  cat "$ROOT/bad.err" >&2
  exit 1
fi

echo "build manifest dry-run JSON test passed."
