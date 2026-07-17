#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

shopt -s nullglob
TEST_SOURCES=(tests/parser/*.c tests/syntax/*.c)
if [ ${#TEST_SOURCES[@]} -eq 0 ]; then
  echo "No parser/syntax spec inputs found." >&2
  exit 1
fi

GOLDEN_ROOT="tests/spec/goldens"
UPDATE_GOLDENS="${UPDATE_GOLDENS:-0}"
SPEC_FILTER="${SPEC_FILTER:-}"
status=0

golden_expected_status() {
  local golden="$1"
  local exit_oracle="${golden}.exit"

  if [ -f "$exit_oracle" ]; then
    local explicit_status
    explicit_status="$(tr -d '[:space:]' < "$exit_oracle")"
    if [ "$explicit_status" != "0" ] && [ "$explicit_status" != "1" ]; then
      echo "Invalid golden exit oracle $exit_oracle: expected exactly 0 or 1" >&2
      return 2
    fi
    printf '%s\n' "$explicit_status"
    return
  fi

  # A pre-existing hard diagnostic is part of the golden contract: the
  # compiler must reject that fixture with the ordinary diagnostic exit (1).
  # Missing goldens are intentionally treated as success-only so a failing
  # compiler cannot bless a new expected-reject fixture implicitly.
  if [ -f "$golden" ] && grep -Eq '^Error([[:space:]]+at)?([[:space:]]|:)' "$golden"; then
    printf '1\n'
  else
    printf '0\n'
  fi
}

extract_sections() {
  local input="$1"
  local output="$2"
  awk '
    /^[[:space:]]*AST Output:/ {
        if (section != "") print ""
        section = "ast"
        print "AST:"
        next
    }
    /^[[:space:]]*Semantic Analysis:/ {
        if (section != "") print ""
        section = "diag"
        print "Diagnostics:"
        next
    }
    /^[[:space:]]*Semantic Model Dump:/ { section = ""; next }
    index($0, "LLVM Code Generation:") > 0 { section = ""; next }
    section == "ast" { print; next }
    section == "diag" { print; next }
  ' "$input" > "$output"
}

for src in "${TEST_SOURCES[@]}"; do
  rel="${src#tests/}"
  extra_args=()
  if [ "$src" = "tests/syntax/semantic_atomic_qualifier.c" ]; then
    extra_args=(-std=c11)
  fi
  if [ -n "$SPEC_FILTER" ] && [ "$rel" != "$SPEC_FILTER" ]; then
    continue
  fi
  if [[ "$rel" == parser/gnu_* ]] && [ -z "${ENABLE_GNU_STATEMENT_EXPRESSIONS:-}" ]; then
    continue
  fi
  name="${rel%.c}"
  golden_path="${GOLDEN_ROOT}/${name}.golden"
  golden_dir="$(dirname "$golden_path")"
  expected_status="$(golden_expected_status "$golden_path")"

  tmp_output=$(mktemp)
  filtered_output=$(mktemp)
  diff_output=$(mktemp)
  fixture_ok=1

  compiler_status=0
  "$BIN" "${extra_args[@]}" "$src" >"$tmp_output" 2>&1 || compiler_status=$?

  if [ "$compiler_status" -ne 0 ] && [ "$compiler_status" -ne 1 ]; then
    echo "Invalid compiler exit status for $src: got $compiler_status, expected exactly 0 or 1" >&2
    fixture_ok=0
    status=1
  elif [ "$compiler_status" -ne "$expected_status" ]; then
    echo "Compiler exit status mismatch for $src: got $compiler_status, expected $expected_status from $golden_path" >&2
    fixture_ok=0
    status=1
  fi

  # Optional strict mode: record a failure if diagnostics appear in raw output.
  # Enable by setting STRICT_RAW_ERRORS=1 when regenerating goldens to catch
  # unexpected parse/lex errors that might be filtered out.
  if [ "${STRICT_RAW_ERRORS:-0}" = "1" ] &&
     [ "$expected_status" -eq 0 ] &&
     grep -q "Error:" "$tmp_output"; then
    echo "Diagnostics produced for $src; refusing to update/compare golden (STRICT_RAW_ERRORS)." >&2
    cat "$tmp_output" >&2
    fixture_ok=0
    status=1
  fi

  extract_sections "$tmp_output" "$filtered_output"

  if [ ! -s "$filtered_output" ]; then
    echo "Warning: filtered spec output empty for $src" >&2
  fi

  if { [ "$UPDATE_GOLDENS" = "1" ] || [ ! -f "$golden_path" ]; } && [ "$fixture_ok" -eq 1 ]; then
    mkdir -p "$golden_dir"
    cp "$filtered_output" "$golden_path"
    echo "Updated golden for $src -> $golden_path"
  elif [ -f "$golden_path" ]; then
    if ! diff -u "$golden_path" "$filtered_output" > "$diff_output"; then
      echo "Spec mismatch for $src" >&2
      cat "$diff_output" >&2
      status=1
    else
      echo "Spec OK for $src"
    fi
  else
    echo "Refusing to create missing golden for failed fixture $src" >&2
    status=1
  fi

  rm -f "$tmp_output" "$filtered_output" "$diff_output"
done

exit $status
