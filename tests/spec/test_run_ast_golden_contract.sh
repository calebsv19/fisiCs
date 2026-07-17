#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
RUNNER="$ROOT/tests/spec/run_ast_golden.sh"
BIN="${1:-$ROOT/fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

tmp_root=$(mktemp -d)
trap 'rm -rf "$tmp_root"' EXIT

run_case() {
  local label="$1"
  local expected_status="$2"
  local expected_text="$3"
  local expected_text_2="$4"
  shift 4
  local output="$tmp_root/${label}.out"
  local actual_status=0

  "$@" >"$output" 2>&1 || actual_status=$?
  if [ "$actual_status" -ne "$expected_status" ]; then
    echo "$label: got status $actual_status, expected $expected_status" >&2
    cat "$output" >&2
    exit 1
  fi
  if [ -n "$expected_text" ] && ! grep -Fq "$expected_text" "$output"; then
    echo "$label: missing expected output: $expected_text" >&2
    cat "$output" >&2
    exit 1
  fi
  if [ -n "$expected_text_2" ] && ! grep -Fq "$expected_text_2" "$output"; then
    echo "$label: missing expected output: $expected_text_2" >&2
    cat "$output" >&2
    exit 1
  fi
}

cd "$ROOT"

# Positive controls prove both sides of the 0/1 golden-derived contract.
run_case clean_fixture 0 "Spec OK for tests/parser/attributes.c" "" \
  env SPEC_FILTER=parser/attributes.c DISABLE_CODEGEN=1 "$RUNNER" "$BIN"
run_case expected_reject_fixture 0 "Spec OK for tests/syntax/case_non_ice.c" "" \
  env SPEC_FILTER=syntax/case_non_ice.c DISABLE_CODEGEN=1 "$RUNNER" "$BIN"

# A textual mismatch must not obscure a wrong compiler status.
run_case false_accept_reject 1 "Compiler exit status mismatch" "Spec mismatch" \
  env SPEC_FILTER=syntax/case_non_ice.c "$RUNNER" /usr/bin/true
run_case false_reject_clean 1 "Compiler exit status mismatch" "Spec mismatch" \
  env SPEC_FILTER=parser/attributes.c "$RUNNER" /usr/bin/false

# Exit values outside the compiler's exact 0/1 contract are never accepted.
run_case crash_or_abnormal_exit 1 "Invalid compiler exit status" "Spec mismatch" \
  env SPEC_FILTER=parser/attributes.c "$RUNNER" /bin/sh

# With no pre-existing golden, nonzero is fail-closed and cannot bless output.
missing_root="$tmp_root/missing-golden"
mkdir -p "$missing_root/tests/parser" "$missing_root/tests/spec/goldens"
touch "$missing_root/tests/parser/new_fixture.c"
run_case missing_golden_nonzero 1 "Refusing to create missing golden" "Compiler exit status mismatch" \
  env -C "$missing_root" "$RUNNER" /usr/bin/false
if [ -e "$missing_root/tests/spec/goldens/parser/new_fixture.golden" ]; then
  echo "missing_golden_nonzero: runner created a golden for a rejected fixture" >&2
  exit 1
fi

echo "AST golden runner contract OK"
