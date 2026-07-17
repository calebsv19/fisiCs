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

src="$tmpdir/macro_trace_arg_count.c"
json_path="$tmpdir/macro_trace.json"

cat >"$src" <<'SRC'
#define NEED2(a, b) ((a) + (b))

int main(void) {
    return NEED2(1);
}
SRC

compiler_status=0
"$BIN" --emit-diags-json "$json_path" "$src" >/dev/null 2>&1 || compiler_status=$?
if [ "$compiler_status" -ne 1 ]; then
  echo "expected compiler exit 1 for macro arity diagnostic; got $compiler_status" >&2
  exit 1
fi

python3 - "$json_path" "$src" <<'PY'
import json
import os
import sys

json_path, src = sys.argv[1:3]
with open(json_path, "r", encoding="utf-8") as fh:
    data = json.load(fh)

for diag in data.get("diagnostics", []):
    if diag.get("category_name") != "preprocessor":
        continue
    trace = diag.get("macro_trace")
    if not trace:
        continue
    roles = {frame.get("role") for frame in trace}
    macros = {frame.get("macro") for frame in trace}
    files = {os.path.basename(frame.get("file", "")) for frame in trace}
    if {"call_site", "definition"}.issubset(roles) and "NEED2" in macros:
        if os.path.basename(src) not in files:
            raise SystemExit(f"macro trace should reference source file: {trace}")
        for frame in trace:
            if frame.get("role") in {"call_site", "definition"}:
                if frame.get("line", 0) <= 0 or frame.get("column", 0) <= 0:
                    raise SystemExit(f"macro trace frame missing location: {trace}")
        break
else:
    raise SystemExit(f"missing preprocessor macro_trace for NEED2 arity diagnostic: {data}")
PY
