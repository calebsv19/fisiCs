#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

tmpdir="$(mktemp -d)"
cleanup() {
  rm -rf "$tmpdir"
}
trap cleanup EXIT

parser_src="$tmpdir/parser_missing_semicolon.c"
parser_json="$tmpdir/parser_missing_semicolon.json"
semantic_src="$tmpdir/semantic_missing_symbol.c"
semantic_json="$tmpdir/semantic_missing_symbol.json"

cat >"$parser_src" <<'SRC'
int main(void) {
    int value = 1
    return value;
}
SRC

cat >"$semantic_src" <<'SRC'
int main(void) {
    return missing_symbol;
}
SRC

"$BIN" --emit-diags-json "$parser_json" "$parser_src" >/dev/null 2>&1 || true
"$BIN" --emit-diags-json "$semantic_json" "$semantic_src" >/dev/null 2>&1 || true

python3 - "$parser_json" "$semantic_json" <<'PY'
import json
import sys

parser_path, semantic_path = sys.argv[1:3]

def load(path):
    with open(path, "r", encoding="utf-8") as fh:
        return json.load(fh)

def require_diag(path, predicate, label):
    data = load(path)
    if data.get("profile") != "fisics_diagnostics_v1":
        raise SystemExit(f"{label}: unexpected profile {data.get('profile')!r}")
    if data.get("schema_version") != 1:
        raise SystemExit(f"{label}: unexpected schema_version {data.get('schema_version')!r}")
    for diag in data.get("diagnostics", []):
        required = [
            "severity_id",
            "severity_name",
            "category_id",
            "category_name",
            "code_id",
            "code_name",
            "stage",
        ]
        missing = [field for field in required if field not in diag]
        if missing:
            raise SystemExit(f"{label}: missing additive fields {missing}: {diag}")
        if predicate(diag):
            return
    raise SystemExit(f"{label}: no matching diagnostic in {path}")

require_diag(
    parser_path,
    lambda d: d["category_name"] == "parser"
    and d["stage"] == "parse"
    and d["severity_name"] == "error"
    and str(d["code_name"]).startswith("parser."),
    "parser",
)

require_diag(
    semantic_path,
    lambda d: d["category_name"] == "semantic"
    and d["stage"] == "semantic"
    and d["severity_name"] == "error"
    and d["code_name"] == "semantic.generic",
    "semantic",
)
PY
