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

link_main="$tmpdir/link_main.c"
link_lib="$tmpdir/link_lib.c"
link_json="$tmpdir/link.json"
cross_main="$tmpdir/cross_main.c"
cross_lib="$tmpdir/cross_lib.c"
cross_json="$tmpdir/cross.json"

cat >"$link_main" <<'SRC'
int duplicated(void) { return 1; }
int main(void) { return duplicated(); }
SRC

cat >"$link_lib" <<'SRC'
int duplicated(void) { return 2; }
SRC

cat >"$cross_main" <<'SRC'
int shared_value = 1;
int main(void) { return shared_value; }
SRC

cat >"$cross_lib" <<'SRC'
long shared_value = 2;
SRC

"$BIN" --emit-diags-json "$link_json" "$link_main" "$link_lib" -o "$tmpdir/link_out" >/dev/null 2>&1 || true
"$BIN" --emit-diags-json "$cross_json" "$cross_main" "$cross_lib" -o "$tmpdir/cross_out" >/dev/null 2>&1 || true

python3 - "$link_json" "$cross_json" <<'PY'
import json
import sys

link_path, cross_path = sys.argv[1:3]

def load_one(path, label):
    with open(path, "r", encoding="utf-8") as fh:
        data = json.load(fh)
    diagnostics = data.get("diagnostics") or []
    if len(diagnostics) != 1:
        raise SystemExit(f"{label}: expected exactly one diagnostic, got {len(diagnostics)}")
    return diagnostics[0]

def require(diag, label, **fields):
    for key, want in fields.items():
        got = diag.get(key)
        if got != want:
            raise SystemExit(f"{label}: expected {key}={want!r}, got {got!r}: {diag}")

for label, diag in (
    ("link", load_one(link_path, "link")),
    ("cross", load_one(cross_path, "cross")),
):
    for key in (
        "severity_id",
        "severity_name",
        "category_id",
        "category_name",
        "code_id",
        "code_name",
        "stage",
    ):
        if key not in diag:
            raise SystemExit(f"{label}: missing {key}: {diag}")

require(
    load_one(link_path, "link"),
    "link",
    code=7101,
    code_id=7101,
    code_name="link.stage_failed",
    category_name="build",
    stage="link",
    severity_name="error",
    has_file=False,
)

require(
    load_one(cross_path, "cross"),
    "cross",
    code=2000,
    code_id=2000,
    code_name="semantic.generic",
    category_name="semantic",
    stage="semantic",
    severity_name="error",
    has_file=True,
)
PY
