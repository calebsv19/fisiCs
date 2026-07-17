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

main_src="$tmpdir/include_stack_main.c"
header_src="$tmpdir/include_stack_bad.h"
json_path="$tmpdir/include_stack.json"

cat >"$main_src" <<'SRC'
#include "include_stack_bad.h"

int main(void) {
    return VALUE;
}
SRC

cat >"$header_src" <<'SRC'
#if 0
#endif
#endif
#define VALUE 1
SRC

compiler_status=0
"$BIN" --emit-diags-json "$json_path" -I"$tmpdir" "$main_src" >/dev/null 2>&1 || compiler_status=$?
if [ "$compiler_status" -ne 1 ]; then
  echo "expected compiler exit 1 for malformed included header; got $compiler_status" >&2
  exit 1
fi

python3 - "$json_path" "$main_src" "$header_src" <<'PY'
import json
import os
import sys

json_path, main_src, header_src = sys.argv[1:4]
with open(json_path, "r", encoding="utf-8") as fh:
    data = json.load(fh)

for diag in data.get("diagnostics", []):
    if diag.get("category_name") != "preprocessor":
        continue
    stack = diag.get("include_stack")
    if not stack:
        continue
    files = [os.path.basename(frame.get("file", "")) for frame in stack]
    if os.path.basename(main_src) in files and os.path.basename(header_src) in files:
        if stack[-1].get("resolved") is not True:
            raise SystemExit(f"last include stack frame should be resolved: {stack}")
        if "origin" not in stack[-1]:
            raise SystemExit(f"include stack frame missing origin: {stack}")
        break
else:
    raise SystemExit(f"missing preprocessor include_stack from {main_src} to {header_src}: {data}")
PY
