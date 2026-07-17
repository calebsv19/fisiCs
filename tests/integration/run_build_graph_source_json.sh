#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/include_basic.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/fisics_graph.XXXXXX")"
OUT_JSON="$WORK_DIR/source.json"
UNRESOLVED_SRC="$WORK_DIR/unresolved.c"
UNRESOLVED_JSON="$WORK_DIR/unresolved.json"
COMPILE_ONLY_JSON="$WORK_DIR/compile_only.json"
COMPILE_ONLY_OBJ="$WORK_DIR/compile_only.o"
BAD_SRC="$WORK_DIR/bad.c"
BAD_JSON="$WORK_DIR/bad.json"
trap 'rm -rf "$WORK_DIR"' EXIT

"$BIN" --emit-build-graph-json "$OUT_JSON" -Iinclude -DFISICS_GRAPH_TEST=1 "$SRC" >/dev/null

if [ ! -s "$OUT_JSON" ]; then
  echo "Build graph JSON not written" >&2
  exit 1
fi

require_text() {
  local needle="$1"
  local label="$2"
  if ! grep -Fq -- "$needle" "$OUT_JSON"; then
    echo "Missing ${label} in build graph JSON" >&2
    cat "$OUT_JSON" >&2
    exit 1
  fi
}

require_text '"schema":"fisiCs.build_graph"' "schema"
require_text '"mode":"source"' "source mode"
require_text '"translation_units"' "translation unit list"
require_text 'include_basic.c' "source path"
require_text 'include_basic_local.h' "local include edge"
require_text 'include/pp_sys.h' "system include edge"
require_text 'FISICS_GRAPH_TEST=1' "macro define"
require_text '"include_dirs"' "include dirs"
require_text '"include_edges"' "include edges"
require_text '"include_records"' "include records"
require_text '"compile_args"' "compile args"
require_text '"-D"' "macro define compile arg flag"
require_text 'FISICS_GRAPH_TEST=1' "macro define compile arg value"
require_text '"diagnostic_summary"' "diagnostic summary"

python3 - "$OUT_JSON" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as handle:
    graph = json.load(handle)

summary = graph["diagnostic_summary"]
assert summary == {
    "available": True,
    "total": 0,
    "errors": 0,
    "warnings": 0,
    "notes": 0,
    "partial": False,
    "fatal": False,
}, summary

tu_summary = graph["translation_units"][0]["diagnostic_summary"]
assert tu_summary == summary, tu_summary
PY

cat > "$UNRESOLVED_SRC" <<'EOF'
#include <fisics_graph_missing_header_for_unresolved_record.h>
int main(void) { return 0; }
EOF

"$BIN" --emit-build-graph-json "$UNRESOLVED_JSON" "$UNRESOLVED_SRC" >/dev/null

if ! grep -Fq -- 'fisics_graph_missing_header_for_unresolved_record.h' "$UNRESOLVED_JSON"; then
  echo "Missing unresolved include name in build graph JSON" >&2
  cat "$UNRESOLVED_JSON" >&2
  exit 1
fi

if ! grep -Fq -- '"status":"unresolved"' "$UNRESOLVED_JSON"; then
  echo "Missing unresolved include status in build graph JSON" >&2
  cat "$UNRESOLVED_JSON" >&2
  exit 1
fi

"$BIN" -c "$SRC" -Iinclude -DFISICS_GRAPH_COMPILE_ONLY=1 \
  --emit-build-graph-json "$COMPILE_ONLY_JSON" -o "$COMPILE_ONLY_OBJ" >/dev/null

if ! grep -Fq -- "$COMPILE_ONLY_OBJ" "$COMPILE_ONLY_JSON"; then
  echo "Missing compile-only object path in build graph JSON" >&2
  cat "$COMPILE_ONLY_JSON" >&2
  exit 1
fi

if ! grep -Fq -- '"-c"' "$COMPILE_ONLY_JSON"; then
  echo "Missing compile-only flag in build graph JSON compile args" >&2
  cat "$COMPILE_ONLY_JSON" >&2
  exit 1
fi

if ! grep -Fq -- '"-D"' "$COMPILE_ONLY_JSON" ||
   ! grep -Fq -- 'FISICS_GRAPH_COMPILE_ONLY=1' "$COMPILE_ONLY_JSON"; then
  echo "Missing compile-only macro arg in build graph JSON compile args" >&2
  cat "$COMPILE_ONLY_JSON" >&2
  exit 1
fi

cat > "$BAD_SRC" <<'EOF'
int main(void) {
  return missing_symbol +
}
EOF

set +e
"$BIN" --emit-build-graph-json "$BAD_JSON" "$BAD_SRC" >/dev/null 2>&1
status=$?
set -e
if [ "$status" -ne 1 ]; then
  echo "Expected malformed source compiler exit 1, got $status" >&2
  exit 1
fi

python3 - "$BAD_JSON" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as handle:
    graph = json.load(handle)

summary = graph["diagnostic_summary"]
if not summary["available"]:
    raise SystemExit(f"diagnostic summary unavailable: {summary}")
if summary["total"] < 1 or summary["errors"] < 1:
    raise SystemExit(f"expected at least one error diagnostic: {summary}")
if not summary["partial"] or summary["fatal"]:
    raise SystemExit(f"bad partial/fatal state: {summary}")
if graph["translation_units"][0]["diagnostic_summary"] != summary:
    raise SystemExit("translation unit diagnostic summary does not match top-level source summary")
if graph["translation_units"][0]["status"] != "partial":
    raise SystemExit(f"bad translation unit status: {graph['translation_units'][0]['status']}")
PY

echo "build graph source JSON emission test passed."
