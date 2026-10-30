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

known_text="$tmpdir/parser_semicolon.txt"
"$BIN" --explain parser.expect_semicolon >"$known_text"
grep -q "Diagnostic 1001 (parser.expect_semicolon)" "$known_text"
grep -q "category: parser" "$known_text"
grep -q "stage: parse" "$known_text"
grep -q "common causes:" "$known_text"
grep -q "next action:" "$known_text"

known_code="$tmpdir/link_stage.txt"
"$BIN" --explain 7101 >"$known_code"
grep -q "Diagnostic 7101 (link.stage_failed)" "$known_code"
grep -q "stage: link" "$known_code"

argument_text="$tmpdir/argument.txt"
"$BIN" --explain 4112 >"$argument_text"
grep -q "Diagnostic 4112 (extension.units.argument_dim_mismatch)" "$argument_text"
grep -q "category: extension" "$argument_text"

for pair in 4113:return_dim_mismatch 4114:return_contract_mismatch; do
  "$BIN" --explain "${pair%%:*}" >"$tmpdir/return.txt"
  grep -q "extension.units.${pair#*:}" "$tmpdir/return.txt"
done
"$BIN" --explain 4115 >"$tmpdir/sqrt.txt"
grep -q "extension.units.sqrt_unsupported" "$tmpdir/sqrt.txt"
"$BIN" --explain 4116 >"$tmpdir/math.txt"
grep -q "extension.units.math_argument_dim_mismatch" "$tmpdir/math.txt"
unknown_err="$tmpdir/unknown.err"
if "$BIN" --explain not.a.real.diagnostic >"$tmpdir/unknown.out" 2>"$unknown_err"; then
  echo "--explain unexpectedly accepted an unknown diagnostic" >&2
  exit 1
fi
grep -q "unknown diagnostic code or name" "$unknown_err"

json_path="$tmpdir/diagnostics.json"
"$BIN" --list-diagnostics --json >"$json_path"

python3 - "$json_path" <<'PY'
import json
import sys

path = sys.argv[1]
with open(path, "r", encoding="utf-8") as fh:
    data = json.load(fh)

if data.get("profile") != "fisics_diagnostic_explanations":
    raise SystemExit(f"unexpected profile: {data!r}")
if data.get("schema_version") != 1:
    raise SystemExit(f"unexpected schema version: {data!r}")

diagnostics = data.get("diagnostics")
if not isinstance(diagnostics, list) or not diagnostics:
    raise SystemExit(f"missing diagnostics list: {data!r}")

by_name = {diag.get("code_name"): diag for diag in diagnostics}
for required in [
    "parser.expect_semicolon",
    "semantic.generic",
    "preprocessor.generic",
    "link.stage_failed",
    "extension.units.conversion_incompatible",
    "extension.units.argument_dim_mismatch",
    "extension.units.return_dim_mismatch",
    "extension.units.return_contract_mismatch",
    "extension.units.sqrt_unsupported",
    "extension.units.math_argument_dim_mismatch",
]:
    if required not in by_name:
        raise SystemExit(f"missing explanation for {required}: {data!r}")

semicolon = by_name["parser.expect_semicolon"]
if semicolon.get("category_name") != "parser" or semicolon.get("stage") != "parse":
    raise SystemExit(f"bad parser semicolon metadata: {semicolon!r}")
for key in ["description", "common_causes", "next_action"]:
    if not isinstance(semicolon.get(key), str) or not semicolon[key]:
        raise SystemExit(f"missing {key}: {semicolon!r}")
PY
