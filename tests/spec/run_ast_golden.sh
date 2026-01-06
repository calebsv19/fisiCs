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
  if [ -n "$SPEC_FILTER" ] && [ "$rel" != "$SPEC_FILTER" ]; then
    continue
  fi
  if [[ "$rel" == parser/gnu_* ]] && [ -z "${ENABLE_GNU_STATEMENT_EXPRESSIONS:-}" ]; then
    continue
  fi
  name="${rel%.c}"
  golden_path="${GOLDEN_ROOT}/${name}.golden"
  golden_dir="$(dirname "$golden_path")"

  tmp_output=$(mktemp)
  filtered_output=$(mktemp)

  if ! "$BIN" "$src" >"$tmp_output" 2>&1; then
    echo "Compilation failed for $src" >&2
    cat "$tmp_output" >&2
    rm -f "$tmp_output" "$filtered_output"
    exit 1
  fi

  # Optional strict mode: fail fast if any diagnostics appear in raw output.
  # Enable by setting STRICT_RAW_ERRORS=1 when regenerating goldens to catch
  # unexpected parse/lex errors that might be filtered out.
  if [ "${STRICT_RAW_ERRORS:-0}" = "1" ] && grep -q "Error:" "$tmp_output"; then
    echo "Diagnostics produced for $src; refusing to update/compare golden (STRICT_RAW_ERRORS)." >&2
    cat "$tmp_output" >&2
    rm -f "$tmp_output" "$filtered_output"
    exit 1
  fi

  extract_sections "$tmp_output" "$filtered_output"

  if [ ! -s "$filtered_output" ]; then
    echo "Warning: filtered spec output empty for $src" >&2
  fi

  if [ "$UPDATE_GOLDENS" = "1" ] || [ ! -f "$golden_path" ]; then
    mkdir -p "$golden_dir"
    cp "$filtered_output" "$golden_path"
    echo "Updated golden for $src -> $golden_path"
  else
    if ! diff -u "$golden_path" "$filtered_output" > /tmp/specdiff.$$; then
      echo "Spec mismatch for $src" >&2
      cat /tmp/specdiff.$$ >&2
      rm -f /tmp/specdiff.$$
      status=1
    else
      rm -f /tmp/specdiff.$$
      echo "Spec OK for $src"
    fi
  fi

  rm -f "$tmp_output" "$filtered_output"
done

exit $status
